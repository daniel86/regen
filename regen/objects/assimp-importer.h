/*
 * assimp-loader.h
 *
 *  Created on: 24.10.2011
 *      Author: daniel
 */

#ifndef ASSIMP_LOADER_H_
#define ASSIMP_LOADER_H_

#include <stdexcept>

#include <regen/objects/mesh.h>
#include <regen/states/light-state.h>
#include <regen/states/material-state.h>
#include <regen/animation/animation.h>
#include <regen/animation/bones.h>
#include <regen/camera/camera.h>
#include <regen/scene/loading-context.h>

#include <regen/animation/bone-tree.h>
#include <assimp/postprocess.h>

namespace regen {
	/**
	 * Configuration of animations defined in assets.
	 */
	struct AssimpAnimationConfig {
		explicit AssimpAnimationConfig(float tps=20.0f)
				: useAnimation(true),
				  numInstances(1u),
				  forceStates(true),
				  ticksPerSecond(tps),
				  postState(AnimationChannelBehavior::LINEAR),
				  preState(AnimationChannelBehavior::LINEAR) {}

		/**
		 * If false animations are ignored n the asset.
		 */
		bool useAnimation;
		/**
		 * Number of animation copies that
		 * should be created. Can be used in combination
		 * with instanced rendering.
		 */
		uint32_t numInstances;
		/**
		 * Flag indicating if pre/post states should be forced.
		 */
		bool forceStates;
		/**
		 * Animation ticks per second. Influences how fast
		 * a animation plays.
		 */
		float ticksPerSecond;
		/**
		 * Behavior when an animation stops.
		 */
		AnimationChannelBehavior postState;
		/**
		 * Behavior when an animation starts.
		 */
		AnimationChannelBehavior preState;
	};

	/**
	 * \brief Load meshes using the Open Asset import Library.
	 *
	 * Loading of lights,materials,meshes and bone animations
	 * is supported.
	 * @see http://assimp.sourceforge.net/
	 */
	class AssetImporter : public Resource {
	public:
		static constexpr const char *TYPE_NAME = "AssetImporter";

		/**
		 * \brief Flags for importing.
		 */
		enum ImportFlag {
			IGNORE_NORMAL_MAP = 1 << 0,
		};

		/**
		 * \brief Something went wrong processing the model file.
		 */
		class Error : public std::runtime_error {
		public:
			/**
			 * @param message the error message.
			 */
			explicit Error(const std::string &message) : std::runtime_error(message) {}
		};

		/**
		 * @param assetFile The file to import.
		 */
		explicit AssetImporter(const std::string &assetFile);

		~AssetImporter() override;

		/**
		 * Set the path to the texture files.
		 * @param texturePath Path to the texture files.
		 */
		void setTexturePath(const std::string &texturePath) { texturePath_ = texturePath; }

		/**
		 * Set the strength for emissive materials.
		 * @param strength Emission strength.
		 */
		void setEmissionStrength(float strength) { emissionStrength_ = strength; }

		/**
		 * Set a flag for importing.
		 */
		void setImportFlag(ImportFlag flag) { importFlags_ |= flag; }

		/**
		 * Set all import flags.
		 */
		void setImportFlags(int flags) { importFlags_ = flags; }

		/**
		 * Set AssImp process flags.
		 * @param flag AssImp process flag.
		 */
		void setAiProcessFlag(aiPostProcessSteps flag) { aiProcessFlags_ |= flag; }

		/**
		 * Unset AssImp process flags.
		 * @param flag AssImp process flag.
		 */
		void unsetAiProcessFlag(aiPostProcessSteps flag) { aiProcessFlags_ &= ~flag; }

		/**
		 * Set AssImp process flags.
		 * @param flags AssImp process flags.
		 */
		void setAiProcessFlags(int flags) { aiProcessFlags_ = flags; }

		/**
		 * Set preset process flags for fast import.
		 */
		void setAiProcessFlags_Fast();

		/**
		 * Set preset process flags for quality import.
		 */
		void setAiProcessFlags_Quality();

		/**
		 * Set preset process flags.
		 */
		void setAiProcessFlags_Regen();

