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
#include <regen/shading/light-state.h>
#include <regen/states/material-state.h>
#include <regen/animations/animation.h>
#include <regen/animations/bones.h>
#include <regen/states/camera.h>

#include <regen/animations/animation-node.h>

namespace regen {
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
    void loadMeshes(
        const Mat4f &transform,
        VertexBufferObject::Usage usage,
        list< ref_ptr<Mesh> > &meshes);
    /**
     * @return the material associated to a previously loaded meshes.
     */
    ref_ptr<Material> getMeshMaterial(Mesh *state);
    /**
     * @return list of bone animation nodes associated to given mesh.
     */
    list< ref_ptr<AnimationNode> > loadMeshBones(Mesh *meshState, NodeAnimation *anim);
    /**
     * @return number of weights used for bone animation.
     */
    GLuint numBoneWeights(Mesh *meshState);
    /**
     * @return the node animation.
     */
    NodeAnimation* loadNodeAnimation(
        GLboolean forceChannelStates,
        NodeAnimation::Behavior forcedPostState,
        NodeAnimation::Behavior forcedPreState,
        GLdouble defaultTicksPerSecond);

  protected:
    const struct aiScene *scene_;

    // name to node map
    map<string, struct aiNode*> nodes_;

    // user specified texture path
    string texturePath_;

    // loaded lights
    list< ref_ptr<Light> > lights_;

    // loaded materials
    vector< ref_ptr<Material> > materials_;
    // mesh to material mapping
    map< Mesh*, ref_ptr<Material> > meshMaterials_;
    map< Mesh*, const struct aiMesh* > meshToAiMesh_;

    map< Light*, struct aiLight* > lightToAiLight_;

    // root node of skeleton
    ref_ptr<AnimationNode> rootNode_;
    // maps assimp bone nodes to Bone implementation
    map< struct aiNode*, ref_ptr<AnimationNode> > aiNodeToNode_;

    //////

    list< ref_ptr<Light> > loadLights();

    vector< ref_ptr<Material> > loadMaterials();

    void loadMeshes(
        const struct aiNode &node,
        const Mat4f &transform,
        VertexBufferObject::Usage usage,
        list< ref_ptr<Mesh> > &meshes);
    ref_ptr<Mesh> loadMesh(
        const struct aiMesh &mesh,
        const Mat4f &transform,
        VertexBufferObject::Usage usage);

    ref_ptr<AnimationNode> loadNodeTree();
    ref_ptr<AnimationNode> loadNodeTree(
        struct aiNode* assimpNode, ref_ptr<AnimationNode> parent);
  };
} // namespace

#endif /* ASSIMP_MODEL_H_ */
