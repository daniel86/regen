#include "../lod/tessellation.h"
#include "sphere.h"

using namespace regen;

namespace regen {
	std::ostream &operator<<(std::ostream &out, const Sphere::TexcoMode &mode) {
		switch (mode) {
			case Sphere::TEXCO_MODE_NONE:
				return out << "NONE";
			case Sphere::TEXCO_MODE_UV:
				return out << "UV";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, Sphere::TexcoMode &mode) {
		std::string val;
		in >> val;
		boost::to_upper(val);
		if (val == "NONE") mode = Sphere::TEXCO_MODE_NONE;
		else if (val == "UV") mode = Sphere::TEXCO_MODE_UV;
		else {
			REGEN_WARN("Unknown sphere texco mode '" << val << "'. Using NONE texco.");
			mode = Sphere::TEXCO_MODE_NONE;
		}
		return in;
	}
}

Sphere::Sphere(const Config &cfg)
		: Mesh(GL_TRIANGLES, cfg.updateHint) {
	pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	nor_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	texco_ = ref_ptr<ShaderInput2f>::alloc("texco0");
	tan_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);
	radius_ = 0.5f * cfg.posScale.max();
	setBufferMapMode(cfg.mapMode);
	setClientAccessMode(cfg.accessMode);
	updateAttributes(cfg);
}

Sphere::Config::Config()
		: posScale(Vec3f::one()),
		  texcoScale(Vec2f::one()),
		  levelOfDetails({4}),
		  texcoMode(TEXCO_MODE_UV),
		  isNormalRequired(GL_TRUE),
		  isTangentRequired(GL_FALSE),
		  isHalfSphere(GL_FALSE) {
}

static Vec3f computeSphereTangent(const Vec3f &v) {
	Vec3f vAbs = Vec3f(abs(v.x), abs(v.y), abs(v.z));
	Vec3f v_;
	if (1.0f - v.z < std::numeric_limits<float>::epsilon()) {
		// there is a singularity at the back pole
		v_ = Vec3f(1.0, 0.0, 0.0);
	} else if (vAbs.x < vAbs.y && vAbs.x < vAbs.z) {
		v_ = Vec3f(0.0, -v.z, v.y);
	} else if (vAbs.y < vAbs.x && vAbs.y < vAbs.z) {
		v_ = Vec3f(-v.z, 0, v.x);
	} else {
		v_ = Vec3f(-v.y, v.x, 0);
	}
	v_.normalize();
	return v.cross(v_);
}

void pushVertex(
		GLuint vertexIndex,
		GLdouble u,
		GLdouble v,
		const Sphere::Config &cfg,
		Vec3f *pos,
		Vec3f *nor,
		Vec4f *tan,
		Vec2f *texco) {
	GLdouble r = std::sin(M_PI * v);
	pos[vertexIndex] = Vec3f(
			static_cast<float>(r * std::cos(2.0 * M_PI * u)),
			static_cast<float>(r * std::sin(2.0 * M_PI * u)),
			static_cast<float>(std::cos(M_PI * v))
	);
	if (cfg.isNormalRequired) {
		nor[vertexIndex] = pos[vertexIndex];
	}
	if (cfg.isTangentRequired) {
		Vec3f t = computeSphereTangent(pos[vertexIndex]);
		tan[vertexIndex] = Vec4f(t.x, t.y, t.z, 1.0);
	}
	if (texco) {
		texco[vertexIndex] = Vec2f(
				static_cast<float>(u) * cfg.texcoScale.x,
				static_cast<float>(v) * cfg.texcoScale.y);
	}
	pos[vertexIndex] *= cfg.posScale * 0.5;
}

void Sphere::generateLODLevel(const Config &cfg,
							  GLuint lodLevel,
							  GLuint vertexOffset,
							  GLuint indexOffset) {
	// map client data for writing
	auto v_pos = (Vec3f*) pos_->clientBuffer()->clientData(0);
	auto v_nor = (cfg.isNormalRequired ?
				  (Vec3f*) nor_->clientBuffer()->clientData(0) : nullptr);
	auto v_tan = (cfg.isTangentRequired ?
				  (Vec4f*) tan_->clientBuffer()->clientData(0) : nullptr);
	auto v_texco = (texco_.get() ?
					(Vec2f*) texco_->clientBuffer()->clientData(0) : nullptr);
	// map untyped index data (we switch type depending on the number of vertices)
	auto indices = (byte*)indices_->clientBuffer()->clientData(0);
	auto indexType = indices_->baseType();

	GLdouble stepSizeInv = 1.0 / (GLdouble) lodLevel;
	GLuint vertexIndex = vertexOffset, faceIndex = indexOffset / 6;

	for (GLuint i = 0; i < lodLevel; i++) {
		for (GLuint j = 0; j < lodLevel; j++) {
			GLdouble u0 = (GLdouble) i * stepSizeInv;
			GLdouble u1 = (GLdouble) (i + 1) * stepSizeInv;
			GLdouble v0 = (GLdouble) j * stepSizeInv;
			GLdouble v1 = (GLdouble) (j + 1) * stepSizeInv;

			if (cfg.isHalfSphere && u0 < 0.5) continue;

			// create two triangles for each quad
			GLuint index = (faceIndex++) * 6;
			setIndexValue(indices, indexType, index + 0, vertexIndex + 0);
			setIndexValue(indices, indexType, index + 1, vertexIndex + 1);
			setIndexValue(indices, indexType, index + 2, vertexIndex + 2);
			setIndexValue(indices, indexType, index + 3, vertexIndex + 2);
			setIndexValue(indices, indexType, index + 4, vertexIndex + 1);
			setIndexValue(indices, indexType, index + 5, vertexIndex + 3);

			// they are made of 4 vertices
			pushVertex(vertexIndex++, u1, v1, cfg, v_pos, v_nor, v_tan, v_texco);
			pushVertex(vertexIndex++, u1, v0, cfg, v_pos, v_nor, v_tan, v_texco);
			pushVertex(vertexIndex++, u0, v1, cfg, v_pos, v_nor, v_tan, v_texco);
			pushVertex(vertexIndex++, u0, v0, cfg, v_pos, v_nor, v_tan, v_texco);
		}
	}
}