		/**
		 * Set animation configuration.
		 * @param animationCfg Animation configuration.
		 */
		void setAnimationConfig(const AssimpAnimationConfig &animationCfg) { animationCfg_ = animationCfg; }

		const ref_ptr<BoneTree> &getNodeAnimation() { return nodeAnimation_; }

		/**
		 * Load the asset file.
		 */
		void importAsset();

		/**
		 * @return list of lights defined in the assimp file.
		 */
		std::vector<ref_ptr<Light> > &lights() { return lights_; }

		/**
		 * @return list of materials defined in the assimp file.
		 */
		std::vector<ref_ptr<Material> > &materials() { return materials_; }

		/**
		 * @return a node that animates the light position.
		 */
		ref_ptr<LightNode> loadLightNode(const ref_ptr<Light> &light);

		/**
		 * Create Mesh instances from Asset file.
		 * Import all meshes defined in Asset file.
		 * @param transform Transformation applied during import.
		 * @param bufferCfg Buffer usage flags.
		 * @return vector of successfully created meshes.
		 */
		std::vector<ref_ptr<Mesh> > loadAllMeshes(
				const Mat4f &transform, const BufferFlags &bufferFlags);

		/**
		 * Create Mesh instances from Asset file.
		 * @param transform Transformation applied during import.
		 * @param bufferCfg Buffer usage flags.
		 * @param meshIndices Mesh indices in Asset file.
		 * @return vector of successfully created meshes.
		 */
		std::vector<ref_ptr<Mesh> > loadMeshes(
				const Mat4f &transform,
				const BufferFlags &bufferFlags,
				const std::vector<GLuint> &meshIndices);

		/**
		 * @return the material associated to a previously loaded meshes.
		 */
		ref_ptr<Material> getMeshMaterial(Mesh *state);

		/**
		 * @return list of bone animation nodes associated to given mesh.
		 */
		std::list<ref_ptr<BoneNode>> loadMeshBones(Mesh *meshState, BoneTree *anim);

		/**
		 * @return number of weights used for bone animation.
		 */
		GLuint numBoneWeights(Mesh *meshState);

		static ref_ptr<AssetImporter> load(LoadingContext &ctx, scene::SceneInputNode &input);

	protected:
		std::string assetFile_;
		const struct aiScene *scene_;
		int importFlags_ = 0;
		int aiProcessFlags_ = 0;
		AssimpAnimationConfig animationCfg_;

		ref_ptr<BoneTree> nodeAnimation_;
		// name to node map
		std::map<std::string, struct aiNode *> nodes_;
		// root node of skeleton
		ref_ptr<BoneNode> rootNode_;
		// maps assimp bone nodes to Bone implementation
		std::map<struct aiNode *, ref_ptr<BoneNode>> aiNodeToNode_;

		// user specified texture path
		std::string texturePath_;
		float emissionStrength_ = 1.0f;

		// loaded lights
		std::vector<ref_ptr<Light> > lights_;

		// loaded materials
		std::vector<ref_ptr<Material> > materials_;
		// mesh to material mapping
		std::map<Mesh *, ref_ptr<Material> > meshMaterials_;
		std::map<Mesh *, const struct aiMesh *> meshToAiMesh_;

		std::map<Light *, struct aiLight *> lightToAiLight_;

		//////

		std::vector<ref_ptr<Light> > loadLights();

		std::vector<ref_ptr<Material> > loadMaterials();

		void loadMeshes(
				const struct aiNode &node,
				const Mat4f &transform,
				const BufferFlags &bufferFlags,
				const std::vector<GLuint> &meshIndices,
				GLuint &currentIndex,
				std::vector<ref_ptr<Mesh> > &out);

		ref_ptr<Mesh> loadMesh(
				const struct aiMesh &mesh,
				const Mat4f &transform,
				const BufferFlags &bufferFlags);

		void loadNodeAnimation(const AssimpAnimationConfig &animConfig);

		ref_ptr<BoneNode> loadNodeTree();

		ref_ptr<BoneNode> loadNodeTree(struct aiNode *assimpNode, const ref_ptr<BoneNode> &parent);
	};
} // namespace

#endif /* ASSIMP_MODEL_H_ */
