#include "skirt-quad.h"

using namespace regen;

namespace regen {
	class SkirtMesh : public Mesh {
	public:
		explicit SkirtMesh(const BufferUpdateFlags &updateHint);

		void addSkirtLOD(const Rectangle::Config &rectangleConfig, const Tessellation &planeTess);

		void setSkirtSize(float skirtSize) { skirtSize_ = skirtSize; }

		void updateAttributes(const Rectangle::Config &rectangleConfig);

	protected:
		std::vector<Tessellation> skirtTess_;
		ref_ptr<ShaderInput3f> pos_;
		ref_ptr<ShaderInput> indices_;
		float skirtSize_ = 0.05f;

		void generateLODLevel(
				const Rectangle::Config &cfg,
				const Tessellation &tessellation,
				const Mat4f &rotMat,
				GLuint vertexOffset,
				GLuint indexOffset);
	};
}

SkirtQuad::SkirtQuad(const Config &cfg)
		: Rectangle(cfg){
	skirtMesh_ = ref_ptr<SkirtMesh>::alloc(cfg.updateHint);
	skirtMesh_->setBufferMapMode(cfg.mapMode);
	skirtMesh_->setClientAccessMode(cfg.accessMode);
	skirtMesh_->shaderDefine("IS_SKIRT_MESH", "TRUE");
}

void SkirtQuad::setSkirtSize(float skirtSize) {
	skirtSize_ = skirtSize;
	if (skirtMesh_.get()) {
		((SkirtMesh*)skirtMesh_.get())->setSkirtSize(skirtSize);
	}
}

void SkirtQuad::tessellateRectangle(uint32_t lod, Tessellation &t) {
	Rectangle::tessellateRectangle(lod, t);
	if (skirtMesh_.get()) {
		((SkirtMesh*)skirtMesh_.get())->addSkirtLOD(rectangleConfig_, t);
	}
}

void SkirtQuad::updateAttributes() {
	Rectangle::updateAttributes();
	if (skirtMesh_.get()) {
		((SkirtMesh*)skirtMesh_.get())->updateAttributes(rectangleConfig_);
	}
}

SkirtMesh::SkirtMesh(const BufferUpdateFlags &updateHint)
		: Mesh(GL_TRIANGLES, updateHint) {
	pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
}

void SkirtMesh::updateAttributes(const Rectangle::Config &rectangleConfig) {
	GLuint numVertices = 0;
	GLuint numIndices = 0;
	for (auto &lodTess : skirtTess_) {
		auto &x = meshLODs_.emplace_back();
		x.d->numVertices = lodTess.vertices.size();
		x.d->numIndices = lodTess.outputFaces.size() * 3;
		x.d->vertexOffset = numVertices;
		x.d->indexOffset = numIndices;
		numVertices += lodTess.vertices.size();
		numIndices += lodTess.outputFaces.size() * 3;
	}

	// allocate attributes
	pos_->setVertexData(numVertices);
	indices_ = createIndexInput(numIndices, numVertices);

	minPosition_ = Vec3f::zero();
	maxPosition_ = Vec3f::zero();

	Mat4f rotMat = Mat4f::rotationMatrix(
		rectangleConfig.rotation.x,
		rectangleConfig.rotation.y,
		rectangleConfig.rotation.z);
	for (auto i = 0u; i < skirtTess_.size(); ++i) {
		generateLODLevel(rectangleConfig,
						 skirtTess_[i],
						 rotMat,
						 meshLODs_[i].d->vertexOffset,
						 meshLODs_[i].d->indexOffset);
	}
	skirtTess_.clear();

	auto indexRef = setIndices(indices_, numVertices);
	setInput(pos_);
	updateVertexData();

	for (auto &x: meshLODs_) {
		// add the index buffer offset (in number of bytes)
		x.d->indexOffset = indexRef->address() + x.d->indexOffset * indices_->dataTypeBytes();
	}
	activateLOD(0);
	shaderDefine("SKIRT_PATCH_SIZE_HALF", REGEN_STRING(rectangleConfig.posScale.x*0.5f));
}

