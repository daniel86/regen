/*
 * assimp-model.h
 *
 *  Created on: 24.10.2011
 *      Author: daniel
 */

#ifndef ASSIMP_MODEL_H_
#define ASSIMP_MODEL_H_

// assimp include files. These three are usually needed.
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <ogle/states/material-state.h>
#include <ogle/states/attribute-state.h>
#include <ogle/animations/bone.h>

/**
 * A primitive set that can load data from assimp meshes.
 */
class AssimpMesh : public AttributeState
{
public:
  /**
   * Default constructor.
   */
  AssimpMesh();

  /**
   * Set geometry from assimp mesh data.
   */
  void loadMesh(
      const struct aiNode &node,
      const struct aiMesh &mesh,
      struct aiNode &rootNode,
      ref_ptr<Bone> rootBoneNode,
      map< aiNode*, ref_ptr<Bone> > &nodeToNodeMap,
      aiMatrix4x4 &transform,
      vector< ref_ptr<Material> > &materials,
      const Vec3f &translation);
};

#endif /* ASSIMP_MODEL_H_ */
