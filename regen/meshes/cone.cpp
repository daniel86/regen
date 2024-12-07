/*
 * cone.cpp
 *
 *  Created on: 03.02.2013
 *      Author: daniel
 */

#include "cone.h"

using namespace regen;

Cone::Cone(GLenum primitive, VBO::Usage usage)
		: Mesh(primitive, usage) {
	pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	nor_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
}

/////////////////
/////////////////

ConeOpened::Config::Config()
		: cosAngle(0.5),
		  height(1.0f),
		  isNormalRequired(GL_TRUE),
		  levelOfDetails({1}),
		  usage(VBO::USAGE_DYNAMIC) {
}

ConeOpened::ConeOpened(const Config &cfg)
		: Cone(GL_TRIANGLE_FAN, cfg.usage) {
	updateAttributes(cfg);
}

void ConeOpened::generateLODLevel(const Config &cfg,
				GLuint lodLevel,
				GLuint vertexOffset,
				GLuint indexOffset) {
	GLfloat phi = acos(cfg.cosAngle);
	GLfloat radius = tan(phi) * cfg.height;
	GLfloat angle = 0.0f;
	GLfloat angleStep = 2.0f * M_PI / (GLfloat) lodLevel;
	GLuint i = vertexOffset;

	pos_->setVertex(i, Vec3f(0.0f));
	if (cfg.isNormalRequired) {
		nor_->setVertex(i, Vec3f(0.0f, -1.0f, 0.0f));
	}

	for (; i < lodLevel + 1; ++i) {
		angle += angleStep;
		GLfloat s = sin(angle) * radius;
		GLfloat c = cos(angle) * radius;
		pos_->setVertex(i + 1, Vec3f(c, s, cfg.height));
		if (cfg.isNormalRequired) {
			Vec3f n(c, 0.0, s);
			n.normalize();
			nor_->setVertex(i + 1, n);
		}
	}
}

void ConeOpened::updateAttributes(const Config &cfg) {
    std::vector<GLuint> LODs;
    GLuint vertexOffset = 0;
    for (auto &lod : cfg.levelOfDetails) {
    	GLuint lodLevel = 4u * pow(lod, 2);
		LODs.push_back(lodLevel);
		auto &x = meshLODs_.emplace_back();
		x.numVertices = lodLevel + 2;
		x.vertexOffset = vertexOffset;
		vertexOffset += x.numVertices;
	}
	GLuint numVertices = vertexOffset;

	pos_->setVertexData(numVertices);
	if (cfg.isNormalRequired) {
		nor_->setVertexData(numVertices);
	}

    for (auto i=0u; i<LODs.size(); ++i) {
    	generateLODLevel(cfg,
    			LODs[i],
    			meshLODs_[i].vertexOffset,
    			meshLODs_[i].indexOffset);
	}

	begin(ShaderInputContainer::INTERLEAVED);
	setInput(pos_);
	if (cfg.isNormalRequired)
		setInput(nor_);
	end();

	minPosition_ = Vec3f(-cfg.height);
	maxPosition_ = Vec3f(cfg.height);
	activateLOD(0);
}

/////////////////
/////////////////

ConeClosed::Config::Config()
		: radius(0.5),
		  height(1.0f),
		  isNormalRequired(GL_TRUE),
		  isBaseRequired(GL_TRUE),
		  levelOfDetails({1}),
		  usage(VBO::USAGE_DYNAMIC) {
}

ref_ptr<Mesh> ConeClosed::getBaseCone() {
	static ref_ptr<ConeClosed> mesh;
	if (mesh.get() == nullptr) {
		Config cfg;
		cfg.height = 1.0f;
		cfg.radius = 0.5;
		cfg.levelOfDetails = {3, 2, 1};
		cfg.isNormalRequired = GL_FALSE;
		cfg.isBaseRequired = GL_TRUE;
		cfg.usage = VBO::USAGE_STATIC;
		mesh = ref_ptr<ConeClosed>::alloc(cfg);
		return mesh;
	} else {
		return ref_ptr<Mesh>::alloc(mesh);
	}
}

ConeClosed::ConeClosed(const Config &cfg)
		: Cone(GL_TRIANGLES, cfg.usage) {
	indices_ = ref_ptr<ShaderInput1ui>::alloc("i");
	updateAttributes(cfg);
}