void SkirtMesh::addSkirtLOD(
		const Rectangle::Config &rectangleConfig,
		const Tessellation &planeTess) {
	// add skirt to tessellation, i.e. duplicate boundary vertices and pull them down.
	// to this end, we first need to find the boundary edges, then we duplicate the vertices,
	// and finally we add the skirt triangles.
	auto makeEdge = [](uint32_t a, uint32_t b) {
		return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
	};
	Tessellation &skirtTess = skirtTess_.emplace_back();

	// First find boundary edges, these are the ones that are only used by one triangle.
	using Edge = std::pair<uint32_t, uint32_t>;
	std::set<Edge> boundaryEdges; {
		std::map<Edge, uint32_t> edgeCount;
		for (const auto &face: planeTess.outputFaces) {
			edgeCount[makeEdge(face.v1, face.v2)]++;
			edgeCount[makeEdge(face.v2, face.v3)]++;
			edgeCount[makeEdge(face.v3, face.v1)]++;
		}
		for (const auto &edge: edgeCount) {
			if (edge.second == 1) {
				boundaryEdges.insert(edge.first);
			}
		}
	}

	// create indices for skirt vertices
	std::map<uint32_t, uint32_t> boundaryToSkirt;
	uint32_t skirtVertexOffset = 0; //tessellation.vertices.size();
	for (const auto &edge: boundaryEdges) {
		if (boundaryToSkirt.find(edge.first) == boundaryToSkirt.end()) {
			boundaryToSkirt[edge.first] = skirtVertexOffset++;
		}
		if (boundaryToSkirt.find(edge.second) == boundaryToSkirt.end()) {
			boundaryToSkirt[edge.second] = skirtVertexOffset++;
		}
	}
	// create vertex data for skirt vertices
	Mat4f rotMat = Mat4f::rotationMatrix(
		rectangleConfig.rotation.x,
		rectangleConfig.rotation.y,
		rectangleConfig.rotation.z);
	skirtTess.vertices.resize(boundaryToSkirt.size()*2);
	for (const auto [bIdx,sIdx] : boundaryToSkirt) {
		auto boundaryPos = planeTess.vertices[bIdx];
		skirtTess.vertices[sIdx*2]   = boundaryPos;
		skirtTess.vertices[sIdx*2+1] = boundaryPos +
			rotMat.transformVector(Vec3f(0.0f, -skirtSize_, 0.0f));
	}

	// Finally, add skirt triangles: for each boundary edge, create two triangles.
    // Iterate over the original faces to avoid winding issues (probably can be improved).
    for (const auto &face : planeTess.outputFaces) {
        const Edge edges[3] = {
            Edge{face.v1, face.v2},
            Edge{face.v2, face.v3},
            Edge{face.v3, face.v1}
        };
        for (const auto &edge : edges) {
            if (boundaryEdges.find(makeEdge(edge.first, edge.second)) != boundaryEdges.end()) {
                auto skirtV1 = boundaryToSkirt[edge.first];
                auto skirtV2 = boundaryToSkirt[edge.second];
                skirtTess.outputFaces.emplace_back(skirtV1*2+1, skirtV2*2+1, skirtV1*2);
                skirtTess.outputFaces.emplace_back(skirtV2*2+1, skirtV2*2,   skirtV1*2);
            }
        }
    }
}

void SkirtMesh::generateLODLevel(const Rectangle::Config &cfg,
								 const Tessellation &skirtTess,
								 const Mat4f &rotMat,
								 GLuint vertexOffset,
								 GLuint indexOffset) {
	// map client data for writing
	auto v_pos = (Vec3f*) pos_->clientBuffer()->clientData(0);
	auto indices = (byte*)indices_->clientBuffer()->clientData(0);
	auto indexType = indices_->baseType();

	GLuint nextIndex = indexOffset;
	for (auto &tessFace: skirtTess.outputFaces) {
		setIndexValue(indices, indexType, nextIndex++, vertexOffset + tessFace.v1);
		setIndexValue(indices, indexType, nextIndex++, vertexOffset + tessFace.v2);
		setIndexValue(indices, indexType, nextIndex++, vertexOffset + tessFace.v3);
	}

	Vec3f startPos;
	if (cfg.centerAtOrigin) {
		startPos = Vec3f(-cfg.posScale.x * 0.5f, 0.0f, -cfg.posScale.z * 0.5f);
	} else {
		startPos = Vec3f(0.0f, 0.0f, 0.0f);
	}

	for (GLuint faceIndex = 0; faceIndex < skirtTess.outputFaces.size(); ++faceIndex) {
		auto &face = skirtTess.outputFaces[faceIndex];

		for (auto &tessIndex: {face.v1, face.v2, face.v3}) {
			auto &vertex = skirtTess.vertices[tessIndex];
			auto vertexIndex = vertexOffset + tessIndex;
			Vec3f pos = rotMat.transformVector(
					cfg.posScale * vertex + startPos) + cfg.translation;
			v_pos[vertexIndex] = pos;
			minPosition_.setMin(pos);
			maxPosition_.setMax(pos);
		}
	}
}
