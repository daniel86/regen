#include "skirt-quad.h"

using namespace regen;

SkirtQuad::SkirtQuad(const Config &cfg)
		: Rectangle(cfg){
}

SkirtQuad::SkirtQuad(const ref_ptr<SkirtQuad> &other)
		: Rectangle(other) {
}

void SkirtQuad::tessellateRectangle(uint32_t lod, Tessellation &t) {
	Rectangle::tessellateRectangle(lod, t);
	addSkirt(t);
}

void SkirtQuad::addSkirt(Tessellation &tessellation) {
	// add skirt to tessellation, i.e. duplicate boundary vertices and pull them down.
	// to this end, we first need to find the boundary edges, then we duplicate the vertices,
	// and finally we add the skirt triangles.
	auto makeEdge = [](uint32_t a, uint32_t b) {
		return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
	};

	// First find boundary edges, these are the ones that are only used by one triangle.
	using Edge = std::pair<uint32_t, uint32_t>;
	std::set<Edge> boundaryEdges; {
		std::map<Edge, uint32_t> edgeCount;
		for (const auto &face: tessellation.outputFaces) {
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
	uint32_t skirtVertexOffset = tessellation.vertices.size();
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
		rectangleConfig_.rotation.x,
		rectangleConfig_.rotation.y,
		rectangleConfig_.rotation.z);
	tessellation.vertices.resize(tessellation.vertices.size() + boundaryToSkirt.size());
	for (const auto [bIdx,sIdx] : boundaryToSkirt) {
		auto boundaryPos = tessellation.vertices[bIdx];
		tessellation.vertices[sIdx] = boundaryPos +
			rotMat.transformVector(Vec3f(0.0f, -skirtSize_, 0.0f));
	}

	// Finally, add skirt triangles: for each boundary edge, create two triangles.
    // Iterate over the original faces to avoid winding issues (probably can be improved).
    for (const auto &face : tessellation.outputFaces) {
        const Edge edges[3] = {
            Edge{face.v1, face.v2},
            Edge{face.v2, face.v3},
            Edge{face.v3, face.v1}
        };
        for (const auto &edge : edges) {
            if (boundaryEdges.find(makeEdge(edge.first, edge.second)) != boundaryEdges.end()) {
                auto skirtV1 = boundaryToSkirt[edge.first];
                auto skirtV2 = boundaryToSkirt[edge.second];
                tessellation.outputFaces.emplace_back(skirtV1, skirtV2, edge.first);
                tessellation.outputFaces.emplace_back(skirtV2, edge.second, edge.first);
            }
        }
    }
}
