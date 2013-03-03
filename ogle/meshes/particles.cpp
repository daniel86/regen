/*
 * particles.cpp
 *
 *  Created on: 03.11.2012
 *      Author: daniel
 */

#include <ogle/utility/string-util.h>
#include <ogle/states/blend-state.h>
#include <ogle/states/depth-state.h>
#include <ogle/gl-types/gl-enum.h>
#include "particles.h"

///////////

ParticleState::ParticleState(GLuint numParticles, BlendMode blendMode)
: MeshState(GL_POINTS)
{
  // enable blending
  joinStates(ref_ptr<State>::manage(new BlendState(blendMode)));
  init(numParticles);
}

ParticleState::ParticleState(GLuint numParticles)
: MeshState(GL_POINTS)
{
  init(numParticles);
}

void ParticleState::init(GLuint numParticles)
{
  set_useVBOManager(GL_FALSE);

  // do not write depth values
  ref_ptr<DepthState> depth = ref_ptr<DepthState>::manage(new DepthState);
  depth->set_useDepthWrite(GL_FALSE);
  joinStates(ref_ptr<State>::cast(depth));

  numVertices_ = numParticles;

  {
    softScale_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("softParticleScale"));
    softScale_->setUniformData(30.0);
    setInput(ref_ptr<ShaderInput>::cast(softScale_));

    gravity_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("gravity"));
    gravity_->setUniformData(Vec3f(0.0,-9.81,0.0));
    setInput(ref_ptr<ShaderInput>::cast(gravity_));

    brightness_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("particleBrightness"));
    brightness_->setUniformData(0.4);
    setInput(ref_ptr<ShaderInput>::cast(brightness_));

    dampingFactor_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("dampingFactor"));
    dampingFactor_->setUniformData(2.5);
    setInput(ref_ptr<ShaderInput>::cast(dampingFactor_));

    noiseFactor_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("noiseFactor"));
    noiseFactor_->setUniformData(0.5);
    setInput(ref_ptr<ShaderInput>::cast(noiseFactor_));
  }

  maxNumParticleEmits_ = ref_ptr<ShaderInput1i>::manage(new ShaderInput1i("maxNumParticleEmits"));
  maxNumParticleEmits_->setUniformData(1);
  //setInput(ref_ptr<ShaderInput>::cast(maxNumParticleEmits_));

  {
    // get a random seed for each particle
    srand(time(0));
    GLuint initialSeedData[numParticles];
    for(GLuint i=0u; i<numParticles; ++i) initialSeedData[i] = rand();
    ref_ptr<ShaderInput1ui> randomSeed_ = ref_ptr<ShaderInput1ui>::manage(new ShaderInput1ui("randomSeed"));
    randomSeed_->setVertexData(numParticles, (byte*)initialSeedData);
    addParticleAttribute(ref_ptr<ShaderInput>::cast(randomSeed_));

    // initially set lifetime to zero so that particles
    // get emitted in the first step
    GLfloat zeroLifetimeData[numParticles];
    memset(zeroLifetimeData, 0, sizeof(zeroLifetimeData));
    lifetimeInput_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("lifetime"));
    lifetimeInput_->setVertexData(numParticles, (byte*)zeroLifetimeData);
    addParticleAttribute(ref_ptr<ShaderInput>::cast(lifetimeInput_));
  }

  updateShaderState_ = ref_ptr<ShaderState>::manage(new ShaderState);
  drawShaderState_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(drawShaderState_));

  set_isShadowReceiver(GL_TRUE);
  set_softParticles(GL_TRUE);
  set_isShadowReceiver(GL_TRUE);
}

void ParticleState::set_isShadowReceiver(GLboolean v)
{
  shaderDefine("IS_SHADOW_RECEIVER", v?"TRUE":"FALSE");
}
void ParticleState::set_softParticles(GLboolean v)
{
  shaderDefine("USE_SOFT_PARTICLES", v?"TRUE":"FALSE");
}
void ParticleState::set_nearCameraSoftParticles(GLboolean v)
{
  shaderDefine("USE_NEAR_CAMERA_SOFT_PARTICLES", v?"TRUE":"FALSE");
}

void ParticleState::addParticleAttribute(const ref_ptr<ShaderInput> &in)
{
  setInput(in);
  attributes_.push_back(ref_ptr<VertexAttribute>::cast(in));
  // add shader defines for attribute
  GLuint counter = attributes_.size()-1;
  shaderDefine(
      FORMAT_STRING("PARTICLE_ATTRIBUTE"<<counter<<"_TYPE"),
      glslDataType(in->dataType(), in->valsPerElement()) );
  shaderDefine(
      FORMAT_STRING("PARTICLE_ATTRIBUTE"<<counter<<"_NAME"),
      in->name() );
}

void ParticleState::set_depthTexture(const ref_ptr<Texture> &tex)
{
  if(depthTexture_.get()!=NULL) {
    disjoinStates(ref_ptr<State>::cast(depthTexture_));
  }
  depthTexture_ = ref_ptr<TextureState>::manage(new TextureState(tex,"depthTexture"));
  joinStates(ref_ptr<State>::cast(depthTexture_));
}

void ParticleState::createBuffer()
{
  feedbackBuffer_ = ref_ptr<VertexBufferObject>::manage(new VertexBufferObject(
      VertexBufferObject::USAGE_STREAM, VertexBufferObject::attributeStructSize(attributes_)));
  particleBuffer_ = ref_ptr<VertexBufferObject>::manage(
      new VertexBufferObject(VertexBufferObject::USAGE_DYNAMIC, feedbackBuffer_->bufferSize()));
  particleBuffer_->allocateInterleaved(attributes_);
  shaderDefine("NUM_PARTICLE_ATTRIBUTES", FORMAT_STRING(attributes_.size()));
}

void ParticleState::createShader(ShaderConfig &shaderCfg, const string &updateKey, const string &drawKey)
{
  shaderCfg.feedbackAttributes_.clear();
  for(list< ref_ptr<VertexAttribute> >::const_iterator
      it=attributes_.begin(); it!=attributes_.end(); ++it)
  {
    shaderCfg.feedbackAttributes_.push_back((*it)->name());
  }
  shaderCfg.feedbackMode_ = GL_INTERLEAVED_ATTRIBS;
  shaderCfg.feedbackStage_ = GL_VERTEX_SHADER;
  updateShaderState_->createShader(shaderCfg, updateKey);
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
const ref_ptr<ShaderInput1f>& ParticleState::brightness() const
{
  return brightness_;
}
const ref_ptr<ShaderInput1i>& ParticleState::maxNumParticleEmits() const
{
  return maxNumParticleEmits_;
}

void ParticleState::update(RenderState *rs, GLdouble dt)
{
  if(rs->isTransformFeedbackAcive()) {
    WARN_LOG("Transform Feedback was active when the Particles were updated.");
    return;
  }

  ref_ptr<Shader> shaderBuf = drawShaderState_->shader();
  drawShaderState_->set_shader(updateShaderState_->shader());
  updateShaderState_->set_shader(shaderBuf);
  enable(rs);

  glBindBufferRange(
      GL_TRANSFORM_FEEDBACK_BUFFER,
      0, feedbackBuffer_->id(),
      0, feedbackBuffer_->bufferSize());

  rs->beginTransformFeedback(feedbackPrimitive_);
  rs->toggles().push(RenderState::RASTARIZER_DISCARD, GL_TRUE);

  glDrawArrays(primitive_, 0, numVertices_);

  rs->toggles().pop(RenderState::RASTARIZER_DISCARD);
  rs->endTransformFeedback();

  glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);

  disable(rs);
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
