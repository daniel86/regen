/*
 * Cube.cpp
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#include "tessellation.h"
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

static void sphereUV(const Vec3f &p, GLfloat *s, GLfloat *t) {
	*s = atan2(p.x, p.z) / (2. * M_PI) + 0.5;
	*t = asin(p.y) / M_PI + 0.5;
}


Sphere::Sphere(const Config &cfg)
		: Mesh(GL_TRIANGLES, cfg.usage) {
	pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	nor_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	texco_ = ref_ptr<ShaderInput2f>::alloc("texco0");
	tan_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);

	updateAttributes(cfg);
}

Sphere::Config::Config()
		: posScale(Vec3f(1.0f)),
		  texcoScale(Vec2f(1.0f)),
		  levelOfDetail(4),
		  texcoMode(TEXCO_MODE_UV),
		  isNormalRequired(GL_TRUE),
		  isTangentRequired(GL_FALSE),
		  isHalfSphere(GL_FALSE),
		  usage(VBO::USAGE_DYNAMIC) {
}

static GLboolean isOnPosYSpherePartially(TriangleVertex *triangle) {
	for (GLuint i = 0; i < 3; ++i) {
		if (triangle[i].p.y > 0) return GL_TRUE;
	}
	return GL_FALSE;
}

static GLboolean isOnPosYSphereEntirely(TriangleVertex *triangle) {
	for (GLuint i = 0; i < 3; ++i) {
		if (triangle[i].p.y < 0) return GL_FALSE;
	}
	return GL_TRUE;
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

void Sphere::pushVertex(GLuint vertexIndex, GLdouble u, GLdouble v, const Config &cfg) {
	auto positions = ((Vec3f*) pos_->clientData());

	GLdouble r = std::sin(M_PI * v);
	positions[vertexIndex] = Vec3f(
		static_cast<float>(r * std::cos(2.0 * M_PI * u)),
		static_cast<float>(r * std::sin(2.0 * M_PI * u)),
		static_cast<float>(std::cos(M_PI * v))
	);
	if (cfg.isNormalRequired) {
		nor_->setVertex(vertexIndex, positions[vertexIndex]);
	}
	if (cfg.isTangentRequired) {
		Vec3f t = computeSphereTangent(positions[vertexIndex]);
		tan_->setVertex(vertexIndex, Vec4f(t.x, t.y, t.z, 1.0));
	}
	if (texco_.get()) {
		auto texco = (Vec2f *) texco_->clientData();
		texco[vertexIndex] = Vec2f(
				static_cast<float>(u) * cfg.texcoScale.x,
				static_cast<float>(v) * cfg.texcoScale.y);
	}
	positions[vertexIndex] *= cfg.posScale * 0.5;
}

void Sphere::updateAttributes(const Config &cfg) {
	GLuint stepSize = 4 + cfg.levelOfDetail * cfg.levelOfDetail;
	GLuint numFaces;
	if(cfg.isHalfSphere) {
		numFaces = stepSize * stepSize;
	} else {
		numFaces = 2 * stepSize * stepSize;
	}
	GLuint numVertices = numFaces * 2;

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

	// Allocate RAM for indices
	GLuint numIndices = numFaces * 3;
	ref_ptr<ShaderInput1ui> indices = ref_ptr<ShaderInput1ui>::alloc("i");
	indices->setVertexData(numIndices);
	auto *indicesPtr = (GLuint *) indices->clientDataPtr();

	GLdouble stepSizeInv = 1.0 / (GLdouble) stepSize;
	GLuint vertexIndex = 0, faceIndex = 0;
	for (GLuint i=0; i<stepSize; i++) {
		for (GLuint j=0; j<stepSize; j++) {
            GLdouble u0 = (GLdouble)i * stepSizeInv;
            GLdouble u1 = (GLdouble)(i + 1) * stepSizeInv;
            GLdouble v0 = (GLdouble)j * stepSizeInv;
            GLdouble v1 = (GLdouble)(j + 1) * stepSizeInv;

            if (cfg.isHalfSphere && u0 < 0.5) continue;

            // create two triangles for each quad
            GLuint index = (faceIndex++) * 6;
            indicesPtr[index + 0] = vertexIndex + 0;
            indicesPtr[index + 1] = vertexIndex + 1;
            indicesPtr[index + 2] = vertexIndex + 2;
            indicesPtr[index + 3] = vertexIndex + 2;
            indicesPtr[index + 4] = vertexIndex + 1;
            indicesPtr[index + 5] = vertexIndex + 3;

            // they are made of 4 vertices
            pushVertex(vertexIndex++, u1, v1, cfg);
            pushVertex(vertexIndex++, u1, v0, cfg);
            pushVertex(vertexIndex++, u0, v1, cfg);
            pushVertex(vertexIndex++, u0, v0, cfg);
		}
	}

	begin(ShaderInputContainer::INTERLEAVED);
	setIndices(indices, numVertices);
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
	end();

	minPosition_ = -cfg.posScale;
	maxPosition_ = cfg.posScale;
}

///////////
///////////

SphereSprite::Config::Config()
		: radius(nullptr),
		  position(nullptr),
		  sphereCount(0),
		  usage(VBO::USAGE_DYNAMIC) {
}

SphereSprite::SphereSprite(const Config &cfg)
		: Mesh(GL_POINTS, cfg.usage), HasShader("regen.models.sprite-sphere") {
	updateAttributes(cfg);
	joinStates(shaderState());
}

void SphereSprite::updateAttributes(const Config &cfg) {
	ref_ptr<ShaderInput1f> radiusIn = ref_ptr<ShaderInput1f>::alloc("sphereRadius");
	radiusIn->setVertexData(cfg.sphereCount);

	ref_ptr<ShaderInput3f> positionIn = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	positionIn->setVertexData(cfg.sphereCount);

	minPosition_ = Vec3f(999999.0f);
	maxPosition_ = Vec3f(-999999.0f);
	Vec3f v;
	for (GLuint i = 0; i < cfg.sphereCount; ++i) {
		radiusIn->setVertex(i, cfg.radius[i]);
		positionIn->setVertex(i, cfg.position[i]);

		v = cfg.position[i] - Vec3f(cfg.radius[i]);
		if (minPosition_.x > v.x) minPosition_.x = v.x;
		if (minPosition_.y > v.y) minPosition_.y = v.y;
		if (minPosition_.z > v.z) minPosition_.z = v.z;
		v = cfg.position[i] + Vec3f(cfg.radius[i]);
		if (maxPosition_.x < v.x) maxPosition_.x = v.x;
		if (maxPosition_.y < v.y) maxPosition_.y = v.y;
		if (maxPosition_.z < v.z) maxPosition_.z = v.z;
	}
	centerPosition_ = (maxPosition_ + minPosition_) * 0.5;
	maxPosition_ -= centerPosition_;
	minPosition_ -= centerPosition_;

	begin(ShaderInputContainer::INTERLEAVED);
	setInput(radiusIn);
	setInput(positionIn);
	end();
}
