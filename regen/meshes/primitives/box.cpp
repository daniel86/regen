/*
 * box.cpp
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#include "../tessellation.h"
#include "box.h"

using namespace regen;

namespace regen {
	std::ostream &operator<<(std::ostream &out, const Box::TexcoMode &mode) {
		switch (mode) {
			case Box::TEXCO_MODE_NONE:
				return out << "NONE";
			case Box::TEXCO_MODE_UV:
				return out << "UV";
			case Box::TEXCO_MODE_CUBE_MAP:
				return out << "CUBE_MAP";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, Box::TexcoMode &mode) {
		std::string val;
		in >> val;
		boost::to_upper(val);
		if (val == "NONE") mode = Box::TEXCO_MODE_NONE;
		else if (val == "UV") mode = Box::TEXCO_MODE_UV;
		else if (val == "CUBE_MAP") mode = Box::TEXCO_MODE_CUBE_MAP;
		else {
			REGEN_WARN("Unknown box texco mode '" << val << "'. Using NONE texco.");
			mode = Box::TEXCO_MODE_NONE;
		}
		return in;
	}
}

ref_ptr<Box> Box::getUnitCube() {
	static ref_ptr<Box> mesh;
	if (mesh.get() == nullptr) {
		Config cfg;
		cfg.posScale = Vec3f(1.0f);
		cfg.rotation = Vec3f(0.0, 0.0f, 0.0f);
		cfg.texcoMode = TEXCO_MODE_NONE;
		cfg.isNormalRequired = GL_FALSE;
		cfg.isTangentRequired = GL_FALSE;
		cfg.usage = VBO::USAGE_STATIC;
		cfg.levelOfDetails = {0};
		mesh = ref_ptr<Box>::alloc(cfg);
		return mesh;
	} else {
		return ref_ptr<Box>::alloc(mesh);
	}
}

Box::Box(const Config &cfg)
		: Mesh(GL_TRIANGLES, cfg.usage),
		  texcoMode_(cfg.texcoMode) {
	pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	nor_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	tan_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);
	indices_ = ref_ptr<ShaderInput1ui>::alloc("i");
	updateAttributes(cfg);
}

Box::Box(const ref_ptr<Box> &other)
		: Mesh(other),
		  texcoMode_(other->texcoMode_) {
	pos_ = ref_ptr<ShaderInput3f>::dynamicCast(
			inputContainer_->getInput(ATTRIBUTE_NAME_POS));
	nor_ = ref_ptr<ShaderInput3f>::dynamicCast(
			inputContainer_->getInput(ATTRIBUTE_NAME_NOR));
	tan_ = ref_ptr<ShaderInput4f>::dynamicCast(
			inputContainer_->getInput(ATTRIBUTE_NAME_TAN));
	indices_ = ref_ptr<ShaderInput1ui>::dynamicCast(
			inputContainer_->getInput("i"));
}

Box::Config::Config()
		: levelOfDetails({0}),
		  posScale(Vec3f(1.0f)),
		  rotation(Vec3f(0.0f)),
		  texcoScale(Vec2f(1.0f)),
		  texcoMode(TEXCO_MODE_UV),
		  isNormalRequired(GL_TRUE),
		  isTangentRequired(GL_FALSE),
		  usage(VBO::USAGE_DYNAMIC) {
}

void Box::generateLODLevel(
		const Config &cfg,
		GLuint sideIndex,
		GLuint lodLevel,
		const std::vector<Tessellation> &tessellations) {
	static const Vec3f cubeNormals[] = {
			Vec3f(0.0f, 0.0f, 1.0f), // Front
			Vec3f(0.0f, 0.0f, -1.0f), // Back
			Vec3f(0.0f, 1.0f, 0.0f), // Top
			Vec3f(0.0f, -1.0f, 0.0f), // Bottom
			Vec3f(1.0f, 0.0f, 0.0f), // Right
			Vec3f(-1.0f, 0.0f, 0.0f)  // Left
	};
	static const Mat4f faceRotations[] = {
			Mat4f::identity(), // Front
			Mat4f::rotationMatrix(0.0f, M_PI, 0.0f), // Back
			Mat4f::rotationMatrix(M_PI_2, 0.0f, 0.0f), // Top
			Mat4f::rotationMatrix(-M_PI_2, 0.0f, 0.0f), // Bottom
			Mat4f::rotationMatrix(0.0f, -M_PI_2, 0.0f), // Right
			Mat4f::rotationMatrix(0.0f, M_PI_2, 0.0f)  // Left
	};

	auto &tessellation = tessellations[lodLevel];
	auto vertexOffset = meshLODs_[lodLevel].vertexOffset + sideIndex * tessellation.vertices.size();
	auto indexOffset = meshLODs_[lodLevel].indexOffset + sideIndex * tessellation.outputFaces.size() * 3;
	const Vec3f &normal = cubeNormals[sideIndex];
	const Mat4f &faceRotMat = faceRotations[sideIndex];
	auto *indicesPtr = (GLuint *) indices_->clientDataPtr();
	GLuint nextIndex = indexOffset;

	for (const auto &tessFace: tessellation.outputFaces) {
		indicesPtr[nextIndex++] = vertexOffset + tessFace.v1;
		indicesPtr[nextIndex++] = vertexOffset + tessFace.v2;
		indicesPtr[nextIndex++] = vertexOffset + tessFace.v3;
	}

	GLuint triIndices[3];
	Vec3f triVertices[3];
	Vec2f triTexco[3];

	for (GLuint faceIndex = 0; faceIndex < tessellation.outputFaces.size(); ++faceIndex) {
		const auto &face = tessellation.outputFaces[faceIndex];
		GLuint faceVertIndex = 0;

		for (const auto &tessIndex: {face.v1, face.v2, face.v3}) {
			const auto &vertex = tessellation.vertices[tessIndex];
			auto vertexIndex = vertexOffset + tessIndex;
			triIndices[faceVertIndex] = vertexIndex;

			Vec3f faceVertex = faceRotMat.transformVector(vertex) + normal;
			Vec3f transformedVertex = cfg.posScale * modelRotation_.transformVector(faceVertex);
			pos_->setVertex(vertexIndex, transformedVertex);
			minPosition_.setMin(transformedVertex);
			maxPosition_.setMax(transformedVertex);
			if (cfg.isNormalRequired) {
				nor_->setVertex(vertexIndex, normal);
			}
			if (texcoMode_ == TEXCO_MODE_CUBE_MAP) {
				auto *texco = (Vec3f *) texco_->clientData();
				Vec3f v = faceVertex;
				v.normalize();
				texco[vertexIndex] = v;
				triTexco[faceVertIndex] = Vec2f(vertex.x, vertex.y) * 0.5f + Vec2f(0.5f);
			} else if (texcoMode_ == TEXCO_MODE_UV) {
				auto *texco = (Vec2f *) texco_->clientData();
				Vec2f uv;
				switch (sideIndex) {
					case 0: // Front face
						uv = Vec2f((faceVertex.x + 1.0f) * 0.5f, (faceVertex.y + 1.0f) * 0.5f);
						uv.x *= cfg.posScale.x;
						uv.y *= cfg.posScale.y;
						break;
					case 1: // Back face
						uv = Vec2f((faceVertex.x + 1.0f) * 0.5f, (faceVertex.y + 1.0f) * 0.5f);
						uv.x *= cfg.posScale.x;
						uv.y *= cfg.posScale.y;
						break;
					case 2: // Top face
						uv = Vec2f((faceVertex.x + 1.0f) * 0.5f, (faceVertex.z + 1.0f) * 0.5f);
						uv.x *= cfg.posScale.x;
						uv.y *= cfg.posScale.z;
						break;
					case 3: // Bottom face
						uv = Vec2f((faceVertex.x + 1.0f) * 0.5f, (faceVertex.z + 1.0f) * 0.5f);
						uv.x *= cfg.posScale.x;
						uv.y *= cfg.posScale.z;
						break;
					case 4: // Right face
						uv = Vec2f((faceVertex.z + 1.0f) * 0.5f, (faceVertex.y + 1.0f) * 0.5f);
						uv.x *= cfg.posScale.z;
						uv.y *= cfg.posScale.y;
						break;
					case 5: // Left face
						uv = Vec2f((1.0f - faceVertex.z) * 0.5f, (faceVertex.y + 1.0f) * 0.5f);
						uv.x *= cfg.posScale.z;
						uv.y *= cfg.posScale.y;
						break;
					default:
						uv = Vec2f(0.0f);
				}
				uv *= cfg.texcoScale;
				texco[vertexIndex] = uv;
				triTexco[faceVertIndex] = uv;
			}
			if (cfg.isTangentRequired) {
				triVertices[faceVertIndex] = pos_->getVertex(vertexIndex);
			}
			faceVertIndex += 1;
		}

		if (cfg.isTangentRequired) {
			Vec4f tangent = calculateTangent(triVertices, triTexco, normal);
			for (GLuint i = 0; i < 3; ++i) {
				tan_->setVertex(triIndices[i], tangent);
			}
		}
	}
}

void Box::updateAttributes(const Config &cfg) {
	std::vector<Tessellation> tessellations;
	GLuint numVertices = 0;
	GLuint numIndices = 0;

	// Generate base tessellation for a single face (front face)
	Tessellation baseTess;
	baseTess.vertices.resize(4);
	baseTess.vertices[0] = Vec3f(-1.0, -1.0, 0.0);
	baseTess.vertices[1] = Vec3f(1.0, -1.0, 0.0);
	baseTess.vertices[2] = Vec3f(1.0, 1.0, 0.0);
	baseTess.vertices[3] = Vec3f(-1.0, 1.0, 0.0);
	baseTess.inputFaces.resize(2);
	baseTess.inputFaces[0] = TessellationFace(0, 1, 3);
	baseTess.inputFaces[1] = TessellationFace(1, 2, 3);

	// Generate tessellations for each LOD level
	for (GLuint lodLevel: cfg.levelOfDetails) {
		auto &lodTess = tessellations.emplace_back();
		lodTess.vertices = baseTess.vertices;
		lodTess.inputFaces = baseTess.inputFaces;
		tessellate(lodLevel, lodTess);

		auto &x = meshLODs_.emplace_back();
		x.numVertices = lodTess.vertices.size() * 6; // 6 faces
		x.numIndices = lodTess.outputFaces.size() * 3 * 6; // 6 faces
		x.vertexOffset = numVertices;
		x.indexOffset = numIndices;
		numVertices += x.numVertices;
		numIndices += x.numIndices;
	}

	modelRotation_ = Mat4f::rotationMatrix(cfg.rotation.x, cfg.rotation.y, cfg.rotation.z);

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
	if (texcoMode == TEXCO_MODE_CUBE_MAP) {
		texco_ = ref_ptr<ShaderInput3f>::alloc("texco0");
		texco_->setVertexData(numVertices);
	} else if (texcoMode == TEXCO_MODE_UV) {
		texco_ = ref_ptr<ShaderInput2f>::alloc("texco0");
		texco_->setVertexData(numVertices);
	}
	indices_->setVertexData(numIndices);

	minPosition_ = Vec3f(999999.9);
	maxPosition_ = Vec3f(-999999.9);
	for (auto lodLevel = 0u; lodLevel < tessellations.size(); ++lodLevel) {
		for (GLuint sideIndex = 0; sideIndex < 6; ++sideIndex) {
			generateLODLevel(cfg, sideIndex, lodLevel, tessellations);
		}
	}

	begin(ShaderInputContainer::INTERLEAVED);
	auto indexRef = setIndices(indices_, numVertices);
	setInput(pos_);
	if (cfg.isNormalRequired)
		setInput(nor_);
	if (cfg.texcoMode != TEXCO_MODE_NONE)
		setInput(texco_);
	if (cfg.isTangentRequired)
		setInput(tan_);
	auto vertexRef = end();

	for (auto &x: meshLODs_) {
		// add the index buffer offset (in number of bytes)
		x.indexOffset = indexRef->address() + x.indexOffset * sizeof(GLuint);
	}
	activateLOD(0);
}
