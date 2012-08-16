/*
 * bones-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "bones-state.h"

BonesState::BonesState(
    vector< ref_ptr<AnimationNode> > &bones,
    GLuint numBoneWeights)
: State(),
  bones_(bones),
  numBoneWeights_(numBoneWeights)
{
  // create and join bone matrix uniform
  boneMatrices_ = ref_ptr<ShaderInputMat4>::manage(
       new ShaderInputMat4("boneMatrices", bones.size()) );
  boneMatrices_->set_forceArray(GL_TRUE);

  Mat4f *m = new Mat4f[bones.size()];

  boneMatrices_->setInstanceData(1, 1, (byte*)m[0].x);
  delete[] m;

  joinShaderInput( ref_ptr<ShaderInput>::cast(boneMatrices_) );

  // initially calculate the bone matrices
  update(0.0f);
}

string BonesState::name()
{
  return "BonesState";
}

void BonesState::update(GLfloat dt)
{
  // ptr to bone matrix uniform
  Mat4f* boneMats = (Mat4f*)boneMatrices_->dataPtr();
  for (register GLuint i=0; i < bones_.size(); i++)
  {
    // the bone matrix is actually calculated in the animation thread
    // by NodeAnimation.
    boneMats[i] = bones_[i]->boneTransformationMatrix();
  }
}

void BonesState::configureShader(ShaderConfiguration *cfg)
{
  State::configureShader(cfg);
  cfg->setNumBoneWeights(numBoneWeights_);
}
