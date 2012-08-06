/*
 * bones-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "bones-state.h"

BonesState::BonesState()
{

}

void BonesState::setBones(
    ref_ptr<Bone> rootNode,
    vector< ref_ptr<Bone> > bones)
{
  rootBoneNode_ = rootNode;
  bones_ = bones;
  hasBones_ = (rootBoneNode_.get() != NULL && bones_.size() > 0);

  // create and join bone matrix uniform
  if(boneMatrices_.get() != NULL) {
    disjoinStates( boneMatrices_ );
  }
  // TODO BONES: use tbo instead, no size limit like uniform arrays!
  boneMatrices_ = ref_ptr<UniformMat4>::manage(
       new UniformMat4("boneMatrices", bones.size()) );

  Mat4f *m = new Mat4f[bones.size()];
  boneMatrices_->set_value((Mat4f*) m[0].x);
  delete[] m;

  joinStates( boneMatrices_ );

  // initially calculate the bone matrices
  if(hasBones_) calculateBoneMatrices();
}

void BonesState::calculateBoneMatrices()
{
  // FIXME: must be done each frame!!!
  // calculate the mesh's inverse global transform
  Mat4f inverseMeshTransform = inverse(rootBoneNode_->globalTransform());
  // ptr to bone matrix uniform
  Mat4f* boneMats = &boneMatrices_->valuePtr();

  // Bone matrices transform from mesh coordinates in bind pose
  // to mesh coordinates in skinned pose
  // Therefore the formula is:
  //    offsetMatrix * boneTransform * inverseMeshTransform
  for (unsigned int i = 0; i < bones_.size(); i++) {
    ref_ptr<Bone> bone = bones_[i];
    boneMats[i] = transpose( inverseMeshTransform * bone->globalTransform() * bone->offsetMatrix() );
  }
}

void BonesState::configureShader(ShaderConfiguration *cfg)
{
  State::configureShader(cfg);
  cfg->setHasBones(true);
}
