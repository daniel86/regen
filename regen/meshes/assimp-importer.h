/*
 * assimp-loader.h
 *
 *  Created on: 24.10.2011
 *      Author: daniel
 */

#ifndef ASSIMP_LOADER_H_
#define ASSIMP_LOADER_H_

#include <stdexcept>

#include <regen/meshes/mesh-state.h>
#include <regen/states/light-state.h>
#include <regen/states/material-state.h>
#include <regen/animations/animation.h>
#include <regen/animations/bones.h>
#include <regen/camera/camera.h>
#include <regen/scene/loading-context.h>

#include <regen/animations/animation-node.h>
#include <assimp/postprocess.h>

namespace regen {
	/**
	 * Configuration of animations defined in assets.
	 */
	struct AssimpAnimationConfig {
		explicit AssimpAnimationConfig(float tps=20.0f)
				: useAnimation(GL_TRUE),
				  numInstances(1u),
				  forceStates(GL_TRUE),
				  ticksPerSecond(tps),
				  postState(NodeAnimation::BEHAVIOR_LINEAR),
				  preState(NodeAnimation::BEHAVIOR_LINEAR) {}

		/**
		 * If false animations are ignored n the asset.
		 */
		GLboolean useAnimation;
		/**
		 * Number of animation copies that
		 * should be created. Can be used in combination
		 * with instanced rendering.
		 */
		GLuint numInstances;
		/**
		 * Flag indicating if pre/post states should be forced.
		 */
		GLboolean forceStates;
		/**
		 * Animation ticks per second. Influences how fast
		 * a animation plays.
		 */
		GLfloat ticksPerSecond;
		/**
		 * Behavior when an animation stops.
		 */
		NodeAnimation::Behavior postState;
		/**
		 * Behavior when an animation starts.
		 */
		NodeAnimation::Behavior preState;
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

		/**
		 * Load the asset file.
		 */
		void importAsset();

		/**
		 * @return list of lights defined in the assimp file.
		 */
		std::vector<ref_ptr<Light> > &lights();

		/**
		 * @return list of materials defined in the assimp file.
		 */
		std::vector<ref_ptr<Material> > &materials();

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
		std::list<ref_ptr<AnimationNode> > loadMeshBones(Mesh *meshState, NodeAnimation *anim);

		/**
		 * @return number of weights used for bone animation.
		 */
		GLuint numBoneWeights(Mesh *meshState);

		/**
		 * @return asset animations.
		 */
		const std::vector<ref_ptr<NodeAnimation> > &getNodeAnimations();

		static ref_ptr<AssetImporter> load(LoadingContext &ctx, scene::SceneInputNode &input);

	protected:
		std::string assetFile_;
		const struct aiScene *scene_;
		int importFlags_ = 0;
		int aiProcessFlags_ = 0;
		AssimpAnimationConfig animationCfg_;

		std::vector<ref_ptr<NodeAnimation> > nodeAnimations_;
		// name to node map
		std::map<std::string, struct aiNode *> nodes_;

		// user specified texture path
		std::string texturePath_;

		// loaded lights
		std::vector<ref_ptr<Light> > lights_;

		// loaded materials
		std::vector<ref_ptr<Material> > materials_;
		// mesh to material mapping
		std::map<Mesh *, ref_ptr<Material> > meshMaterials_;
		std::map<Mesh *, const struct aiMesh *> meshToAiMesh_;

		std::map<Light *, struct aiLight *> lightToAiLight_;

		// root node of skeleton
		ref_ptr<AnimationNode> rootNode_;
		// maps assimp bone nodes to Bone implementation
		std::map<struct aiNode *, ref_ptr<AnimationNode> > aiNodeToNode_;

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

		ref_ptr<AnimationNode> loadNodeTree();

		ref_ptr<AnimationNode> loadNodeTree(struct aiNode *assimpNode, const ref_ptr<AnimationNode> &parent);
	};
} // namespace

#endif /* ASSIMP_MODEL_H_ */
