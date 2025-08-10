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
		mesh = ref_ptr<Rectangle>::alloc(cfg);
		mesh->updateAttributes();
	}
	return ref_ptr<Rectangle>::alloc(mesh);
}

Rectangle::Rectangle(const Config &cfg)
		: Mesh(GL_TRIANGLES, cfg.updateHint),
		  rectangleConfig_(cfg) {
	pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	nor_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	texco_ = ref_ptr<ShaderInput2f>::alloc("texco0");
	tan_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);
	indices_ = ref_ptr<ShaderInput1ui>::alloc("i");
	setBufferMapMode(cfg.mapMode);
	setClientAccessMode(cfg.accessMode);
}

Rectangle::Rectangle(const ref_ptr<Rectangle> &other)
		: Mesh(other),
		  rectangleConfig_(other->rectangleConfig_) {
	pos_ = ref_ptr<ShaderInput3f>::dynamicCast(getInput(ATTRIBUTE_NAME_POS));
	nor_ = ref_ptr<ShaderInput3f>::dynamicCast(getInput(ATTRIBUTE_NAME_NOR));
	texco_ = ref_ptr<ShaderInput2f>::dynamicCast(getInput("texco0"));
	tan_ = ref_ptr<ShaderInput4f>::dynamicCast(getInput(ATTRIBUTE_NAME_TAN));
	indices_ = ref_ptr<ShaderInput1ui>::dynamicCast(getInput("i"));
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
		  centerAtOrigin(GL_FALSE) {
}

void Rectangle::generateLODLevel(const Config &cfg,
								 const Tessellation &tessellation,
								 const Mat4f &rotMat,
								 GLuint vertexOffset,
								 GLuint indexOffset) {
	// map client data for writing
	auto indices = (GLuint*)indices_->clientBuffer()->clientData(0);
	auto v_pos = (Vec3f*) pos_->clientBuffer()->clientData(0);
	auto v_nor = (cfg.isNormalRequired ?
				  (Vec3f*) nor_->clientBuffer()->clientData(0) : nullptr);
	auto v_tan = (cfg.isTangentRequired ?
				  (Vec4f*) tan_->clientBuffer()->clientData(0) : nullptr);
	auto v_texco = (cfg.isTexcoRequired ?
					(Vec2f*) texco_->clientBuffer()->clientData(0) : nullptr);

	GLuint nextIndex = indexOffset;
	for (auto &tessFace: tessellation.outputFaces) {
		indices[nextIndex++] = vertexOffset + tessFace.v1;
		indices[nextIndex++] = vertexOffset + tessFace.v2;
		indices[nextIndex++] = vertexOffset + tessFace.v3;
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
			v_pos[vertexIndex] = pos;
			minPosition_.setMin(pos);
			maxPosition_.setMax(pos);
			if (cfg.isNormalRequired) {
				v_nor[vertexIndex] = normal;
			}
			if (cfg.isTexcoRequired) {
				v_texco[vertexIndex] = cfg.texcoScale - (cfg.texcoScale * Vec2f(vertex.x, vertex.z));
			}
			if (cfg.isTangentRequired) {
				triVertices[faceVertIndex] = v_pos[vertexIndex];
				triTexco[faceVertIndex] = v_texco[vertexIndex];
			}
			faceVertIndex += 1;
		}

		if (cfg.isTangentRequired) {
			Vec4f tangent = calculateTangent(triVertices, triTexco, normal);
			for (GLuint i = 0; i < 3; ++i) {
				v_tan[triIndices[i]] = tangent;
			}
		}
	}
}

void Rectangle::tessellateRectangle(uint32_t lod, Tessellation &t) {
	tessellate(lod, t);
}

void Rectangle::updateAttributes() {
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

		for (GLuint lodLevel: rectangleConfig_.levelOfDetails) {
			auto &lodTess = tessellations.emplace_back();
			lodTess.vertices = baseTess.vertices;
			lodTess.inputFaces = baseTess.inputFaces;
			tessellateRectangle(lodLevel, lodTess);

			auto &x = meshLODs_.emplace_back();
			x.d->numVertices = lodTess.vertices.size();
			x.d->numIndices = lodTess.outputFaces.size() * 3;
			x.d->vertexOffset = numVertices;
			x.d->indexOffset = numIndices;
			numVertices += lodTess.vertices.size();
			numIndices += lodTess.outputFaces.size() * 3;
		}
	}
	if (rectangleConfig_.isTangentRequired) {
		rectangleConfig_.isNormalRequired = GL_TRUE;
		rectangleConfig_.isTexcoRequired = GL_TRUE;
	}

	// allocate attributes
	pos_->setVertexData(numVertices);
	if (rectangleConfig_.isNormalRequired) {
		nor_->setVertexData(numVertices);
	}
	if (rectangleConfig_.isTexcoRequired) {
		texco_->setVertexData(numVertices);
	}
	if (rectangleConfig_.isTangentRequired) {
		tan_->setVertexData(numVertices);
	}
	indices_->setVertexData(numIndices);

	minPosition_ = Vec3f(0.0);
	maxPosition_ = Vec3f(0.0);

	Mat4f rotMat = Mat4f::rotationMatrix(
		rectangleConfig_.rotation.x,
		rectangleConfig_.rotation.y,
		rectangleConfig_.rotation.z);
	for (auto i = 0u; i < tessellations.size(); ++i) {
		generateLODLevel(rectangleConfig_,
						 tessellations[i],
						 rotMat,
						 meshLODs_[i].d->vertexOffset,
						 meshLODs_[i].d->indexOffset);
	}

	begin(INTERLEAVED);
	auto indexRef = setIndices(indices_, numVertices);
	setInput(pos_);
	if (rectangleConfig_.isNormalRequired)
		setInput(nor_);
	if (rectangleConfig_.isTexcoRequired)
		setInput(texco_);
	if (rectangleConfig_.isTangentRequired)
		setInput(tan_);
	end();

	for (auto &x: meshLODs_) {
		// add the index buffer offset (in number of bytes)
		x.d->indexOffset = indexRef->address() + x.d->indexOffset * sizeof(GLuint);
	}
	activateLOD(0);
}
