/*
 * particles.cpp
 *
 *  Created on: 03.11.2012
 *      Author: daniel
 */

#include <regen/utility/string-util.h>
#include <regen/states/blend-state.h>
#include <regen/states/depth-state.h>
#include <regen/gl-types/gl-enum.h>
#include "particles.h"
using namespace regen;

///////////

Particles::Particles(GLuint numParticles, BlendMode blendMode)
: Mesh(GL_POINTS,VertexBufferObject::USAGE_STREAM), Animation(GL_TRUE,GL_FALSE)
{
  // enable blending
  joinStates(ref_ptr<State>::manage(new BlendState(blendMode)));
  init(numParticles);
}
Particles::Particles(GLuint numParticles)
: Mesh(GL_POINTS,VertexBufferObject::USAGE_STREAM), Animation(GL_TRUE,GL_FALSE)
{
  init(numParticles);
}

void Particles::init(GLuint numParticles)
{
  feedbackBuffer_ = ref_ptr<VertexBufferObject>::manage(
      new VertexBufferObject(VertexBufferObject::USAGE_FEEDBACK));
  inputBuffer_ = inputContainer_->inputBuffer();

  // do not write depth values
  ref_ptr<DepthState> depth = ref_ptr<DepthState>::manage(new DepthState);
  depth->set_useDepthWrite(GL_FALSE);
  joinStates(depth);

  inputContainer_->set_numVertices(numParticles);

  {
    softScale_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("softParticleScale"));
    softScale_->setUniformData(30.0);
    setInput(softScale_);

    gravity_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("gravity"));
    gravity_->setUniformData(Vec3f(0.0,-9.81,0.0));
    setInput(gravity_);

    brightness_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("particleBrightness"));
    brightness_->setUniformData(0.4);
    setInput(brightness_);

    dampingFactor_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("dampingFactor"));
    dampingFactor_->setUniformData(2.5);
    setInput(dampingFactor_);

    noiseFactor_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("noiseFactor"));
    noiseFactor_->setUniformData(0.5);
    setInput(noiseFactor_);
  }

  maxNumParticleEmits_ = ref_ptr<ShaderInput1i>::manage(new ShaderInput1i("maxNumParticleEmits"));
  maxNumParticleEmits_->setUniformData(1);
  //setInput(ref_ptr<ShaderInput>::cast(maxNumParticleEmits_));

  {
    srand(time(0));
    // get a random seed for each particle
    GLuint *initialSeedData = new GLuint[numParticles];
    for(GLuint i=0u; i<numParticles; ++i) initialSeedData[i] = rand();
    ref_ptr<ShaderInput1ui> randomSeed_ = ref_ptr<ShaderInput1ui>::manage(new ShaderInput1ui("randomSeed"));
    randomSeed_->setVertexData(numParticles, (byte*)initialSeedData);
    addParticleAttribute(randomSeed_);
    delete []initialSeedData;

    // initially set lifetime to zero so that particles
    // get emitted in the first step
    GLfloat *zeroLifetimeData = new GLfloat[numParticles];
    for(GLuint i=0u; i<numParticles; ++i) zeroLifetimeData[i] = -1.0;
    lifetimeInput_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("lifetime"));
    lifetimeInput_->setVertexData(numParticles, (byte*)zeroLifetimeData);
    addParticleAttribute(lifetimeInput_);
    delete []zeroLifetimeData;
  }

  updateShaderState_ = ref_ptr<ShaderState>::manage(new ShaderState);
  drawShaderState_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(drawShaderState_);

  vaoFeedback_ = ref_ptr<VertexArrayObject>::manage(new VertexArrayObject);
  vao_ = ref_ptr<VertexArrayObject>::manage(new VertexArrayObject);

  set_softParticles(GL_TRUE);
  set_isShadowReceiver(GL_TRUE);
}

void Particles::set_isShadowReceiver(GLboolean v)
{
  shaderDefine("IS_SHADOW_RECEIVER", v?"TRUE":"FALSE");
}
void Particles::set_softParticles(GLboolean v)
{
  shaderDefine("USE_SOFT_PARTICLES", v?"TRUE":"FALSE");
}
void Particles::set_nearCameraSoftParticles(GLboolean v)
{
  shaderDefine("USE_NEAR_CAMERA_SOFT_PARTICLES", v?"TRUE":"FALSE");
}

void Particles::addParticleAttribute(const ref_ptr<ShaderInput> &in)
{
  setInput(in);
  attributes_.push_front(in);
  // add shader defines for attribute
  GLuint counter = attributes_.size()-1;
  shaderDefine(
      REGEN_STRING("PARTICLE_ATTRIBUTE"<<counter<<"_TYPE"),
      GLEnum::glslDataType(in->dataType(), in->valsPerElement()) );
  shaderDefine(
      REGEN_STRING("PARTICLE_ATTRIBUTE"<<counter<<"_NAME"),
      in->name() );
  REGEN_DEBUG("particle attribute " << in->name() << " added.");
}

