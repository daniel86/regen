#ifndef REGEN_MESH_SIMPLIFIER_H
#define REGEN_MESH_SIMPLIFIER_H

#include <regen/meshes/mesh-state.h>
#include <queue>
#include <utility>
#include <unordered_set>
#include "regen/math/Quadric.h"
#include "lod-attribute.h"
#include "attribute-semantic.h"
#include "triangle.h"
#include "lod-level.h"
#include "edge-collapse.h"

namespace regen {
	/**
	 * \brief Mesh simplification class.
	 *
	 * This class implements a mesh simplification algorithm based on
	 * edge collapses. It uses a priority queue to select the edges to
	 * collapse based on the cost of the collapse.
	 */
	class MeshSimplifier {
	public:
		/**
		 * \brief Default Constructor.
		 * @param mesh The mesh to be simplified.
		 */
		explicit MeshSimplifier(const ref_ptr<Mesh> &mesh);

		/**
		 * Set the thresholds for LOD levels.
		 * Each value is a fraction of the original mesh size measured in number of faces.
		 * @param lodMid fraction of faces to keep for the first LOD level.
		 * @param lodFar fraction of faces to keep for the second LOD level.
		 */
		void setThresholds(float lodMid, float lodFar) {
			thresholds_[0] = lodMid;
			thresholds_[1] = lodFar;
		}

		/**
		 * Run the mesh simplification algorithm.
		 */
		void simplifyMesh();

	protected:
		bool hasValidAttributes_ = true;
		float thresholds_[2] = { 0.75f, 0.25f };

		ref_ptr<Mesh> mesh_;
		ref_ptr<ShaderInput1ui> inputIndices_;
		ref_ptr<ShaderInput3f> inputPos_;
		std::vector<std::pair<AttributeSemantic,ref_ptr<ShaderInput>>> inputAttributes_;

		ref_ptr<ShaderInput1ui> outputIndices_;
		ref_ptr<ShaderInput3f> outputPos_;
		std::vector<std::pair<AttributeSemantic,ref_ptr<ShaderInput>>> outputAttributes_;

		// array of faces for each LOD level. element 0 is the original mesh.
		// after each iteration, the faces of the current LOD level are
		// stored in lodLevels_[i], where i is the LOD level.
		std::vector<std::vector<Triangle>> lodLevels_;
		std::vector<LODLevel> lodData_;
		// per vertex quadrics
		std::vector<Quadric> quadrics_;
		std::vector<bool> isBoundary_;
		std::vector<std::unordered_set<uint32_t>> neighbors_;
		EgdeCollapseQueue edgeCollapses_;
		// vertex offset for the next LOD level
		unsigned int vertexOffset_ = 0;

		bool addInputAttribute(const NamedShaderInput &namedAttribute);

		void applyAttributes();

		uint32_t createOutputAttributes();

		void computeQuadrics(const std::vector<Triangle> &faces, const Vec3f *posData, uint32_t numVertices);

		void buildEdgeQueue(const std::vector<Triangle> &faces, const LODLevel &data);

		void pushEdge(uint32_t idx0, uint32_t idx2, const Quadric &Q, const Vec3f *posData);

		uint32_t generateLodLevel(size_t targetFaceCount, LODLevel &lodData);

		void updateEdgeCosts(uint32_t vNew, const LODLevel &levelData, std::vector<uint32_t> &mapping);

		static bool solve(const Quadric& Q, Vec3f& outPos);

		bool isBoundaryVertex(uint32_t idx) const;
	};
}

#endif //REGEN_MESH_SIMPLIFIER_H
