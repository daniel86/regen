#ifndef REGEN_PROC_TREE_H
#define REGEN_PROC_TREE_H

#include "regen/external/proctree/proctree.h"
#include "regen/meshes/mesh-state.h"

namespace regen {
	/**
	 * A procedural tree.
	 */
	class ProcTree {
	public:
		ProcTree();

		/**
		 * note: will only take effect after calling update().
		 * @return The properties of the tree.
		 */
		auto &properties() { return handle.mProperties; }

		/**
		 * @return The trunk mesh.
		 */
		auto &trunkMesh() const { return trunk.mesh; }

		/**
		 * @return The twig mesh.
		 */
		auto &twigMesh() const { return twig.mesh; }

		/**
		 * Update the tree meshes, uploading the new data to the GPU.
		 */
		void update();

		ref_ptr<ProcTree> computeMediumDetailTree();
		ref_ptr<ProcTree> computeLowDetailTree();

	protected:
		Proctree::Tree handle;
		struct TreeMesh {
			ref_ptr<Mesh> mesh;
			ref_ptr<ShaderInput3f> pos;
			ref_ptr<ShaderInput3f> nor;
			ref_ptr<ShaderInput4f> tan;
			ref_ptr<ShaderInput2f> texco;
			ref_ptr<ShaderInput1ui> indices;
		};
		TreeMesh trunk;
		TreeMesh twig;

		void updateTrunkAttributes();

		void updateTwigAttributes();

		static void updateAttributes(TreeMesh &treeMesh,
			int numVertices, int numFaces,
			Proctree::fvec3 *vertices,
			Proctree::fvec3 *normals,
			Proctree::fvec2 *uvs,
			Proctree::ivec3 *faces);
	};
}

#endif //REGEN_PROC_TREE_H
