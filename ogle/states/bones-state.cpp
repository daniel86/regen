/*
 * bones-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "bones-state.h"
#include <ogle/states/render-state.h>
#include <ogle/states/texture-state.h>

// FIXME: only one tbo used for dwarf model.
//  axe is transformed by first body matrix.
// #define USE_BONE_TBO

BonesState::BonesState(
    vector< ref_ptr<AnimationNode> > &bones,
    GLuint numBoneWeights)
: State(),
  Animation(),
  bones_(bones),
  numBoneWeights_(numBoneWeights)
{
#ifdef USE_BONE_TBO
  GLuint bufferSize = sizeof(GLfloat)*16*bones_.size();
  // vbo holding 4 rgba values for each bone matrix
  boneMatrixVBO_ = 0u;
  glGenBuffers(1, &boneMatrixVBO_);
  glBindBuffer(GL_TEXTURE_BUFFER_EXT, boneMatrixVBO_);
  glBufferData(GL_TEXTURE_BUFFER_EXT, bufferSize, 0, GL_STATIC_DRAW);
  // attach vbo to tbo
  boneMatrixTBO_ = ref_ptr<TextureBufferObject>::manage(
      new TextureBufferObject(GL_RGBA32F));
  // and make the tbo available
  ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(boneMatrixTBO_)));
  texState->set_name("boneMatrices");
  texState->set_mapping(MAPPING_CUSTOM);
  texState->setMapTo(MAP_TO_CUSTOM);
  joinStates(ref_ptr<State>::cast(texState));

  boneMatrixData_ = new Mat4f[bones.size()];
#else
  // create and join bone matrix uniform
  boneMatrices_ = ref_ptr<ShaderInputMat4>::manage(
       new ShaderInputMat4("boneMatrices", bones.size()) );
  boneMatrices_->set_forceArray(GL_TRUE);
  Mat4f *m = new Mat4f[bones.size()];
  boneMatrices_->setInstanceData(1, 1, (byte*)m[0].x);
  delete[] m;
  joinShaderInput( ref_ptr<ShaderInput>::cast(boneMatrices_) );
#endif

#ifdef USE_BONE_TBO
  shaderDefine("USE_BONE_TBO", "TRUE");
#endif

  // initially calculate the bone matrices
  updateGraphics(0.0f);
#ifdef USE_BONE_TBO
  boneMatrixTBO_->bind();
  boneMatrixTBO_->attach(boneMatrixVBO_);
#endif
}
BonesState::~BonesState()
{
  delete []boneMatrixData_;
}

void BonesState::animate(GLdouble dt)
{
}
void BonesState::updateGraphics(GLdouble dt)
{
  // ptr to bone matrix uniform
#ifndef USE_BONE_TBO
  Mat4f* boneMatrixData_ = (Mat4f*)boneMatrices_->dataPtr();
#endif
  for (register GLuint i=0; i < bones_.size(); ++i) {
    // the bone matrix is actually calculated in the animation thread
    // by NodeAnimation.
    boneMatrixData_[i] = bones_[i]->boneTransformationMatrix();
  }
#ifdef USE_BONE_TBO
  GLuint bufferSize = sizeof(GLfloat)*16*bones_.size();
  glBindBuffer(GL_TEXTURE_BUFFER, boneMatrixVBO_);
  glBufferData(GL_TEXTURE_BUFFER, bufferSize, boneMatrixData_, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_TEXTURE_BUFFER, 0);
#endif
}

void BonesState::enable(RenderState *rs)
{
#ifndef USE_BONE_TBO
  Mat4f* boneMatrixData_ = (Mat4f*)boneMatrices_->dataPtr();
#endif
  lastBoneMatrices_ = rs->boneMatrices();
  lastBoneWeights_ = rs->boneWeightCount();
  lastBoneCount_ = rs->boneCount();
  rs->set_boneMatrices(boneMatrixData_, numBoneWeights_, bones_.size());
  State::enable(rs);
}
void BonesState::disable(RenderState *rs)
{
  rs->set_boneMatrices(lastBoneMatrices_,lastBoneWeights_,lastBoneCount_);
  State::disable(rs);
}

void BonesState::configureShader(ShaderConfig *cfg)
{
  State::configureShader(cfg);
  cfg->setNumBoneWeights(numBoneWeights_, bones_.size());
}
