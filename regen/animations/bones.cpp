/*
 * bones.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include <regen/states/texture-state.h>

#include "bones.h"
using namespace regen;

Bones::Bones(GLuint numBoneWeights)
: HasInputState(VBO::USAGE_TEXTURE),
  Animation(GL_TRUE,GL_FALSE)
{
  bufferSize_ = 0u;

  numBoneWeights_ = ref_ptr<ShaderInput1i>::alloc("numBoneWeights");
  numBoneWeights_->setUniformData(numBoneWeights);
  joinShaderInput(numBoneWeights_);

  boneMatrixData_ = NULL;
  // prepend '#define HAS_BONES' to loaded shaders
  shaderDefine("HAS_BONES", "TRUE");
}
Bones::~Bones()
{
  if(boneMatrixData_!=NULL) delete []boneMatrixData_;
}

GLint Bones::numBoneWeights() const
{ return numBoneWeights_->getVertex1i(0); }

void Bones::setBones(const list< ref_ptr<AnimationNode> > &bones)
{
  GL_ERROR_LOG();
  RenderState *rs = RenderState::get();
  bones_ = bones;

  if(boneMatrixData_!=NULL) delete []boneMatrixData_;
  boneMatrixData_ = new Mat4f[bones_.size()];

  bufferSize_ = sizeof(GLfloat)*16*bones_.size();
  vboRef_ = inputContainer_->inputBuffer()->alloc(bufferSize_);

  // attach vbo to texture
  rs->textureBuffer().push(vboRef_->bufferID());
  boneMatrixTex_ = ref_ptr<TextureBuffer>::alloc(GL_RGBA32F);
  boneMatrixTex_->begin(rs);
  boneMatrixTex_->attach(inputContainer_->inputBuffer(), vboRef_);
  boneMatrixTex_->end(rs);
  rs->textureBuffer().pop();

  // and make the tbo available
  if(texState_.get()) disjoinStates(texState_);
  texState_ = ref_ptr<TextureState>::alloc(boneMatrixTex_, "boneMatrices");
  texState_->set_mapping(TextureState::MAPPING_CUSTOM);
  texState_->set_mapTo(TextureState::MAP_TO_CUSTOM);
  joinStates(texState_);

  GL_ERROR_LOG();

  // initially calculate the bone matrices
  glAnimate(rs, 0.0f);
}

void Bones::glAnimate(RenderState *rs, GLdouble dt)
{
  GL_ERROR_LOG();
  if(bufferSize_<=0) return;

  register GLuint i=0;
  for(list< ref_ptr<AnimationNode> >::const_iterator
      it=bones_.begin(); it!=bones_.end(); ++it)
  {
    // the bone matrix is actually calculated in the animation thread
    // by NodeAnimation.
    boneMatrixData_[i] = (*it)->boneTransformationMatrix();
    i += 1;
  }

  rs->textureBuffer().push(vboRef_->bufferID());
  glBufferSubData(GL_TEXTURE_BUFFER,
      vboRef_->address(), bufferSize_, &boneMatrixData_[0].x);
  rs->textureBuffer().pop();
  GL_ERROR_LOG();
}