static void loadConeData(
		Vec3f *pos, Vec3f *nor,
		GLboolean useBase,
		GLuint subdivisions,
		GLfloat radius,
		GLfloat height) {
	GLfloat angle = 0.0f;
	GLfloat angleStep = 2.0f * M_PI / (GLfloat) subdivisions;
	GLint i = 0;

	// apex
	pos[i] = Vec3f(0.0f);
	if (nor) nor[i] = Vec3f(0.0f, 0.0f, -1.0f);
	++i;
	// base center
	if (useBase) {
		pos[i] = Vec3f(0.0f, 0.0f, height);
		if (nor) nor[i] = Vec3f(0.0f, 0.0f, 1.0f);
		++i;
	}

	GLint numVertices = subdivisions + i;
	for (; i < numVertices; ++i) {
		angle += angleStep;
		GLfloat s = sin(angle) * radius;
		GLfloat c = cos(angle) * radius;
		pos[i] = Vec3f(c, s, height);
		if (nor) {
			nor[i] = Vec3f(c, s, 0.0);
			nor[i].normalize();
		}
	}
}

void ConeClosed::generateLODLevel(
				const Config &cfg,
				GLuint lodLevel,
				GLuint vertexOffset,
				GLuint indexOffset) {
	// create cone vertex data
	auto *pos = ((Vec3f *) pos_->clientDataPtr()) + vertexOffset;
	auto *nor = cfg.isNormalRequired ? ((Vec3f *) nor_->clientDataPtr()) + vertexOffset : nullptr;
	loadConeData(pos, nor,
			cfg.isBaseRequired, lodLevel,
			cfg.radius, cfg.height);

	// create cone index data
	auto *faceIndices = (GLuint *) indices_->clientDataPtr();
	const GLuint apexIndex = vertexOffset;
	const GLuint baseCenterIndex = vertexOffset + 1;
	GLuint faceIndex = indexOffset;
	GLint vIndex = vertexOffset + cfg.isBaseRequired ? 2 : 1;
	// cone
	for (GLuint i = 0; i < lodLevel; ++i) {
		faceIndices[faceIndex++] = apexIndex;
		faceIndices[faceIndex++] = (i + 1 == lodLevel ? vIndex : vIndex + i + 1);
		faceIndices[faceIndex++] = vIndex + i;
	}
	// base
	if (cfg.isBaseRequired) {
		for (GLuint i = 0; i < lodLevel; ++i) {
			faceIndices[faceIndex++] = baseCenterIndex;
			faceIndices[faceIndex++] = vIndex + i;
			faceIndices[faceIndex++] = (i + 1 == lodLevel ? vIndex : vIndex + i + 1);
		}
	}
}

void ConeClosed::updateAttributes(const Config &cfg) {
    std::vector<GLuint> LODs;
    GLuint vertexOffset = 0;
    GLuint indexOffset = 0;
    for (auto &lod : cfg.levelOfDetails) {
    	GLuint lodLevel = 4u * pow(lod, 2);
		LODs.push_back(lodLevel);
		auto &x = meshLODs_.emplace_back();
		x.numVertices = lodLevel + 1;
		if (cfg.isBaseRequired) x.numVertices += 1;
		x.vertexOffset = vertexOffset;
		vertexOffset += x.numVertices;

		GLuint numFaces = lodLevel;
		if (cfg.isBaseRequired) { numFaces *= 2; }
		x.numIndices = numFaces * 3;
		x.indexOffset = indexOffset;
		indexOffset += x.numIndices;
	}
	GLuint numVertices = vertexOffset;
	GLuint numIndices = indexOffset;

	pos_->setVertexData(numVertices);
	if (cfg.isNormalRequired) {
		nor_->setVertexData(numVertices);
	}
	indices_->setVertexData(numIndices);

    for (auto i=0u; i<LODs.size(); ++i) {
    	generateLODLevel(cfg,
    			LODs[i],
    			meshLODs_[i].vertexOffset,
    			meshLODs_[i].indexOffset);
	}

	begin(ShaderInputContainer::INTERLEAVED);
	auto indexRef = setIndices(indices_, numVertices);
	setInput(pos_);
	if (cfg.isNormalRequired)
		setInput(nor_);
	end();

    for (auto &x : meshLODs_) {
    	// add the index buffer offset (in number of bytes)
    	x.indexOffset = indexRef->address() + x.indexOffset * sizeof(GLuint);
	}
    inputContainer_->set_numIndices(meshLODs_[0].numIndices);
    indices_->set_offset(meshLODs_[0].indexOffset);
}
