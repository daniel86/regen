/*
 * rectangle.cpp
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#include "../lod/tessellation.h"
#include "rectangle.h"

using namespace regen;

ref_ptr<Rectangle> Rectangle::getUnitQuad() {
	static ref_ptr<Rectangle> mesh;
	if (mesh.get() == nullptr) {
		Config cfg;
		cfg.centerAtOrigin = GL_FALSE;
		cfg.isNormalRequired = GL_FALSE;
		cfg.isTangentRequired = GL_FALSE;
		cfg.isTexcoRequired = GL_FALSE;
		cfg.levelOfDetails = {0};
		cfg.posScale = Vec3f(2.0f);
		cfg.rotation = Vec3f(0.5 * M_PI, 0.0f, 0.0f);
		cfg.texcoScale = Vec2f(1.0);
		cfg.translation = Vec3f(-1.0f, -1.0f, 0.0f);
		cfg.usage = BUFFER_USAGE_STATIC_DRAW;
		mesh = ref_ptr<Rectangle>::alloc(cfg);
		return mesh;
	} else {
		return ref_ptr<Rectangle>::alloc(mesh);
	}
}

Rectangle::Rectangle(const Config &cfg)
		: Mesh(GL_TRIANGLES, cfg.usage) {
	pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	nor_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	texco_ = ref_ptr<ShaderInput2f>::alloc("texco0");
	tan_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);
	indices_ = ref_ptr<ShaderInput1ui>::alloc("i");
	updateAttributes(cfg);
}

Rectangle::Rectangle(const ref_ptr<Rectangle> &other)
		: Mesh(other) {
	pos_ = ref_ptr<ShaderInput3f>::dynamicCast(
			inputContainer_->getInput(ATTRIBUTE_NAME_POS));
	nor_ = ref_ptr<ShaderInput3f>::dynamicCast(
			inputContainer_->getInput(ATTRIBUTE_NAME_NOR));
	texco_ = ref_ptr<ShaderInput2f>::dynamicCast(
			inputContainer_->getInput("texco0"));
	tan_ = ref_ptr<ShaderInput4f>::dynamicCast(
			inputContainer_->getInput(ATTRIBUTE_NAME_TAN));
	indices_ = ref_ptr<ShaderInput1ui>::dynamicCast(
			inputContainer_->getInput("i"));
}

Rectangle::Config::Config()
		: levelOfDetails({0}),
		  posScale(Vec3f(1.0f)),
		  rotation(Vec3f(0.0f)),
		  translation(Vec3f(0.0f)),
		  texcoScale(Vec2f(1.0f)),
		  isNormalRequired(GL_TRUE),
		  isTexcoRequired(GL_TRUE),
		  isTangentRequired(GL_FALSE),
		  centerAtOrigin(GL_FALSE),
		  usage(BUFFER_USAGE_DYNAMIC_DRAW) {
}

void Rectangle::generateLODLevel(const Config &cfg,
								 const Tessellation &tessellation,
								 const Mat4f &rotMat,
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
	auto v_texco = (cfg.isTexcoRequired ?
		texco_->mapClientData<Vec2f>(ShaderData::WRITE) :
		ShaderData_rw<Vec2f>::nullData());

	GLuint nextIndex = indexOffset;
	for (auto &tessFace: tessellation.outputFaces) {
		indices.w[nextIndex++] = vertexOffset + tessFace.v1;
		indices.w[nextIndex++] = vertexOffset + tessFace.v2;
		indices.w[nextIndex++] = vertexOffset + tessFace.v3;
	}

	GLuint triIndices[3];
	Vec3f triVertices[3];
	Vec2f triTexco[3];
	Vec3f normal = rotMat.transformVector(Vec3f(0.0, -1.0, 0.0));
	Vec3f startPos;
	if (cfg.centerAtOrigin) {
		startPos = Vec3f(-cfg.posScale.x * 0.5f, 0.0f, -cfg.posScale.z * 0.5f);
	} else {
		startPos = Vec3f(0.0f, 0.0f, 0.0f);
	}

	for (GLuint faceIndex = 0; faceIndex < tessellation.outputFaces.size(); ++faceIndex) {
		auto &face = tessellation.outputFaces[faceIndex];
		GLuint faceVertIndex = 0;

		for (auto &tessIndex: {face.v1, face.v2, face.v3}) {
			auto &vertex = tessellation.vertices[tessIndex];
			auto vertexIndex = vertexOffset + tessIndex;
			triIndices[faceVertIndex] = vertexIndex;

			Vec3f pos = rotMat.transformVector(
					cfg.posScale * vertex + startPos) + cfg.translation;
			v_pos.w[vertexIndex] = pos;
			minPosition_.setMin(pos);
			maxPosition_.setMax(pos);
			if (cfg.isNormalRequired) {
				v_nor.w[vertexIndex] = normal;
			}
			if (cfg.isTexcoRequired) {
				v_texco.w[vertexIndex] = cfg.texcoScale - (cfg.texcoScale * Vec2f(vertex.x, vertex.z));
			}
			if (cfg.isTangentRequired) {
				triVertices[faceVertIndex] = v_pos.w[vertexIndex];
				triTexco[faceVertIndex] = v_texco.w[vertexIndex];
			}
			faceVertIndex += 1;
		}

		if (cfg.isTangentRequired) {
			Vec4f tangent = calculateTangent(triVertices, triTexco, normal);
			for (GLuint i = 0; i < 3; ++i) {
				v_tan.w[triIndices[i]] = tangent;
			}
		}
	}
}

void Rectangle::updateAttributes(Config cfg) {
	std::vector<Tessellation> tessellations;
	GLuint numVertices = 0;
	GLuint numIndices = 0;
	{
		Tessellation baseTess;
		baseTess.vertices.resize(4);
		baseTess.vertices[0] = Vec3f(0.0, 0.0, 0.0);
		baseTess.vertices[1] = Vec3f(1.0, 0.0, 0.0);
		baseTess.vertices[2] = Vec3f(1.0, 0.0, 1.0);
		baseTess.vertices[3] = Vec3f(0.0, 0.0, 1.0);
		baseTess.inputFaces.resize(2);
		baseTess.inputFaces[0] = TessellationFace(0, 1, 3);
		baseTess.inputFaces[1] = TessellationFace(1, 2, 3);

		for (GLuint lodLevel: cfg.levelOfDetails) {
			auto &lodTess = tessellations.emplace_back();
			lodTess.vertices = baseTess.vertices;
			lodTess.inputFaces = baseTess.inputFaces;
			tessellate(lodLevel, lodTess);

			auto &x = meshLODs_.emplace_back();
			x.numVertices = lodTess.vertices.size();
			x.numIndices = lodTess.outputFaces.size() * 3;
			x.vertexOffset = numVertices;
			x.indexOffset = numIndices;
			numVertices += lodTess.vertices.size();
			numIndices += lodTess.outputFaces.size() * 3;
		}
	}
	if (cfg.isTangentRequired) {
		cfg.isNormalRequired = GL_TRUE;
		cfg.isTexcoRequired = GL_TRUE;
	}

	// allocate attributes
	pos_->setVertexData(numVertices);
	if (cfg.isNormalRequired) {
		nor_->setVertexData(numVertices);
	}
	if (cfg.isTexcoRequired) {
		texco_->setVertexData(numVertices);
	}
	if (cfg.isTangentRequired) {
		tan_->setVertexData(numVertices);
	}
	indices_->setVertexData(numIndices);

	minPosition_ = Vec3f(0.0);
	maxPosition_ = Vec3f(0.0);

	Mat4f rotMat = Mat4f::rotationMatrix(cfg.rotation.x, cfg.rotation.y, cfg.rotation.z);
	for (auto i = 0u; i < tessellations.size(); ++i) {
		generateLODLevel(cfg,
						 tessellations[i],
						 rotMat,
						 meshLODs_[i].vertexOffset,
						 meshLODs_[i].indexOffset);
	}

	begin(InputContainer::INTERLEAVED);
	auto indexRef = setIndices(indices_, numVertices);
	setInput(pos_);
	if (cfg.isNormalRequired)
		setInput(nor_);
	if (cfg.isTexcoRequired)
		setInput(texco_);
	if (cfg.isTangentRequired)
		setInput(tan_);
	end();

	for (auto &x: meshLODs_) {
		// add the index buffer offset (in number of bytes)
		x.indexOffset = indexRef->address() + x.indexOffset * sizeof(GLuint);
	}
	activateLOD(0);
}
