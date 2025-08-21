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
		// Maximum number of materials that can be blended.
		static uint32_t MAX_BLENDED_MATERIALS;
		// Minimum weight for a material to be considered.
		static float MIN_MATERIAL_WEIGHT;

		struct SmoothRange {
			Vec2f range;
			float smooth;
			explicit SmoothRange(const Vec3f &v) : range(v.x, v.y), smooth(v.z) {}
		};

		/**
		 * Defines the material types.
		 */
		struct MaterialConfig {
			std::string type = "dirt";
			std::string colorFile;
			std::string normalFile;
			std::string maskFile;
			float uvScale = 1.0f;
			bool isFallback = false;
			SmoothRange height = SmoothRange(Vec3f(0.0f, 1.0f, 0.1f));
			SmoothRange slope  = SmoothRange(Vec3f(0.0f, 180.0f, 10.0f));
		};

		struct BiomeDescription {
			std::string name;
			SmoothRange height      = SmoothRange(Vec3f(0.0f, 1.0f, 0.1f));
			SmoothRange slope       = SmoothRange(Vec3f(0.0f, 180.0f, 10.0f));
			SmoothRange temperature = SmoothRange(Vec3f(0.0f, 1.0f, 0.1f));
			SmoothRange rockiness   = SmoothRange(Vec3f(0.0f, 1.0f, 0.1f));
			SmoothRange humidity    = SmoothRange(Vec3f(0.0f, 1.0f, 0.1f));
			SmoothRange concavity   = SmoothRange(Vec3f(0.0f, 1.0f, 0.1f));
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
			joinSkirtStates(tf);
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

		/**
		 * Add a biome description for this ground.
		 * @param biome the biome description to add.
		 */
		void setBiome(const BiomeDescription &biome);

		/**
		 * @return the number of biomes used by this ground.
		 */
		uint32_t numBiomes() const { return biomeConfigs_.size(); }

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
		ref_ptr<ShaderInput1f> u_uvScale_;
		ref_ptr<ShaderInput1i> u_normalIdx_;

		Vec2ui numPatches_ = Vec2ui(1);
		uint32_t numPatchesPerRow_ = 1;
		float patchSize_ = 0.0f;

		ref_ptr<Material> groundMaterial_;
		std::vector<MaterialConfig> materialConfigs_;
		std::vector<BiomeDescription> biomeConfigs_;
		// Each material type has a color and normal texture, part
		// of a texture array.
		ref_ptr<Texture2DArray> materialAlbedoTex_;
		ref_ptr<Texture2DArray> materialNormalTex_;
		ref_ptr<Texture2DArray> materialMaskTex_;
		ref_ptr<TextureState> materialAlbedoState_;
		ref_ptr<TextureState> materialNormalState_;
		ref_ptr<TextureState> materialMaskState_;
		uint32_t numNormalMaps_ = 0u; // number of normal maps in the texture array

		// maps that encode the weight of each material over the ground.
		// must only be computed once in case the geometry has stable height values.
		// the weight maps also reassemble ordering in the texture arrays, e.g.
		// first weight map y coordinate maps to the second element in the texture array.
		ref_ptr<FBOState> weightFBO_;
		std::vector<ref_ptr<Texture2D>> weightMaps_;
		std::vector<ref_ptr<Texture2D>> biomeMaps_;
		ref_ptr<FullscreenPass> weightUpdatePass_;
		ref_ptr<State> weightUpdateState_;
		uint32_t weightMapSize_ = 2048;
		bool useWeightMapMips_ = false;

		void updateMaterialMaps();

		void updateWeightMaps();

		void createWeightPass();

		void updateGroundPatches();

		void updatePatchSize();

		void setSkirtInput(const ref_ptr<ShaderInput> &in);

		void joinSkirtStates(const ref_ptr<State> &state);
	};

} // namespace

#endif /* REGEN_GROUND_H_ */
