
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
		cfg.posScale = Vec3f::one();
		cfg.rotation = Vec3f(0.0, 0.0f, 0.0f);
		cfg.texcoMode = TEXCO_MODE_NONE;
		cfg.isNormalRequired = GL_FALSE;
		cfg.isTangentRequired = GL_FALSE;
		cfg.discRadius = 1.0f;
		mesh = ref_ptr<Disc>::alloc(cfg);
		return mesh;
	} else {
		return ref_ptr<Disc>::alloc(mesh);
	}
}

Disc::Disc(const Config &cfg)
		: Mesh(GL_TRIANGLES, cfg.updateHint) {
	pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	nor_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	tan_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);
	setBufferMapMode(cfg.mapMode);
	setClientAccessMode(cfg.accessMode);
	updateAttributes(cfg);
}

Disc::Disc(const ref_ptr<Disc> &other)
		: Mesh(other) {
	pos_ = ref_ptr<ShaderInput3f>::dynamicCast(getInput(ATTRIBUTE_NAME_POS));
	nor_ = ref_ptr<ShaderInput3f>::dynamicCast(getInput(ATTRIBUTE_NAME_NOR));
	tan_ = ref_ptr<ShaderInput4f>::dynamicCast(getInput(ATTRIBUTE_NAME_TAN));
	indices_ = getInput("i");
}

Disc::Config::Config()
		: levelOfDetails({0}),
		  posScale(Vec3f::one()),
		  rotation(Vec3f::zero()),
		  texcoScale(Vec2f::one()),
		  texcoMode(TEXCO_MODE_UV),
		  isNormalRequired(GL_TRUE),
		  isTangentRequired(GL_FALSE),
		  discRadius(1.0f) {
}

void Disc::generateLODLevel(const Config &cfg,
							GLuint lodLevel,
							GLuint vertexOffset,
							GLuint indexOffset) {
	const float angleStep = 2.0f * M_PI / lodLevel;

	auto v_pos = (Vec3f*) pos_->clientBuffer()->clientData(0);
	auto v_nor = (cfg.isNormalRequired ?
				  (Vec3f*) nor_->clientBuffer()->clientData(0) : nullptr);
	auto v_tan = (cfg.isTangentRequired ?
				  (Vec4f*) tan_->clientBuffer()->clientData(0) : nullptr);
	auto v_texco = (cfg.texcoMode == TEXCO_MODE_UV ?
					(Vec2f*) texco_->clientBuffer()->clientData(0) : nullptr);
	auto indices = (byte*)indices_->clientBuffer()->clientData(0);
	auto indexType = indices_->baseType();

	GLuint vertexIndex = vertexOffset;
	for (GLuint i = 0; i <= lodLevel; ++i) {
		float angle = i * angleStep;
		float cosAngle = cos(angle);
		float sinAngle = sin(angle);

		Vec3f pos(cfg.discRadius * cosAngle, 0.0f, cfg.discRadius * sinAngle);
		pos = cfg.posScale * pos;
		v_pos[vertexIndex] = pos;

		if (cfg.isNormalRequired) {
			v_nor[vertexIndex] = Vec3f(0.0f, 1.0f, 0.0f);
		}

		if (cfg.texcoMode == TEXCO_MODE_UV) {
			Vec2f texco(pos.x / cfg.discRadius + 0.5f, pos.z / cfg.discRadius + 0.5f);
			v_texco[vertexIndex] = texco * cfg.texcoScale;
		}

		if (cfg.isTangentRequired) {
			v_tan[vertexIndex] = Vec4f(-sinAngle, 0.0f, cosAngle, 1.0f);
		}

		++vertexIndex;
	}

	// Generate indices
	GLuint index = indexOffset;
	for (GLuint i = 0; i < lodLevel; ++i) {
		setIndexValue(indices, indexType, index++, vertexOffset + lodLevel);
		setIndexValue(indices, indexType, index++, vertexOffset + i + 1);
		setIndexValue(indices, indexType, index++, vertexOffset + i);
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
		x.d->numVertices = lodLevel + 1;
		x.d->numIndices = lodLevel * 3;
		x.d->vertexOffset = vertexOffset;
		x.d->indexOffset = indexOffset;
		vertexOffset += x.d->numVertices;
		indexOffset += x.d->numIndices;
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
	indices_ = createIndexInput(numIndices, numVertices);

	for (auto i = 0u; i < LODs.size(); ++i) {
		generateLODLevel(cfg,
						 LODs[i],
						 meshLODs_[i].d->vertexOffset,
						 meshLODs_[i].d->indexOffset);
	}

	// Set up the vertex attributes
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
	updateVertexData();

	for (auto &x: meshLODs_) {
		// add the index buffer offset (in number of bytes)
		x.d->indexOffset = indexRef->address() + x.d->indexOffset * indices_->dataTypeBytes();
	}
	activateLOD(0);

	maxPosition_ = Vec3f::zero();
	maxPosition_.x += cfg.discRadius;
	maxPosition_.z += cfg.discRadius;
	maxPosition_ *= cfg.posScale;

	minPosition_ = Vec3f::zero();
	minPosition_.x -= cfg.discRadius;
	minPosition_.z -= cfg.discRadius;
	minPosition_ *= cfg.posScale;
}
