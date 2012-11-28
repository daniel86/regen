/*
 * bones-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "bones-state.h"
#include <ogle/states/render-state.h>

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

  cerr << "BONES " << bones.size() << " , " << numBoneWeights_ << endl;
  joinShaderInput( ref_ptr<ShaderInput>::cast(boneMatrices_) );

  // initially calculate the bone matrices
  updateBoneMatrices();
}

void BonesState::updateBoneMatrices()
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

void BonesState::enable(RenderState *rs)
{
  updateBoneMatrices();
  Mat4f* boneMats = (Mat4f*)boneMatrices_->dataPtr();
  lastBoneMatrices_ = rs->boneMatrices();
  lastBoneWeights_ = rs->boneWeightCount();
  lastBoneCount_ = rs->boneCount();
  rs->set_boneMatrices(boneMats, numBoneWeights_, bones_.size());
  State::enable(rs);
}
void BonesState::disable(RenderState *rs)
{
  rs->set_boneMatrices(lastBoneMatrices_,lastBoneWeights_,lastBoneCount_);
  State::enable(rs);
}

void BonesState::configureShader(ShaderConfig *cfg)
{
  State::configureShader(cfg);
  cfg->setNumBoneWeights(numBoneWeights_, boneMatrices_->elementCount());
}
