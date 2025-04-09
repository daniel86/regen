#ifndef REGEN_MESH_SIMPLIFIER_H
#define REGEN_MESH_SIMPLIFIER_H

#include <regen/meshes/mesh-state.h>
#include "regen/math/Quadric.h"

namespace regen {
	// TODO: should be moved somewhere else
	enum class AttributeType {
		POSITION,
		NORMAL,
		TEXCOORD,
		COLOR,
		TANGENT,
		BITANGENT,
		UNKNOWN // keep this at the end
	};

	struct TriangleFace {
		int v0, v1, v2;
	};

	class MeshSimplifier {
	public:
		explicit MeshSimplifier(const ref_ptr<Mesh> &mesh);

		void run();

	protected:
		ref_ptr<Mesh> mesh_;
		std::map<AttributeType, std::list<NamedShaderInput>> inputAttributes_;
		std::map<AttributeType, std::list<NamedShaderInput>> outputAttributes_;
		ref_ptr<ShaderInput1ui> inputIndices_;
		ref_ptr<ShaderInput1ui> outputIndices_;
		// per vertex quadrics
		std::vector<Quadric> quadrics_;
		bool hasValidAttributes_ = true;

		struct Triangle {
			// Indices into the original vertex array
			uint32_t v0, v1, v2;
			// A flag to indicate if the triangle is still active, i.e. not collapsed
			bool active = true;
			// A flag to indicate if the triangle is degenerate (i.e. has zero area)
			bool isDegenerate() const {
				return v0 == v1 || v1 == v2 || v2 == v0;
			}
			Triangle(uint32_t _v0, uint32_t _v1, uint32_t _v2)
					: v0(_v0), v1(_v1), v2(_v2) {}
		};
		// array of faces for each LOD level. element 0 is the original mesh.
		// after each iteration, the faces of the current LOD level are
		// stored in lodLevels_[i], where i is the LOD level.
		std::vector<std::vector<Triangle>> lodLevels_;

		struct EdgeCollapse {
			uint32_t v1, v2;
			Vec3f optimalPos;
			double cost;
		};

		void createAttribute(const NamedShaderInput &namedAttribute);

		void applyAttributes();

		void computeQuadrics();
	};
}

#endif //REGEN_MESH_SIMPLIFIER_H
