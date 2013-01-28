/*
 * particles.cpp
 *
 *  Created on: 03.11.2012
 *      Author: daniel
 */

#include <ogle/utility/string-util.h>
#include <ogle/states/render-state.h>
#include "particles.h"

///////////

ParticleState::ParticleState(GLuint numParticles)
: MeshState(GL_POINTS),
  Animation()
{
  set_useVBOManager(GL_FALSE);
  //set_feedbackMode(GL_INTERLEAVED_ATTRIBS);
  //set_feedbackStage(GL_VERTEX_SHADER);

  numVertices_ = numParticles;

  softScale_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("softParticleScale"));
  softScale_->setUniformData(30.0);
  setInput(ref_ptr<ShaderInput>::cast(softScale_));

  gravity_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("gravity"));
  gravity_->setUniformData(Vec3f(0.0,-9.81,0.0));
  setInput(ref_ptr<ShaderInput>::cast(gravity_));

  dampingFactor_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("dampingFactor"));
  dampingFactor_->setUniformData(2.5);
  setInput(ref_ptr<ShaderInput>::cast(dampingFactor_));

  noiseFactor_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("noiseFactor"));
  noiseFactor_->setUniformData(0.5);
  setInput(ref_ptr<ShaderInput>::cast(noiseFactor_));

  updateShaderState_ = ref_ptr<ShaderState>::manage(new ShaderState);
  drawShaderState_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(drawShaderState_));
}

void ParticleState::addParticleAttribute(const ref_ptr<ShaderInput> &in)
{
  setInput(in);
  attributes_.push_back(ref_ptr<VertexAttribute>::cast(in));
  // add shader defines for attribute
  GLuint counter = attributes_.size()-1;
  shaderDefine(
      FORMAT_STRING("PARTICLE_ATTRIBUTE"<<counter<<"_TYPE"),
      in->shaderDataType() );
  shaderDefine(
      FORMAT_STRING("PARTICLE_ATTRIBUTE"<<counter<<"_NAME"),
      in->name() );
}

void ParticleState::createBuffer()
{
  feedbackBuffer_ = ref_ptr<VertexBufferObject>::manage(new VertexBufferObject(
      VertexBufferObject::USAGE_STREAM, VertexBufferObject::attributeStructSize(attributes_)));
  particleBuffer_ = ref_ptr<VertexBufferObject>::manage(
      new VertexBufferObject(VertexBufferObject::USAGE_DYNAMIC, feedbackBuffer_->bufferSize()));
  VBOBlockIterator bufferIt = particleBuffer_->allocateInterleaved(attributes_);
  // XXX: not needed ?
  for(list< ref_ptr<VertexAttribute> >::const_iterator
      it=attributes_.begin(); it!=attributes_.end(); ++it)
  {
    ref_ptr<VertexAttribute> in = *it;
    in->set_bufferIterator(bufferIt);
  }

  shaderDefine("NUM_PARTICLE_ATTRIBUTES", FORMAT_STRING(attributes_.size()));
}

void ParticleState::createShader(ShaderConfig &shaderCfg, const string &updateKey, const string &drawKey)
{
  shaderCfg.defines_["HAS_GEOMETRY_SHADER"] = "FALSE";
  shaderCfg.defines_["HAS_FRAGMENT_SHADER"] = "FALSE";
  shaderCfg.feedbackAttributes_.clear();
  for(list< ref_ptr<VertexAttribute> >::const_iterator
      it=attributes_.begin(); it!=attributes_.end(); ++it)
  {
    shaderCfg.feedbackAttributes_.push_back((*it)->name());
  }
  shaderCfg.feedbackMode_ = GL_INTERLEAVED_ATTRIBS;
  shaderCfg.feedbackStage_ = GL_VERTEX_SHADER;
  updateShaderState_->createShader(shaderCfg, updateKey);

  shaderCfg.defines_["HAS_GEOMETRY_SHADER"] = "TRUE";
  shaderCfg.defines_["HAS_FRAGMENT_SHADER"] = "TRUE";
  shaderCfg.feedbackAttributes_.clear();
  drawShaderState_->createShader(shaderCfg, drawKey);
}

const ref_ptr<ShaderInput1f>& ParticleState::softScale() const
{
  return softScale_;
}
const ref_ptr<ShaderInput3f>& ParticleState::gravity() const
{
  return gravity_;
}
const ref_ptr<ShaderInput1f>& ParticleState::dampingFactor() const
{
  return dampingFactor_;
}
const ref_ptr<ShaderInput1f>& ParticleState::noiseFactor() const
{
  return noiseFactor_;
}

GLboolean ParticleState::useGLAnimation() const
{
  return GL_TRUE;
}
GLboolean ParticleState::useAnimation() const
{
  return GL_FALSE;
}
void ParticleState::animate(GLdouble dt)
{
}
void ParticleState::glAnimate(GLdouble dt)
{
  RenderState rs;

  ref_ptr<Shader> shaderBuf = drawShaderState_->shader();
  drawShaderState_->set_shader(updateShaderState_->shader());
  updateShaderState_->set_shader(shaderBuf);
  enable(&rs);

  glEnable(GL_RASTERIZER_DISCARD);
  glBindBufferRange(
      GL_TRANSFORM_FEEDBACK_BUFFER,
      0, feedbackBuffer_->id(),
      0, feedbackBuffer_->bufferSize());
  glBeginTransformFeedback(feedbackPrimitive_);

  glDrawArrays(primitive_, 0, numVertices_);

  glEndTransformFeedback();
  glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
  glDisable(GL_RASTERIZER_DISCARD);

  disable(&rs);
  updateShaderState_->set_shader(drawShaderState_->shader());
  drawShaderState_->set_shader(shaderBuf);

  // ping pong buffers
  ref_ptr<VertexBufferObject> buf = particleBuffer_;
  particleBuffer_ = feedbackBuffer_;
  feedbackBuffer_ = buf;
  for(list< ref_ptr<VertexAttribute> >::const_iterator
      it=attributes_.begin(); it!=attributes_.end(); ++it)
  {
    const ref_ptr<VertexAttribute> &att = *it;
    att->set_buffer(particleBuffer_->id());
  }
}
