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
    ref_ptr<Bone> &rootNode,
    vector< ref_ptr<Bone> > &bones)
{
  rootBoneNode_ = rootNode;
  bones_ = bones;
  hasBones_ = (rootBoneNode_.get() != NULL && bones_.size() > 0);

  // create and join bone matrix uniform
  if(boneMatrices_.get() != NULL) {
    disjoinStates( boneMatrices_ );
  }
  boneMatrices_ = ref_ptr<UniformMat4>::manage(
       new UniformMat4("boneMatrices", bones.size()) );

  Mat4f *m = new Mat4f[bones.size()];
  boneMatrices_->set_value((Mat4f*) m[0].x);
  delete[] m;

  joinStates( boneMatrices_ );

  // initially calculate the bone matrices
  if(hasBones_) { update(0.0f); }
}

void BonesState::update(GLfloat dt)
{
  // ptr to bone matrix uniform
  Mat4f* boneMats = &boneMatrices_->valuePtr();
  for (register GLuint i=0; i < bones_.size(); i++)
  {
    boneMats[i] = bones_[i]->transformationMatrix();
  }
}

void BonesState::configureShader(ShaderConfiguration *cfg)
{
  State::configureShader(cfg);
  cfg->setHasBones(true);
}
