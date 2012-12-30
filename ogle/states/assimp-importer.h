/*
 * assimp-loader.h
 *
 *  Created on: 24.10.2011
 *      Author: daniel
 */

#ifndef ASSIMP_LOADER_H_
#define ASSIMP_LOADER_H_

#include <stdexcept>

#include <ogle/states/mesh-state.h>
#include <ogle/states/light-state.h>
#include <ogle/states/material-state.h>
#include <ogle/animations/animation.h>
#include <ogle/states/camera.h>
#include <ogle/states/bones-state.h>

#include <ogle/exceptions/io-exceptions.h>
#include <ogle/animations/animation-node.h>

// assimp include files. These three are usually needed.
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

/**
 * Something went wrong processing the model file.
 */
class AssimpError : public runtime_error
{
public:
  AssimpError(const string &message)
  : runtime_error(message) {}
};

/**
 * Loads assimp files.
 * Loading of lights,materials,meshes and bone animations
 * is supported.
 */
class AssimpImporter
{
public:
  AssimpImporter(
      const string &assimpFile,
      const string &texturePath,
      GLint assimpFlags=-1) throw(AssimpError);
  ~AssimpImporter();

  /**
   * Returns list of lights defined in the assimp file.
   */
  list< ref_ptr<Light> >& lights();

  /**
   * Returns list of materials defined in the assimp file.
   */
  vector< ref_ptr<Material> >& materials();

  /**
   * A node that animates the light position.
   */
  ref_ptr<LightNode> loadLightNode(ref_ptr<Light> light);

  /**
   * Load AttributeState's from assimp file.
   */
  list< ref_ptr<MeshState> > loadMeshes(const aiMatrix4x4 &transform=aiMatrix4x4());
  /**
   * Get the material associated to a previously
   * loaded AttributeState.
   */
  ref_ptr<Material> getMeshMaterial(MeshState *state);

  list< ref_ptr<AnimationNode> > loadMeshBones(MeshState *meshState, NodeAnimation *anim);
  /**
   * Number of weights used for bone animation.
   */
  GLuint numBoneWeights(MeshState *meshState);

  /**
   * Load BoneAnimation from assimp file.
   */
  ref_ptr<NodeAnimation> loadNodeAnimation(
      GLboolean forceChannelStates,
      AnimationBehaviour forcedPostState,
      AnimationBehaviour forcedPreState,
      GLdouble defaultTicksPerSecond);

protected:
  const struct aiScene *scene_;

  // name to node map
  map<string, aiNode*> nodes_;

  // user specified texture path
  string texturePath_;

  // loaded lights
  list< ref_ptr<Light> > lights_;

  // loaded materials
  vector< ref_ptr<Material> > materials_;
  // mesh to material mapping
  map< MeshState*, ref_ptr<Material> > meshMaterials_;
  map< MeshState*, const struct aiMesh* > meshToAiMesh_;

  map< Light*, aiLight* > lightToAiLight_;

  // root node of skeleton
  ref_ptr<AnimationNode> rootNode_;
  // maps assimp bone nodes to ogle Bone implementation
  map< aiNode*, ref_ptr<AnimationNode> > aiNodeToNode_;

  //////

  list< ref_ptr<Light> > loadLights();

  vector< ref_ptr<Material> > loadMaterials();

  list< ref_ptr<MeshState> > loadMeshes(
      const struct aiNode &node,
      const aiMatrix4x4 &transform=aiMatrix4x4());
  ref_ptr<MeshState> loadMesh(
      const struct aiMesh &mesh,
      const aiMatrix4x4 &transform);

  ref_ptr<AnimationNode> loadNodeTree();
  ref_ptr<AnimationNode> loadNodeTree(
      aiNode* assimpNode, ref_ptr<AnimationNode> parent);
};

#endif /* ASSIMP_MODEL_H_ */
