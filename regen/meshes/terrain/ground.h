#ifndef REGEN_GROUND_H_
#define REGEN_GROUND_H_

#include <regen/meshes/primitives/skirt-quad.h>
#include "regen/states/material-state.h"
#include "regen/textures/texture.h"
#include "regen/textures/fbo-state.h"
#include "regen/states/fullscreen-pass.h"

namespace regen {
	/**
	 * \brief Ground mesh.
	 *
	 * This class is used to create a ground mesh. For the geometry, patches of quads with "skirt" are used.
	 * The skirts are used to hide the seams between adjacent patches of terrain with different LOD.
	 * A fixed set of LOD levels is generated, and can be used to alter the detail based on distance to
	 * the camera. The geometry is altered in shaders based on height value.
	 * The coloring of the ground is done by blending multiple materials via weights.
	 * For this, weight maps are computed, which are used to blend between the different materials.
	 */
	class Ground : public SkirtQuad {
	public:
		/**
		 * Defines the material types.
		 */
		struct MaterialConfig {
			std::string type = "dirt";
			std::string colorFile;
			std::string normalFile;
			std::string maskFile;
			Vec2f heightRange = Vec2f(0.0f, 1.0f);
			float heightSmoothStep = 0.1f;
			Vec2f slopeRange = Vec2f(0.0f, 180.0f);
			float slopeSmoothStep = 10.0f;
			float uvScale = 1.0f;
			bool isFallback = false;
		};

		Ground();

		/**
		 * @return the material used by this ground.
		 */
		ref_ptr<Material>& groundMaterial() { return groundMaterial_; }

		/**
		 * Assigns a model transformation to this ground.
		 * @param tf the model transformation to assign.
		 */
		void setModelTransform(const ref_ptr<ModelTransformation> &tf) {
			tf_ = tf;
			joinStates(tf);
		}

		/**
		 * @return the model transformation assigned to this ground.
		 */
		const ref_ptr<ModelTransformation> &tf() const { return tf_; }

		/**
		 * Set LOD configuration parameters.
		 * @param numPatchesPerRow number of patches per row, for square ground
		 *                      you get numPatchesPerRow*numPatchesPerRow patches.
		 * @param levelOfDetails lod levels for each patch.
		 */
		void setLODConfig(uint32_t numPatchesPerRow,
				const std::vector<GLuint> &levelOfDetails);

		/**
		 * Set the geometry of the ground.
		 * @param mapCenter the center of the map.
		 * @param mapSize the extent of the map.
		 */
		void setMapGeometry(const Vec3f &mapCenter, const Vec3f &mapSize);

		/**
		 * Set the height and normal maps.
		 * @param heightMap the height map.
		 * @param normalMap the normal map.
		 */
		void setMapTextures(
				const ref_ptr<Texture2D> &heightMap,
				const ref_ptr<Texture2D> &normalMap);

		/**
		 * Set the material for the ground.
		 * @param type the material type.
		 * @param descr the material description.
		 * @param maskFile the mask file.
		 * @param weightKey the weight key.
		 */
		void setMaterial(const MaterialConfig &cfg);

		/**
		 * Set the size of the weight maps.
		 * @param size the size of the weight maps.
		 */
		void setWeightMapSize(uint32_t size) { weightMapSize_ = size; }

		/**
		 * Set whether to use mipmaps for the weight maps.
		 * @param useMips true if mipmaps should be used, false otherwise.
		 */
		void setUseWeightMapMips(bool useMips) { useWeightMapMips_ = useMips; }

		/**
		 * @return the number of materials used by this ground.
		 */
		uint32_t numMaterials() const { return materialConfigs_.size(); }

		// override
		void updateAttributes() override;

		// override
		virtual void createResources();

		/**
		 * Load a ground mesh from a property tree.
		 * @param ctx the loading context.
		 * @param input the input node.
		 * @return the loaded ground mesh.
		 */
		static ref_ptr<Ground> load(LoadingContext &ctx, scene::SceneInputNode &input);

	protected:
		ref_ptr<ModelTransformation> tf_;
		ref_ptr<State> groundShaderDefines_;

		Vec3f mapCenter_ = Vec3f(0.0f);
		Vec3f mapSize_ = Vec3f(1.0f);
		ref_ptr<Texture2D> heightMap_;
		ref_ptr<Texture2D> normalMap_;

		ref_ptr<ShaderInput3f> u_mapCenter_;
		ref_ptr<ShaderInput3f> u_mapSize_;
		ref_ptr<ShaderInput1f> u_skirtSize_;

		Vec2ui numPatches_ = Vec2ui(1);
		uint32_t numPatchesPerRow_ = 1;
		float patchSize_ = 0.0f;

		ref_ptr<Material> groundMaterial_;
		std::vector<MaterialConfig> materialConfigs_;
		// Each material type has a color and normal texture, part
		// of a texture array.
		ref_ptr<Texture2DArray> materialAlbedoTex_;
		ref_ptr<Texture2DArray> materialNormalTex_;
		ref_ptr<Texture2DArray> materialMaskTex_;
		ref_ptr<TextureState> materialAlbedoState_;
		ref_ptr<TextureState> materialNormalState_;
		ref_ptr<TextureState> materialMaskState_;

		// maps that encode the weight of each material over the ground.
		// must only be computed once in case the geometry has stable height values.
		// the weight maps also reassemble ordering in the texture arrays, e.g.
		// first weight map y coordinate maps to the second element in the texture array.
		ref_ptr<FBOState> weightFBO_;
		std::vector<ref_ptr<Texture2D>> weightMaps_;
		ref_ptr<FullscreenPass> weightUpdatePass_;
		ref_ptr<State> weightUpdateState_;
		uint32_t weightMapSize_ = 2048;
		bool useWeightMapMips_ = false;

		void updateMaterialMaps();

		void updateWeightMaps();

		void createWeightPass();

		void updateGroundPatches();

		void updatePatchSize();
	};

} // namespace

#endif /* REGEN_GROUND_H_ */
