#include "tessellation.h"
#include "disc.h"

using namespace regen;

namespace regen {
	std::ostream &operator<<(std::ostream &out, const Disc::TexcoMode &mode) {
		switch (mode) {
			case Disc::TEXCO_MODE_NONE:
				return out << "NONE";
			case Disc::TEXCO_MODE_UV:
				return out << "UV";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, Disc::TexcoMode &mode) {
		std::string val;
		in >> val;
		boost::to_upper(val);
		if (val == "NONE") mode = Disc::TEXCO_MODE_NONE;
		else if (val == "UV") mode = Disc::TEXCO_MODE_UV;
		else {
			REGEN_WARN("Unknown texco mode '" << val << "'. Using NONE texco.");
			mode = Disc::TEXCO_MODE_NONE;
		}
		return in;
	}
}

ref_ptr<Disc> Disc::getUnitDisc() {
	static ref_ptr<Disc> mesh;
	if (mesh.get() == nullptr) {
		Config cfg;
		cfg.posScale = Vec3f(1.0f);
		cfg.rotation = Vec3f(0.0, 0.0f, 0.0f);
		cfg.texcoMode = TEXCO_MODE_NONE;
		cfg.isNormalRequired = GL_FALSE;
		cfg.isTangentRequired = GL_FALSE;
		cfg.discRadius = 1.0f;
		cfg.usage = VBO::USAGE_STATIC;
		mesh = ref_ptr<Disc>::alloc(cfg);
		return mesh;
	} else {
		return ref_ptr<Disc>::alloc(mesh);
	}
}

Disc::Disc(const Config &cfg)
		: Mesh(GL_TRIANGLES, cfg.usage) {
	pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	nor_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	tan_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);
	indices_ = ref_ptr<ShaderInput1ui>::alloc("i");
	updateAttributes(cfg);
}

Disc::Disc(const ref_ptr<Disc> &other)
		: Mesh(other) {
	pos_ = ref_ptr<ShaderInput3f>::dynamicCast(
			inputContainer_->getInput(ATTRIBUTE_NAME_POS));
	nor_ = ref_ptr<ShaderInput3f>::dynamicCast(
			inputContainer_->getInput(ATTRIBUTE_NAME_NOR));
	tan_ = ref_ptr<ShaderInput4f>::dynamicCast(
			inputContainer_->getInput(ATTRIBUTE_NAME_TAN));
	indices_ = ref_ptr<ShaderInput1ui>::dynamicCast(
			inputContainer_->getInput("i"));
}

Disc::Config::Config()
		: levelOfDetail(0),
		  posScale(Vec3f(1.0f)),
		  rotation(Vec3f(0.0f)),
		  texcoScale(Vec2f(1.0f)),
		  texcoMode(TEXCO_MODE_UV),
		  isNormalRequired(GL_TRUE),
		  isTangentRequired(GL_FALSE),
		  usage(VBO::USAGE_DYNAMIC),
		  discRadius(1.0f) {
}

void Disc::updateAttributes(const Config &cfg) {
	// Generate disc vertices
	const GLuint levelOfDetail = pow(4u, 1 + cfg.levelOfDetail);
	const GLuint numVertices = levelOfDetail + 1;
	const GLuint numIndices = levelOfDetail * 3;
	pos_->setVertexData(numVertices);
	if (cfg.isNormalRequired) {
		nor_->setVertexData(numVertices);
	}
	if (cfg.isTangentRequired) {
		tan_->setVertexData(numVertices);
	}
	if (cfg.texcoMode == TEXCO_MODE_UV) {
		texco_ = ref_ptr<ShaderInput2f>::alloc("texco0");
		texco_->setVertexData(numVertices);
	}

	const float angleStep = 2.0f * M_PI / levelOfDetail;

	GLuint vertexIndex = 0;
	for (GLuint i = 0; i <= levelOfDetail; ++i) {
		float angle = i * angleStep;
		float cosAngle = cos(angle);
		float sinAngle = sin(angle);

		Vec3f pos(cfg.discRadius * cosAngle, 0.0f, cfg.discRadius * sinAngle);
		pos = cfg.posScale * pos;

		pos_->setVertex(vertexIndex, pos);

		if (cfg.isNormalRequired) {
			nor_->setVertex(vertexIndex, Vec3f(0.0f, 1.0f, 0.0f));
		}

		if (cfg.texcoMode == TEXCO_MODE_UV) {
			Vec2f texco(pos.x/cfg.discRadius + 0.5f, pos.z/cfg.discRadius + 0.5f);
			((ShaderInput2f*)texco_.get())->setVertex(vertexIndex, texco * cfg.texcoScale);
		}

		if (cfg.isTangentRequired) {
			tan_->setVertex(vertexIndex, Vec4f(-sinAngle, 0.0f, cosAngle, 1.0f));
		}

		++vertexIndex;
	}

	// Generate indices
	indices_->setVertexData(numIndices);
	auto *indices = (GLuint *) indices_->clientDataPtr();
	GLuint index = 0;
	for (GLuint i = 0; i < levelOfDetail; ++i) {
		indices[index++] = levelOfDetail;
		indices[index++] = i + 1;
		indices[index++] = i;
	}

	// Set up the vertex attributes
	begin(ShaderInputContainer::INTERLEAVED);
	setIndices(indices_, numVertices);
	setInput(pos_);
	if (cfg.isNormalRequired) {
		setInput(nor_);
	}
	if (cfg.texcoMode != TEXCO_MODE_NONE) {
		setInput(texco_);
	}
	if (cfg.isTangentRequired) {
		setInput(tan_);
	}
	end();

	minPosition_ = -cfg.posScale;
	maxPosition_ = cfg.posScale;
}