void Particles::set_depthTexture(const ref_ptr<Texture> &tex)
{
  if(depthTexture_.get()!=NULL) {
    disjoinStates(depthTexture_);
  }
  depthTexture_ = ref_ptr<TextureState>::manage(new TextureState(tex,"depthTexture"));
  joinStatesFront(depthTexture_);
}

void Particles::updateVAO(ref_ptr<VertexArrayObject> &vao, VBOReference &ref)
{
  GLuint currOffset = ref->address();

  RenderState::get()->vao().push(vao->id());

  glBindBuffer(GL_ARRAY_BUFFER, ref->bufferID());
  // note:setInput adds attribute to front of list.
  for(list< ref_ptr<ShaderInput> >::const_iterator
      it=attributes_.begin(); it!=attributes_.end(); ++it)
  {
    ShaderInput *att = it->get();
    att->set_buffer(ref->bufferID(), ref);
    att->set_offset(currOffset);
    currOffset += att->elementSize();

    GLint loc = updateShaderState_->shader()->attributeLocation(att->name());
    if(loc!=-1) att->enableAttribute(loc);
  }

  RenderState::get()->vao().pop();
}

void Particles::createBuffer()
{
  GLuint bufferSize = VertexBufferObject::attributeSize(attributes_);
  feedbackRef_ = feedbackBuffer_->alloc(bufferSize);
  particleRef_ = inputBuffer_->allocInterleaved(attributes_);
  shaderDefine("NUM_PARTICLE_ATTRIBUTES", REGEN_STRING(attributes_.size()));
  bufferRange_.size_ = bufferSize;
  REGEN_DEBUG("particle buffers created size="<<bufferSize<<".");

  if(drawShaderState_->shader().get()) {
    updateVAO(vaoFeedback_, feedbackRef_);
    updateVAO(vao_, particleRef_);
  }
}

void Particles::createShader(
    StateConfig &shaderCfg,
    const string &updateKey, const string &drawKey)
{
  shaderCfg.feedbackAttributes_.clear();
  for(list< ref_ptr<ShaderInput> >::const_iterator
      it=attributes_.begin(); it!=attributes_.end(); ++it)
  {
    shaderCfg.feedbackAttributes_.push_back((*it)->name());
  }
  shaderCfg.feedbackMode_ = GL_INTERLEAVED_ATTRIBS;
  shaderCfg.feedbackStage_ = GL_VERTEX_SHADER;
  updateShaderState_->createShader(shaderCfg, updateKey);
  shaderCfg.feedbackAttributes_.clear();

  drawShaderState_->createShader(shaderCfg, drawKey);

  if(feedbackRef_.get()) {
    updateVAO(vaoFeedback_, feedbackRef_);
    updateVAO(vao_, particleRef_);
  }
}

void Particles::glAnimate(RenderState *rs, GLdouble dt)
{
  GL_ERROR_LOG();

  rs->toggles().push(RenderState::RASTARIZER_DISCARD, GL_TRUE);
  updateShaderState_->enable(rs);
  rs->vao().push(vao_->id());

  bufferRange_.buffer_ = feedbackRef_->bufferID();
  bufferRange_.offset_ = feedbackRef_->address();
  rs->feedbackBufferRange().push(0, bufferRange_);
  rs->beginTransformFeedback(feedbackPrimitive_);

  glDrawArrays(primitive_, 0, inputContainer_->numVertices());

  rs->endTransformFeedback();
  rs->feedbackBufferRange().pop(0);

  rs->vao().pop();
  updateShaderState_->disable(rs);
  rs->toggles().pop(RenderState::RASTARIZER_DISCARD);

  // ping pong buffers
  {
    ref_ptr<VertexBufferObject> buf = inputBuffer_;
    inputBuffer_ = feedbackBuffer_;
    feedbackBuffer_ = buf;
  }
  {
    ref_ptr<VertexArrayObject> buf = vao_;
    vao_ = vaoFeedback_;
    vaoFeedback_ = buf;
  }
  {
    VBOReference buf = particleRef_;
    particleRef_ = feedbackRef_;
    feedbackRef_ = buf;
  }
  // update particle attribute offset
  GLuint currOffset = bufferRange_.offset_;
  for(list< ref_ptr<ShaderInput> >::const_iterator
      it=attributes_.begin(); it!=attributes_.end(); ++it)
  {
    ref_ptr<ShaderInput> att = *it;
    att->set_buffer(bufferRange_.buffer_, particleRef_);
    att->set_offset(currOffset);
    currOffset += att->elementSize();
  }
  GL_ERROR_LOG();
}

const ref_ptr<ShaderInput1f>& Particles::softScale() const
{ return softScale_; }
const ref_ptr<ShaderInput3f>& Particles::gravity() const
{ return gravity_; }
const ref_ptr<ShaderInput1f>& Particles::dampingFactor() const
{ return dampingFactor_; }
const ref_ptr<ShaderInput1f>& Particles::noiseFactor() const
{ return noiseFactor_; }
const ref_ptr<ShaderInput1f>& Particles::brightness() const
{ return brightness_; }
const ref_ptr<ShaderInput1i>& Particles::maxNumParticleEmits() const
{ return maxNumParticleEmits_; }
