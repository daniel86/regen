
#include "../lod/tessellation.h"
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
		cfg.usage = BUFFER_USAGE_STATIC_DRAW;
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
		: levelOfDetails({0}),
		  posScale(Vec3f(1.0f)),
		  rotation(Vec3f(0.0f)),
		  texcoScale(Vec2f(1.0f)),
		  texcoMode(TEXCO_MODE_UV),
		  isNormalRequired(GL_TRUE),
		  isTangentRequired(GL_FALSE),
		  usage(BUFFER_USAGE_DYNAMIC_DRAW),
		  discRadius(1.0f) {
}

void Disc::generateLODLevel(const Config &cfg,
							GLuint lodLevel,
							GLuint vertexOffset,
							GLuint indexOffset) {
	const float angleStep = 2.0f * M_PI / lodLevel;

	auto indices = indices_->mapClientData<GLuint>(ShaderData::WRITE);
	auto v_pos = pos_->mapClientData<Vec3f>(ShaderData::WRITE);
	auto v_nor = (cfg.isNormalRequired ?
		nor_->mapClientData<Vec3f>(ShaderData::WRITE) :
		ShaderData_rw<Vec3f>::nullData());
	auto v_tan = (cfg.isTangentRequired ?
		tan_->mapClientData<Vec4f>(ShaderData::WRITE) :
		ShaderData_rw<Vec4f>::nullData());
	auto v_texco = (cfg.texcoMode == TEXCO_MODE_UV ?
		texco_->mapClientData<Vec2f>(ShaderData::WRITE) :
		ShaderData_rw<Vec2f>::nullData());

	GLuint vertexIndex = vertexOffset;
	for (GLuint i = 0; i <= lodLevel; ++i) {
		float angle = i * angleStep;
		float cosAngle = cos(angle);
		float sinAngle = sin(angle);

		Vec3f pos(cfg.discRadius * cosAngle, 0.0f, cfg.discRadius * sinAngle);
		pos = cfg.posScale * pos;
		v_pos.w[vertexIndex] = pos;

		if (cfg.isNormalRequired) {
			v_nor.w[vertexIndex] = Vec3f(0.0f, 1.0f, 0.0f);
		}

		if (cfg.texcoMode == TEXCO_MODE_UV) {
			Vec2f texco(pos.x / cfg.discRadius + 0.5f, pos.z / cfg.discRadius + 0.5f);
			v_texco.w[vertexIndex] = texco * cfg.texcoScale;
		}

		if (cfg.isTangentRequired) {
			v_tan.w[vertexIndex] = Vec4f(-sinAngle, 0.0f, cosAngle, 1.0f);
		}

		++vertexIndex;
	}

	// Generate indices
	GLuint index = indexOffset;
	for (GLuint i = 0; i < lodLevel; ++i) {
		indices.w[index++] = vertexOffset + lodLevel;
		indices.w[index++] = vertexOffset + i + 1;
		indices.w[index++] = vertexOffset + i;
	}
}

void Disc::updateAttributes(const Config &cfg) {
	std::vector<GLuint> LODs;
	GLuint vertexOffset = 0;
	GLuint indexOffset = 0;
	for (auto &lod: cfg.levelOfDetails) {
		GLuint lodLevel = 4u * pow(2u, lod);
		LODs.push_back(lodLevel);
		auto &x = meshLODs_.emplace_back();
		x.numVertices = lodLevel + 1;
		x.numIndices = lodLevel * 3;
		x.vertexOffset = vertexOffset;
		x.indexOffset = indexOffset;
		vertexOffset += x.numVertices;
		indexOffset += x.numIndices;
	}
	GLuint numVertices = vertexOffset;
	GLuint numIndices = indexOffset;

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
	indices_->setVertexData(numIndices);

	for (auto i = 0u; i < LODs.size(); ++i) {
		generateLODLevel(cfg,
						 LODs[i],
						 meshLODs_[i].vertexOffset,
						 meshLODs_[i].indexOffset);
	}

	// Set up the vertex attributes
	begin(InputContainer::INTERLEAVED);
	auto indexRef = setIndices(indices_, numVertices);
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

	for (auto &x: meshLODs_) {
		// add the index buffer offset (in number of bytes)
		x.indexOffset = indexRef->address() + x.indexOffset * sizeof(GLuint);
	}
	activateLOD(0);

	maxPosition_ = Vec3f(0.0);
	maxPosition_.x += cfg.discRadius;
	maxPosition_.z += cfg.discRadius;
	maxPosition_ *= cfg.posScale;

	minPosition_ = Vec3f(0.0);
	minPosition_.x -= cfg.discRadius;
	minPosition_.z -= cfg.discRadius;
	minPosition_ *= cfg.posScale;
}
