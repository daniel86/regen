
#include "../lod/tessellation.h"
#include "torus.h"

using namespace regen;

namespace regen {
	std::ostream &operator<<(std::ostream &out, const Torus::TexcoMode &mode) {
		switch (mode) {
			case Torus::TEXCO_MODE_NONE:
				return out << "NONE";
			case Torus::TEXCO_MODE_UV:
				return out << "UV";
			case Torus::TEXCO_MODE_CUBE_MAP:
				return out << "CUBE_MAP";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, Torus::TexcoMode &mode) {
		std::string val;
		in >> val;
		boost::to_upper(val);
		if (val == "NONE") mode = Torus::TEXCO_MODE_NONE;
		else if (val == "UV") mode = Torus::TEXCO_MODE_UV;
		else if (val == "CUBE_MAP") mode = Torus::TEXCO_MODE_CUBE_MAP;
		else {
			REGEN_WARN("Unknown box texco mode '" << val << "'. Using NONE texco.");
			mode = Torus::TEXCO_MODE_NONE;
		}
		return in;
	}
}

ref_ptr<Torus> Torus::getUnitTorus() {
	static ref_ptr<Torus> mesh;
	if (mesh.get() == nullptr) {
		Config cfg;
		cfg.posScale = Vec3f(1.0f);
		cfg.rotation = Vec3f(0.0, 0.0f, 0.0f);
		cfg.texcoMode = TEXCO_MODE_NONE;
		cfg.isNormalRequired = GL_FALSE;
		cfg.isTangentRequired = GL_FALSE;
		cfg.usage = BUFFER_USAGE_STATIC_DRAW;
		mesh = ref_ptr<Torus>::alloc(cfg);
		return mesh;
	} else {
		return ref_ptr<Torus>::alloc(mesh);
	}
}

Torus::Torus(const Config &cfg)
		: Mesh(GL_TRIANGLES, cfg.usage) {
	pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	nor_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	tan_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);
	indices_ = ref_ptr<ShaderInput1ui>::alloc("i");
	updateAttributes(cfg);
}

Torus::Torus(const ref_ptr<Torus> &other)
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

Torus::Config::Config()
		: levelOfDetails({0}),
		  posScale(Vec3f(1.0f)),
		  rotation(Vec3f(0.0f)),
		  texcoScale(Vec2f(1.0f)),
		  texcoMode(TEXCO_MODE_UV),
		  isNormalRequired(GL_TRUE),
		  isTangentRequired(GL_FALSE),
		  usage(BUFFER_USAGE_DYNAMIC_DRAW),
		  ringRadius(1.0f),
		  tubeRadius(0.5f) {
}

void Torus::generateLODLevel(const Config &cfg,
							 GLuint lodLevel,
							 GLuint vertexOffset,
							 GLuint indexOffset) {
	// map client data for writing
	auto indices = indices_->mapClientData<GLuint>(ShaderData::WRITE);
	auto v_pos = pos_->mapClientData<Vec3f>(ShaderData::WRITE);
	auto v_nor = (cfg.isNormalRequired ?
		nor_->mapClientData<Vec3f>(ShaderData::WRITE) :
		ShaderData_rw<Vec3f>::nullData());
	auto v_tan = (cfg.isTangentRequired ?
		tan_->mapClientData<Vec4f>(ShaderData::WRITE) :
		ShaderData_rw<Vec4f>::nullData());
	auto v_texco = (texco_.get() ?
		texco_->mapClientData<float>(ShaderData::WRITE) :
		ShaderData_rw<float>::nullData());

	GLuint vertexIndex = vertexOffset;
	const float ringStep = 2.0f * M_PI / lodLevel;
	const float tubeStep = 2.0f * M_PI / lodLevel;

	for (GLuint i = 0; i <= lodLevel; ++i) {
		float theta = i * ringStep;
		float cosTheta = cos(theta);
		float sinTheta = sin(theta);

		for (GLuint j = 0; j <= lodLevel; ++j) {
			float phi = j * tubeStep;
			float cosPhi = cos(phi);
			float sinPhi = sin(phi);

			Vec3f pos(
					(cfg.ringRadius + cfg.tubeRadius * cosPhi) * cosTheta,
					cfg.tubeRadius * sinPhi,
					(cfg.ringRadius + cfg.tubeRadius * cosPhi) * sinTheta
			);

			pos = cfg.posScale * pos;
			v_pos.w[vertexIndex] = pos;

			if (cfg.isNormalRequired) {
				v_nor.w[vertexIndex] = Vec3f(
						cosPhi * cosTheta,
						sinPhi,
						cosPhi * sinTheta);
			}

			if (cfg.texcoMode == TEXCO_MODE_UV) {
				Vec2f texco((float) i / lodLevel, (float) j / lodLevel);
				((Vec2f*)v_texco.w)[vertexIndex] = texco * cfg.texcoScale;
			} else if (cfg.texcoMode == TEXCO_MODE_CUBE_MAP) {
				Vec3f texco = pos;
				texco.normalize();
				((Vec3f*)v_texco.w)[vertexIndex] = texco;
			}

			if (cfg.isTangentRequired) {
				v_tan.w[vertexIndex] = Vec4f(
						-sinTheta,
						0.0f,
						cosTheta, 1.0f);
			}

			++vertexIndex;
		}
	}

	GLuint iOffset = indexOffset;
	for (GLuint i = 0; i < lodLevel; ++i) {
		for (GLuint j = 0; j < lodLevel; ++j) {
			GLuint first = (i * (lodLevel + 1)) + j;
			GLuint second = vertexOffset + first + lodLevel + 1;
			first += vertexOffset;

			indices.w[iOffset++] = first;
			indices.w[iOffset++] = first + 1;
			indices.w[iOffset++] = second;

			indices.w[iOffset++] = second;
			indices.w[iOffset++] = first + 1;
			indices.w[iOffset++] = second + 1;
		}
	}
}

void Torus::updateAttributes(const Config &cfg) {
	// Generate multiple LOD levels of the torus
	std::vector<GLuint> LODs;
	GLuint vertexOffset = 0;
	GLuint indexOffset = 0;
	for (auto &lod: cfg.levelOfDetails) {
		GLuint lodLevel = 4u * pow(2u, lod);
		LODs.push_back(lodLevel);
		auto &x = meshLODs_.emplace_back();
		x.numVertices = (lodLevel + 1) * (lodLevel + 1);
		x.numIndices = lodLevel * lodLevel * 6;
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
	} else if (cfg.texcoMode == TEXCO_MODE_CUBE_MAP) {
		texco_ = ref_ptr<ShaderInput3f>::alloc("texco0");
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

	minPosition_ = Vec3f(-cfg.tubeRadius);
	minPosition_.x -= cfg.ringRadius;
	minPosition_.z -= cfg.ringRadius;
	minPosition_ *= cfg.posScale;

	maxPosition_ = Vec3f(cfg.tubeRadius);
	maxPosition_.x += cfg.ringRadius;
	maxPosition_.z += cfg.ringRadius;
	maxPosition_ *= cfg.posScale;
}
