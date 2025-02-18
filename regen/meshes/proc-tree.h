#ifndef REGEN_PROC_TREE_H
#define REGEN_PROC_TREE_H

#include "regen/external/proctree/proctree.h"
#include "regen/meshes/mesh-state.h"
#include "regen/states/material-state.h"
#include "regen/scene/scene-input.h"

namespace regen {
	/**
	 * A procedural tree.
	 */
	class ProcTree {
	public:
		enum Preset {
			PRESET_NONE = 0,
			PRESET_FIR,
			PRESET_OAK_GREEN,
			PRESET_OAK_RED,
			PRESET_OLIVE,
			PRESET_PINE
		};

		ProcTree();

		explicit ProcTree(Preset preset);

		explicit ProcTree(scene::SceneInputNode &input);

		/**
		 * Set whether to use LODs.
		 * @param useLODs True to use LODs, false otherwise.
		 */
		void setUseLODs(bool useLODs) { useLODs_ = useLODs; }

		/**
		 * Load a preset tree.
		 * @param preset The preset to load.
		 */
		void loadPreset(Preset preset);

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
		struct ProcMesh {
			int mVertCount;
			int mFaceCount;
			Proctree::fvec3 *mVert;
			Proctree::fvec3 *mNormal;
			Proctree::fvec2 *mUV;
			Proctree::ivec3 *mFace;
		};

		TreeMesh trunk;
		TreeMesh twig;
		ref_ptr<Material> trunkMaterial_;
		ref_ptr<Material> twigMaterial_;

		bool useLODs_ = true;
		ref_ptr<Proctree::Tree> lodMedium_;
		ref_ptr<Proctree::Tree> lodLow_;

		void updateTrunkAttributes();

		void updateTwigAttributes();

		ref_ptr<Proctree::Tree> computeMediumDetailTree();

		ref_ptr<Proctree::Tree> computeLowDetailTree();

		void updateAttributes(TreeMesh &treeMesh, const std::vector<ProcMesh> &procLODs) const;

		static void computeTan(TreeMesh &treeMesh, const ProcMesh &procMesh, int vertexOffset, Vec4f *tanData);

		static ProcTree::ProcMesh trunkProcMesh(Proctree::Tree &x);

		static ProcTree::ProcMesh twigProcMesh(Proctree::Tree &x);
	};
}

#endif //REGEN_PROC_TREE_H
