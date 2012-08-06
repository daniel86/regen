/*
 * assimp-loader.h
 *
 *  Created on: 24.10.2011
 *      Author: daniel
 */

#ifndef ASSIMP_LOADER_H_
#define ASSIMP_LOADER_H_

#include <stdexcept>

#include <ogle/states/attribute-state.h>
#include <ogle/states/light-state.h>
#include <ogle/states/material-state.h>
#include <ogle/animations/animation.h>
#include <ogle/states/camera-state.h>

#include <ogle/models/assimp-mesh.h>

#include <ogle/exceptions/io-exceptions.h>
#include <ogle/animations/bone.h>

// assimp include files. These three are usually needed.
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

/**
 * Something went wrong processing the model file.
 */
class AssimpError : public runtime_error {
public: AssimpError(const string &message) : runtime_error(message) {}
};

class AssimpScene;

/**
 * loads data from assimp model files.
 * @see http://assimp.sourceforge.net/index.html
 */
class AssimpLoader
{
public:
  /**
   * Load a model file from filesystem.
   */
  static AssimpScene& loadScene(
      const string &modelPath,
      const string &texturePath,
      int assimpFlags)
  throw(AssimpError,FileNotFoundException);

  /**
   * Assimp scene memory management.
   */
  static void deallocate(AssimpScene &scene);

  /**
   * Returns lights defined in a scene.
   */
  static vector< ref_ptr<Light> > getLights(AssimpScene &aiScene);
  /**
   * Returns materials defined in scene.
   */
  static vector< ref_ptr<Material> > getMaterials(
      AssimpScene &scene);
  /**
   * Returns primitive sets defined in scene.
   */
  static list<AttributeState*> getPrimitiveSets(
      AssimpScene &scene,
      vector< ref_ptr< Material > > &materials,
      const Vec3f &translation);

  /**
   * Returns bone animation defined in scene, if there is any.
   */
  static ref_ptr<BoneAnimation> getBoneAnimation(
      AssimpScene &scene,
      list<AttributeState*> &sets,
      bool forceChannelStates,
      AnimationBehaviour forcedPostState,
      AnimationBehaviour forcedPreState,
      double defaultTicksPerSecond=20.0);

  /**
   * not yet implemented..
   */
  static ref_ptr<Animation> getMeshAnimation(
      AssimpScene &scene, list<AttributeState*> &meshes);

private:
  static map< AssimpScene*, unsigned int > sceneReferences_;

  static bool isInitialized_;

  static ref_ptr<Bone> createNodeTree(
      AssimpScene &scene,
      aiNode* assimpNode,
      ref_ptr<Bone> parent);
  static list<AttributeState*> getPrimitiveSets(
      AssimpScene &scene,
      vector< ref_ptr< Material > > &materials,
      const Vec3f &translation,
      const struct aiNode &node,
      aiMatrix4x4 transform);

  static void initializeAssimp();
};

#endif /* ASSIMP_MODEL_H_ */
