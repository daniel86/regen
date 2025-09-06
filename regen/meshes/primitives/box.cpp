#include "../lod/tessellation.h"
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
		cfg.levelOfDetails = {0};
		mesh = ref_ptr<Box>::alloc(cfg);
	}
	return ref_ptr<Box>::alloc(mesh);
}

Box::Box(const Config &cfg)
		: Mesh(GL_TRIANGLES, cfg.updateHint),
		  texcoMode_(cfg.texcoMode) {
	pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	nor_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	tan_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);
	setBufferMapMode(cfg.mapMode);
	setClientAccessMode(cfg.accessMode);
	updateAttributes(cfg);
}

Box::Box(const ref_ptr<Box> &other)
		: Mesh(other),
		  texcoMode_(other->texcoMode_) {
	pos_ = ref_ptr<ShaderInput3f>::dynamicCast(getInput(ATTRIBUTE_NAME_POS));
	nor_ = ref_ptr<ShaderInput3f>::dynamicCast(getInput(ATTRIBUTE_NAME_NOR));
	tan_ = ref_ptr<ShaderInput4f>::dynamicCast(getInput(ATTRIBUTE_NAME_TAN));
	indices_ = getInput("i");
}

Box::Config::Config()
		: levelOfDetails({0}),
		  posScale(Vec3f(1.0f)),
		  rotation(Vec3f(0.0f)),
		  texcoScale(Vec2f(1.0f)),
		  texcoMode(TEXCO_MODE_UV),
		  isNormalRequired(GL_TRUE),
		  isTangentRequired(GL_FALSE) {
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
	auto vertexOffset = meshLODs_[lodLevel].d->vertexOffset + sideIndex * tessellation.vertices.size();
	auto indexOffset = meshLODs_[lodLevel].d->indexOffset + sideIndex * tessellation.outputFaces.size() * 3;
	const Vec3f &normal = cubeNormals[sideIndex];
	const Mat4f &faceRotMat = faceRotations[sideIndex];
	GLuint nextIndex = indexOffset;

	// map client data for writing
	auto pos = (Vec3f*) pos_->clientBuffer()->clientData(0);
	auto nor = (cfg.isNormalRequired ?
				  (Vec3f*) nor_->clientBuffer()->clientData(0) : nullptr);
	auto tan = (cfg.isTangentRequired ?
				  (Vec4f*) tan_->clientBuffer()->clientData(0) : nullptr);
	auto indices = (byte*)indices_->clientBuffer()->clientData(0);
	auto indexType = indices_->baseType();

	for (const auto &tessFace: tessellation.outputFaces) {
		setIndexValue(indices, indexType, nextIndex++, vertexOffset + tessFace.v1);
		setIndexValue(indices, indexType, nextIndex++, vertexOffset + tessFace.v2);
		setIndexValue(indices, indexType, nextIndex++, vertexOffset + tessFace.v3);
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
			pos[vertexIndex] = transformedVertex;
			minPosition_.setMin(transformedVertex);
			maxPosition_.setMax(transformedVertex);
			if (cfg.isNormalRequired) {
				nor[vertexIndex] = normal;
			}
			if (texcoMode_ == TEXCO_MODE_CUBE_MAP) {
				auto texco = (Vec3f*) texco_->clientBuffer()->clientData(0);
				Vec3f v = faceVertex;
				v.normalize();
				texco[vertexIndex] = v;
				triTexco[faceVertIndex] = Vec2f(vertex.x, vertex.y) * 0.5f + Vec2f(0.5f);
			} else if (texcoMode_ == TEXCO_MODE_UV) {
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
				auto texco = (Vec2f*) texco_->clientBuffer()->clientData(0);
				texco[vertexIndex] = uv;
				triTexco[faceVertIndex] = uv;
			}
			if (cfg.isTangentRequired) {
				triVertices[faceVertIndex] = pos[vertexIndex];
			}
			faceVertIndex += 1;
		}

		if (cfg.isTangentRequired) {
			Vec4f tangent = calculateTangent(triVertices, triTexco, normal);
			for (GLuint i = 0; i < 3; ++i) {
				tan[triIndices[i]] = tangent;
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
		x.d->numVertices = lodTess.vertices.size() * 6; // 6 faces
		x.d->numIndices = lodTess.outputFaces.size() * 3 * 6; // 6 faces
		x.d->vertexOffset = numVertices;
		x.d->indexOffset = numIndices;
		numVertices += x.d->numVertices;
		numIndices += x.d->numIndices;
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
	indices_ = createIndexInput(numIndices, numVertices);

	minPosition_ = Vec3f(999999.9);
	maxPosition_ = Vec3f(-999999.9);
	for (auto lodLevel = 0u; lodLevel < tessellations.size(); ++lodLevel) {
		for (GLuint sideIndex = 0; sideIndex < 6; ++sideIndex) {
			generateLODLevel(cfg, sideIndex, lodLevel, tessellations);
		}
	}

	auto indexRef = setIndices(indices_, numVertices);
	setInput(pos_);
	if (cfg.isNormalRequired)
		setInput(nor_);
	if (cfg.texcoMode != TEXCO_MODE_NONE)
		setInput(texco_);
	if (cfg.isTangentRequired)
		setInput(tan_);
	updateVertexData();

	for (auto &x: meshLODs_) {
		// add the index buffer offset (in number of bytes)
		x.d->indexOffset = indexRef->address() + x.d->indexOffset * indices_->dataTypeBytes();
	}
	activateLOD(0);
}
