/*
 * assimp-loader.h
 *
 *  Created on: 24.10.2011
 *      Author: daniel
 */

#ifndef ASSIMP_LOADER_H_
#define ASSIMP_LOADER_H_

#include <stdexcept>

#include <ogle/meshes/mesh-state.h>
#include <ogle/shading/light-state.h>
#include <ogle/states/material-state.h>
#include <ogle/animations/animation.h>
#include <ogle/animations/bones.h>
#include <ogle/states/camera.h>

#include <ogle/animations/animation-node.h>

// assimp include files. These three are usually needed.
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace ogle {
/**
 * \brief Load meshes using the Open Asset import Library.
 *
 * Loading of lights,materials,meshes and bone animations
 * is supported.
 * @see http://assimp.sourceforge.net/
 */
class AssimpImporter
{
public:
  /**
   * \brief Something went wrong processing the model file.
   */
  class Error : public runtime_error {
  public:
    /**
     * @param message the error message.
     */
    Error(const string &message) : runtime_error(message) {}
  };

  /**
   * @param assimpFile the file to import.
   * @param texturePath base directory for textures defined in the imported file.
   * @param assimpFlags import flags passed to assimp.
   */
  AssimpImporter(const string &assimpFile, const string &texturePath, GLint assimpFlags=-1);
  ~AssimpImporter();

  /**
   * @return list of lights defined in the assimp file.
   */
  list< ref_ptr<Light> >& lights();
  /**
   * @return list of materials defined in the assimp file.
   */
  vector< ref_ptr<Material> >& materials();
  /**
   * @return a node that animates the light position.
   */
  ref_ptr<LightNode> loadLightNode(const ref_ptr<Light> &light);
  /**
   * @return list of meshes.
   */
  list< ref_ptr<MeshState> > loadMeshes(const aiMatrix4x4 &transform=aiMatrix4x4());
  /**
   * @return the material associated to a previously loaded meshes.
   */
  ref_ptr<Material> getMeshMaterial(MeshState *state);
  /**
   * @return list of bone animation nodes associated to given mesh.
   */
  list< ref_ptr<AnimationNode> > loadMeshBones(MeshState *meshState, NodeAnimation *anim);
  /**
   * @return number of weights used for bone animation.
   */
  GLuint numBoneWeights(MeshState *meshState);
  /**
   * @return the node animation.
   */
  ref_ptr<NodeAnimation> loadNodeAnimation(
      GLboolean forceChannelStates,
      NodeAnimation::Behavior forcedPostState,
      NodeAnimation::Behavior forcedPreState,
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

} // end ogle namespace

#endif /* ASSIMP_MODEL_H_ */
