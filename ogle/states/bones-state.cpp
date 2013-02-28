/*
 * bones-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "bones-state.h"
#include <ogle/states/texture-state.h>

BonesState::BonesState(
    list< ref_ptr<AnimationNode> > &bones,
    GLuint numBoneWeights)
: State(),
  Animation(),
  bones_(bones)
{
  GLuint bufferSize = sizeof(GLfloat)*16*bones_.size();
  // vbo holding 4 rgba values for each bone matrix
  boneMatrixVBO_ = ref_ptr<VertexBufferObject>::manage(
      new VertexBufferObject(VertexBufferObject::USAGE_STATIC, bufferSize, GL_TEXTURE_BUFFER));
  // attach vbo to tbo
  ref_ptr<TextureBufferObject> tex = ref_ptr<TextureBufferObject>::manage(
      new TextureBufferObject(GL_RGBA32F));
  tex->bind();
  tex->attach(boneMatrixVBO_);
  // and make the tbo available
  ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(tex)));
  texState->set_name("boneMatrices");
  texState->set_mapping(MAPPING_CUSTOM);
  texState->setMapTo(MAP_TO_CUSTOM);
  joinStates(ref_ptr<State>::cast(texState));

  numBoneWeights_ = ref_ptr<ShaderInput1i>::manage(new ShaderInput1i("numBoneWeights"));
  numBoneWeights_->setUniformData(numBoneWeights);
  joinShaderInput(ref_ptr<ShaderInput>::cast(numBoneWeights_));

  boneMatrixData_ = new Mat4f[bones.size()];
  // prepend '#define HAS_BONES' to loaded shaders
  shaderDefine("HAS_BONES", "TRUE");

  // initially calculate the bone matrices
  glAnimate(NULL, 0.0f);
}
BonesState::~BonesState()
{
  delete []boneMatrixData_;
}

GLint BonesState::numBoneWeights() const
{
  return numBoneWeights_->getVertex1i(0);
}


void BonesState::animate(GLdouble dt) {}
void BonesState::glAnimate(RenderState *rs, GLdouble dt) {
  register GLuint i=0;
  for(list< ref_ptr<AnimationNode> >::iterator
      it=bones_.begin(); it!=bones_.end(); ++it)
  {
    // the bone matrix is actually calculated in the animation thread
    // by NodeAnimation.
    boneMatrixData_[i] = (*it)->boneTransformationMatrix();
    ++i;
  }
  boneMatrixVBO_->bind(GL_TEXTURE_BUFFER);
  boneMatrixVBO_->set_data(boneMatrixVBO_->bufferSize(), boneMatrixData_);
}
GLboolean BonesState::useAnimation() const {
  return GL_FALSE;
}
GLboolean BonesState::useGLAnimation() const {
  return GL_TRUE;
}