void Sphere::updateAttributes(const Config &cfg) {
	std::vector<GLuint> LODs;
	GLuint vertexOffset = 0;
	GLuint indexOffset = 0;
	for (auto &lod: cfg.levelOfDetails) {
		GLuint lodLevel = 4 + lod * lod;
		LODs.push_back(lodLevel);
		GLuint numFaces;
		if (cfg.isHalfSphere) {
			numFaces = lodLevel * lodLevel;
		} else {
			numFaces = 2 * lodLevel * lodLevel;
		}
		auto &x = meshLODs_.emplace_back();
		x.d->numVertices = numFaces * 2;
		x.d->numIndices = numFaces * 3;
		x.d->vertexOffset = vertexOffset;
		x.d->indexOffset = indexOffset;
		vertexOffset += x.d->numVertices;
		indexOffset += x.d->numIndices;
	}
	GLuint numVertices = vertexOffset;
	GLuint numIndices = indexOffset;

	// allocate attributes
	pos_->setVertexData(numVertices);
	if (cfg.isNormalRequired) {
		nor_->setVertexData(numVertices);
	}
	if (cfg.isTangentRequired) {
		tan_->setVertexData(numVertices);
	}
	TexcoMode texcoMode = cfg.texcoMode;
	if (cfg.isTangentRequired && cfg.texcoMode == TEXCO_MODE_NONE) {
		texcoMode = TEXCO_MODE_UV;
	}
	if (texcoMode == TEXCO_MODE_UV) {
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

	auto indexRef = setIndices(indices_, numVertices);
	setInput(pos_);
	if (cfg.isNormalRequired) {
		setInput(nor_);
	}
	if (cfg.isTangentRequired) {
		setInput(tan_);
	}
	if (texcoMode != TEXCO_MODE_NONE) {
		setInput(texco_);
	}
	updateVertexData();

	for (auto &x: meshLODs_) {
		// add the index buffer offset (in number of bytes)
		x.d->indexOffset = indexRef->address() + x.d->indexOffset * indices_->dataTypeBytes();
	}
	activateLOD(0);
	minPosition_ = -cfg.posScale * 0.5f;
	maxPosition_ = cfg.posScale * 0.5f;
}

///////////
///////////

SphereSprite::Config::Config()
		: radius(nullptr),
		  position(nullptr),
		  sphereCount(0) {
}

SphereSprite::SphereSprite(const Config &cfg)
		: Mesh(GL_POINTS, cfg.updateHint),
		  HasShader("regen.models.sprite-sphere") {
	setBufferMapMode(cfg.mapMode);
	setClientAccessMode(cfg.accessMode);
	updateAttributes(cfg);
	joinStates(shaderState());
}

void SphereSprite::updateAttributes(const Config &cfg) {
	ref_ptr<ShaderInput1f> radiusIn = ref_ptr<ShaderInput1f>::alloc("sphereRadius");
	radiusIn->setVertexData(cfg.sphereCount);
	auto mappedRadius = radiusIn->mapClientData<float>(BUFFER_GPU_WRITE);

	ref_ptr<ShaderInput3f> positionIn = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	positionIn->setVertexData(cfg.sphereCount);
	auto mappedPosition = positionIn->mapClientData<Vec3f>(BUFFER_GPU_WRITE);

	minPosition_ = Vec3f::create(999999.0f);
	maxPosition_ = Vec3f::create(-999999.0f);
	Vec3f v;
	for (GLuint i = 0; i < cfg.sphereCount; ++i) {
		mappedRadius.w[i] = cfg.radius[i];
		mappedPosition.w[i] = cfg.position[i];

		v = cfg.position[i] - Vec3f::create(cfg.radius[i]);
		minPosition_.setMin(v);
		v = cfg.position[i] + Vec3f::create(cfg.radius[i]);
		maxPosition_.setMax(v);
	}

	mappedRadius.unmap();
	mappedPosition.unmap();

	setInput(radiusIn);
	setInput(positionIn);
	updateVertexData();
}
