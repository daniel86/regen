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
			case Sphere::TEXCO_MODE_SPHERICAL:
				return out << "SPHERICAL";
			case Sphere::TEXCO_MODE_SPHERE_MAP:
				return out << "SPHERE_MAP";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, Sphere::TexcoMode &mode) {
		std::string val;
		in >> val;
		boost::to_upper(val);
		if (val == "NONE") mode = Sphere::TEXCO_MODE_NONE;
		else if (val == "UV") mode = Sphere::TEXCO_MODE_UV;
		else if (val == "SPHERICAL") mode = Sphere::TEXCO_MODE_SPHERICAL;
		else if (val == "SPHERE_MAP") mode = Sphere::TEXCO_MODE_SPHERE_MAP;
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
		  texcoMode(TEXCO_MODE_SPHERICAL),
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
	if (vAbs.x < vAbs.y && vAbs.x < vAbs.z) {
		v_ = Vec3f(0.0, -v.z, v.y);
	} else if (vAbs.y < vAbs.x && vAbs.y < vAbs.z) {
		v_ = Vec3f(-v.z, 0, v.x);
	} else {
		v_ = Vec3f(-v.y, v.x, 0);
	}
	v_.normalize();
	return v.cross(v_);
}

void Sphere::updateAttributes(const Config &cfg) {
	std::vector<TriangleFace> faces;
	{
		// setup initial level
		GLfloat a = 1.0 / sqrt(2.0) + 0.001;

		TriangleVertex level0[6];
		level0[0] = TriangleVertex(Vec3f(0.0f, 0.0f, 1.0f), 0);
		level0[1] = TriangleVertex(Vec3f(0.0f, 0.0f, -1.0f), 1);
		level0[2] = TriangleVertex(Vec3f(-a, -a, 0.0f), 2);
		level0[3] = TriangleVertex(Vec3f(a, -a, 0.0f), 3);
		level0[4] = TriangleVertex(Vec3f(a, a, 0.0f), 4);
		level0[5] = TriangleVertex(Vec3f(-a, a, 0.0f), 5);

		std::vector<TriangleFace> facesLevel0(8);
		facesLevel0[0] = TriangleFace(level0[0], level0[3], level0[4]);
		facesLevel0[1] = TriangleFace(level0[0], level0[4], level0[5]);
		facesLevel0[2] = TriangleFace(level0[0], level0[5], level0[2]);
		facesLevel0[3] = TriangleFace(level0[0], level0[2], level0[3]);
		facesLevel0[4] = TriangleFace(level0[1], level0[4], level0[3]);
		facesLevel0[5] = TriangleFace(level0[1], level0[5], level0[4]);
		facesLevel0[6] = TriangleFace(level0[1], level0[2], level0[5]);
		facesLevel0[7] = TriangleFace(level0[1], level0[3], level0[2]);
		faces = tessellate(cfg.levelOfDetail, facesLevel0);
	}

	// Find out number of triangle faces
	GLuint faceCounter = 0;
	if (cfg.isHalfSphere) {
		for (GLuint faceIndex = 0; faceIndex < faces.size(); ++faceIndex) {
			TriangleFace &face = faces[faceIndex];
			auto *vertices = (TriangleVertex *) &face;
			if (isOnPosYSphereEntirely(vertices)) continue;
			for (GLuint i = 0; i < 3; ++i) {
				if (vertices[i].p.y > 0) vertices[i].p.y = 0.0;
			}
			faceCounter += 1;
		}
	} else {
		faceCounter = faces.size();
	}
	// Allocate RAM for indices
	GLuint numIndices = faceCounter * 3;
	ref_ptr<ShaderInput1ui> indices = ref_ptr<ShaderInput1ui>::alloc("i");
	indices->setVertexData(numIndices);
	auto *indicesPtr = (GLuint *) indices->clientDataPtr();

	// Set index data and compute vertex count
	std::map<GLuint, GLint> indexMap;
	GLuint currIndex = 0;
	for (GLuint faceIndex = 0; faceIndex < faces.size(); ++faceIndex) {
		TriangleFace &face = faces[faceIndex];
		auto *vertices = (TriangleVertex *) &face;
		if (cfg.isHalfSphere && isOnPosYSpherePartially(vertices)) continue;

		for (GLuint i = 0; i < 3; ++i) {
			TriangleVertex &vertex = vertices[i];
			// Find vertex index
			if (indexMap.count(vertex.i) == 0) {
				indexMap[vertex.i] = currIndex;
				currIndex += 1;
			}
			// Add to indices attribute
			*indicesPtr = indexMap[vertex.i];
			indicesPtr += 1;
		}
		faceCounter += 1;
	}
	// Number of vertices is equal to number of distinctive indices.
	GLuint numVertices = indexMap.size();

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
	if (texcoMode == TEXCO_MODE_SPHERE_MAP) {
		texco_ = ref_ptr<ShaderInput2f>::alloc("texco0");
		texco_->setVertexData(numVertices);
	} else if (texcoMode == TEXCO_MODE_UV || texcoMode == TEXCO_MODE_SPHERICAL) {
		texco_ = ref_ptr<ShaderInput2f>::alloc("texco0");
		texco_->setVertexData(numVertices);
	}

	for (GLuint faceIndex = 0; faceIndex < faces.size(); ++faceIndex) {
		TriangleFace &face = faces[faceIndex];
		auto *vertices = (TriangleVertex *) &face;
		if (cfg.isHalfSphere && isOnPosYSpherePartially(vertices)) continue;

		// Compute uv-factors for this face
		GLfloat s1, s2, s3, t1, t2, t3;
		if (texcoMode == TEXCO_MODE_SPHERE_MAP) {
			sphereUV(vertices[0].p, &s1, &t1);
			sphereUV(vertices[1].p, &s2, &t2);
			if (s2 < 0.75 && s1 > 0.75) {
				s2 += 1.0;
			} else if (s2 > 0.75 && s1 < 0.75) {
				s2 -= 1.0;
			}

			sphereUV(vertices[2].p, &s3, &t3);
			if (s3 < 0.75 && s2 > 0.75) {
				s3 += 1.0;
			} else if (s3 > 0.75 && s2 < 0.75) {
				s3 -= 1.0;
			}
		}

		// Compute index and attribute values
		for (GLuint i = 0; i < 3; ++i) {
			TriangleVertex &vertex = vertices[i];
			GLint vertexIndex = indexMap[vertex.i];
			if (vertexIndex == -1) continue;
			indexMap[vertex.i] = -1;

			vertex.p.normalize();
			pos_->setVertex(vertexIndex, cfg.posScale * vertex.p * 0.5);
			if (cfg.isNormalRequired) {
				nor_->setVertex(vertexIndex, vertex.p);
			}
			if (cfg.isTangentRequired) {
				Vec3f t = computeSphereTangent(pos_->getVertex(vertexIndex));
				tan_->setVertex(vertexIndex, Vec4f(t.x, t.y, t.z, 1.0));
			}
			if (texcoMode == TEXCO_MODE_SPHERICAL) {
				auto *texco = (Vec2f *) texco_->clientData();
				texco[vertexIndex].x = (0.5 + (atan2(vertex.p.z, vertex.p.x) / (2.0 * M_PI)));
				texco[vertexIndex].y = (0.5 - (asin(vertex.p.y) / M_PI));
				// Ensure UV coordinates wrap around correctly
				if (texco[vertexIndex].x < 0.0f) texco[vertexIndex].x += 1.0f;
				if (texco[vertexIndex].x >= 1.0f) texco[vertexIndex].x -= 1.0f;
			} else if (texcoMode == TEXCO_MODE_UV) {
				auto *texco = (Vec2f *) texco_->clientData();
				auto *pos = (Vec3f *) pos_->clientData();
				texco[vertexIndex] = Vec2f(
						0.5f + pos[vertexIndex].x / cfg.posScale.x,
						0.5f + pos[vertexIndex].y / cfg.posScale.y
				);
			} else if (texcoMode == TEXCO_MODE_SPHERE_MAP) {
				auto *texco = (Vec2f *) texco_->clientData();
				if (i == 0) texco[vertexIndex] = Vec2f(s1, t1);
				else if (i == 1) texco[vertexIndex] = Vec2f(s2, t2);
				else if (i == 2) texco[vertexIndex] = Vec2f(s3, t3);
			}
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
