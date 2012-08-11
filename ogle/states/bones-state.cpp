/*
 * bones-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "bones-state.h"

BonesState::BonesState(vector< ref_ptr<AnimationNode> > &bones, GLuint numBoneWeights)
: State(),
  bones_(bones),
  numBoneWeights_(numBoneWeights)
{
  // create and join bone matrix uniform
  boneMatrices_ = ref_ptr<UniformMat4>::manage(
       new UniformMat4("boneMatrices", bones.size()) );

  Mat4f *m = new Mat4f[bones.size()];
  boneMatrices_->set_value((Mat4f*) m[0].x);
  delete[] m;

  joinUniform( ref_ptr<Uniform>::cast(boneMatrices_) );

  // initially calculate the bone matrices
  update(0.0f);
}

void BonesState::update(GLfloat dt)
{
  // ptr to bone matrix uniform
  Mat4f* boneMats = &boneMatrices_->valuePtr();
  for (register GLuint i=0; i < bones_.size(); i++)
  {
    boneMats[i] = bones_[i]->boneTransformationMatrix();
  }
}

void BonesState::configureShader(ShaderConfiguration *cfg)
{
  State::configureShader(cfg);
  cfg->setNumBoneWeights(numBoneWeights_);
}
