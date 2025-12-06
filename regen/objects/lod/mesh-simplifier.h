#ifndef REGEN_MESH_SIMPLIFIER_H
#define REGEN_MESH_SIMPLIFIER_H

#include <regen/objects/mesh.h>
#include <queue>
#include <utility>
#include <unordered_set>
#include "regen/compute/quadric.h"
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
	 *
	 * Currently, it supports only triangle meshes with position and normal attributes.
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
		 */
		void setThresholds(const Vec4f thresholds) {
			thresholds_[0] = thresholds.x;
			thresholds_[1] = thresholds.y;
			thresholds_[2] = thresholds.z;
			thresholds_[3] = thresholds.w;
		}

		/**
		 * Set the penalty for normal interpolation.
		 * @param penalty penalty for normal interpolation.
		 */
		void setNormalPenalty(float penalty) {
			normalPenalty_ = penalty;
		}

		/**
		 * Set the maximum angle for normal interpolation.
		 * @param angle maximum angle for normal interpolation.
		 */
		void setNormalMaxAngle(float angle) {
			normalMaxAngle_ = angle;
		}

		/**
		 * Set the penalty for valence interpolation.
		 * @param penalty penalty for valence interpolation.
		 */
		void setValencePenalty(float penalty) {
			valencePenalty_ = penalty;
		}

		/**
		 * Set the penalty for area interpolation.
		 * @param penalty penalty for area interpolation.
		 */
		void setAreaPenalty(float penalty) {
			areaPenalty_ = penalty;
		}

		/**
		 * Set the use of strict boundary.
		 * In strict mode, no edges are collapsed where a vertex is a boundary vertex.
		 * Otherwise, edges can be collapsed if at most one vertex is a boundary vertex.
		 * @param useStrictBoundary true if strict boundary is used, false otherwise.
		 */
		void setUseStrictBoundary(bool useStrictBoundary) {
			useStrictBoundary_ = useStrictBoundary;
		}

		/**
		 * Run the mesh simplification algorithm.
		 */
		void simplifyMesh();

	protected:
		bool hasValidAttributes_ = true;
		float thresholds_[4] = { 1.0f, 0.75f, 0.25f, 1.0f };

		ref_ptr<Mesh> mesh_;
		ref_ptr<ShaderInput> inputIndices_;
		ref_ptr<ShaderInput3f> inputPos_;
		ref_ptr<ShaderInput3f> inputNor_;
		ref_ptr<ShaderInput1f> inputBoneWeights_;
		ref_ptr<ShaderInput1ui> inputBoneIndices_;
		std::vector<std::pair<AttributeSemantic,ref_ptr<ShaderInput>>> inputAttributes_;

		ref_ptr<ShaderInput> outputIndices_;
		ref_ptr<ShaderInput3f> outputPos_;
		std::vector<std::pair<AttributeSemantic,ref_ptr<ShaderInput>>> outputAttributes_;

		// array of faces for each LOD level. element 0 is the original mesh.
		// after each iteration, the faces of the current LOD level are
		// stored in lodLevels_[i], where i is the LOD level.
		std::vector<std::vector<Triangle>> lodLevels_;
		std::vector<LODLevel> lodData_;
		// per vertex quadrics
		std::vector<quadric> quadrics_;
		std::vector<bool> isBoundary_;
		std::vector<std::unordered_set<uint32_t>> neighbors_;
		EgdeCollapseQueue edgeCollapses_;
		// vertex offset for the next LOD level
		unsigned int vertexOffset_ = 0;
		unsigned int norIndex_ = 0;

		// some parameters for interpolation
		float valencePenalty_ = 0.1f;
		float areaPenalty_ = 0.1f;
		float normalPenalty_ = 0.1f;
		float normalMaxAngle_ = 0.6f;
		bool useStrictBoundary_ = false;

		bool addInputAttribute(const NamedShaderInput &namedAttribute);

		void applyAttributes();

		uint32_t createOutputAttributes();

		void computeQuadrics(const std::vector<Triangle> &faces, const Vec3f *posData, uint32_t numVertices);

		void buildEdgeQueue(const std::vector<Triangle> &faces, const LODLevel &data);

		void pushEdge(uint32_t idx0, uint32_t idx2, const quadric &Q, const LODLevel &lod);

		uint32_t generateLodLevel(size_t targetFaceCount, LODLevel &lodData);

		void updateEdgeCosts(uint32_t vNew, const LODLevel &levelData, std::vector<uint32_t> &mapping);

		static bool solve(const quadric& Q, Vec3f& outPos);

		bool isBoundaryVertex(uint32_t idx) const;

		bool useOriginalData();

		uint32_t getNumFaces(float threshold);

		uint32_t collapseEdge(uint32_t i1, uint32_t i2, const Vec3f &opt, LODLevel &level);
	};
}

#endif //REGEN_MESH_SIMPLIFIER_H
