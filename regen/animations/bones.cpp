/*
 * bones.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include <regen/states/texture-state.h>

#include "bones.h"
using namespace regen;

Bones::Bones(list< ref_ptr<AnimationNode> > &bones, GLuint numBoneWeights)
: State(), Animation(GL_TRUE,GL_FALSE), bones_(bones)
{
  RenderState *rs = RenderState::get();
  GL_ERROR_LOG();
  bufferSize_ = sizeof(GLfloat)*16*bones_.size();
  ref_ptr<VertexBufferObject> vbo = ref_ptr<VertexBufferObject>::manage(
      new VertexBufferObject(VertexBufferObject::USAGE_TEXTURE));
  vboRef_ = vbo->alloc(bufferSize_);

  // attach vbo to texture
  rs->textureBuffer().push(vboRef_->bufferID());
  boneMatrixTex_ = ref_ptr<TextureBufferObject>::manage(new TextureBufferObject(GL_RGBA32F));
  boneMatrixTex_->begin(rs);
  boneMatrixTex_->attach(vbo, vboRef_);
  boneMatrixTex_->end(rs);
  rs->textureBuffer().pop();

  // and make the tbo available
  ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(
      new TextureState(boneMatrixTex_, "boneMatrices"));
  texState->set_mapping(TextureState::MAPPING_CUSTOM);
  texState->set_mapTo(TextureState::MAP_TO_CUSTOM);
  joinStates(texState);

  numBoneWeights_ = ref_ptr<ShaderInput1i>::manage(new ShaderInput1i("numBoneWeights"));
  numBoneWeights_->setUniformData(numBoneWeights);
  joinShaderInput(numBoneWeights_);

  boneMatrixData_ = new Mat4f[bones.size()];
  // prepend '#define HAS_BONES' to loaded shaders
  shaderDefine("HAS_BONES", "TRUE");

  // initially calculate the bone matrices
  glAnimate(rs, 0.0f);
}
Bones::~Bones()
{
  delete []boneMatrixData_;
}

GLint Bones::numBoneWeights() const
{
  return numBoneWeights_->getVertex1i(0);
}

void Bones::glAnimate(RenderState *rs, GLdouble dt)
{
  GL_ERROR_LOG();
  register GLuint i=0;
  for(list< ref_ptr<AnimationNode> >::iterator
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
