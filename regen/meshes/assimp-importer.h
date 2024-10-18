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

#include <regen/animations/animation-node.h>

namespace regen {
	/**
	 * Configuration of animations defined in assets.
	 */
	struct AssimpAnimationConfig {
		AssimpAnimationConfig()
				: useAnimation(GL_TRUE),
				  numInstances(0u),
				  forceStates(GL_TRUE),
				  ticksPerSecond(20.0),
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
	class AssetImporter {
	public:
		/**
		 * \brief Something went wrong processing the model file.
		 */
		class Error : public std::runtime_error {
		public:
			/**
			 * @param message the error message.
			 */
			Error(const std::string &message) : std::runtime_error(message) {}
		};

		/**
		 * @param assimpFile The file to import.
		 * @param texturePath Base directory for textures defined in the imported file.
		 * @param animConfig Configuration for node animation defined in Asset file.
		 * @param assimpFlags Import flags passed to assimp.
		 */
		AssetImporter(const std::string &assimpFile,
					  const std::string &texturePath,
					  const AssimpAnimationConfig &animConfig = AssimpAnimationConfig(),
					  GLint assimpFlags = -1);

		~AssetImporter();

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
		 * @param usage VBO usage hint for vertec data.
		 * @return vector of successfully created meshes.
		 */
		std::vector<ref_ptr<Mesh> > loadAllMeshes(
				const Mat4f &transform, VBO::Usage usage);

		/**
		 * Create Mesh instances from Asset file.
		 * @param transform Transformation applied during import.
		 * @param usage VBO usage hint for vertec data.
		 * @param meshIndices Mesh indices in Asset file.
		 * @return vector of successfully created meshes.
		 */
		std::vector<ref_ptr<Mesh> > loadMeshes(
				const Mat4f &transform, VBO::Usage usage, std::vector<GLuint> meshIndices);

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

	protected:
		const struct aiScene *scene_;

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
				VBO::Usage usage,
				std::vector<GLuint> meshIndices,
				GLuint &currentIndex,
				std::vector<ref_ptr<Mesh> > &out);

		ref_ptr<Mesh> loadMesh(
				const struct aiMesh &mesh,
				const Mat4f &transform,
				VBO::Usage usage);

		void loadNodeAnimation(const AssimpAnimationConfig &animConfig);

		ref_ptr<AnimationNode> loadNodeTree();

		ref_ptr<AnimationNode> loadNodeTree(
				struct aiNode *assimpNode, ref_ptr<AnimationNode> parent);
	};
} // namespace

#endif /* ASSIMP_MODEL_H_ */
