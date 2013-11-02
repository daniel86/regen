/*
 * particles.cpp
 *
 *  Created on: 03.11.2012
 *      Author: daniel
 */

#include <regen/utility/string-util.h>
#include <regen/states/blend-state.h>
#include <regen/states/depth-state.h>
#include <regen/states/state-configurer.h>
#include <regen/gl-types/gl-enum.h>
#include "particles.h"
using namespace regen;

///////////

Particles::Particles(GLuint numParticles, const string &updateShaderKey)
: Mesh(GL_POINTS,VBO::USAGE_STREAM),
  Animation(GL_TRUE,GL_FALSE),
  updateShaderKey_(updateShaderKey)
{
  feedbackBuffer_ = ref_ptr<VBO>::alloc(VBO::USAGE_FEEDBACK);
  inputBuffer_ = inputContainer_->inputBuffer();
  inputContainer_->set_numVertices(numParticles);

  deltaT_ = ref_ptr<ShaderInput1f>::alloc("deltaT");
  deltaT_->setUniformData(0.0);
  setInput(deltaT_);

  gravity_ = ref_ptr<ShaderInput3f>::alloc("gravity");
  gravity_->setUniformData(Vec3f(0.0,-9.81,0.0));
  setInput(gravity_);

  dampingFactor_ = ref_ptr<ShaderInput1f>::alloc("dampingFactor");
  dampingFactor_->setUniformData(2.5);
  setInput(dampingFactor_);

  noiseFactor_ = ref_ptr<ShaderInput1f>::alloc("noiseFactor");
  noiseFactor_->setUniformData(0.5);
  setInput(noiseFactor_);

  {
    srand(time(0));
    // get a random seed for each particle
    ref_ptr<ShaderInput1ui> randomSeed_ = ref_ptr<ShaderInput1ui>::alloc("randomSeed");
    randomSeed_->setVertexData(numParticles, NULL);
    for(GLuint i=0u; i<numParticles; ++i) {
      randomSeed_->setVertex(i, rand());
    }
    setInput(randomSeed_);

    // initially set lifetime to zero so that particles
    // get emitted in the first step
    lifetimeInput_ = ref_ptr<ShaderInput1f>::alloc("lifetime");
    lifetimeInput_->setVertexData(numParticles, NULL);
    for(GLuint i=0u; i<numParticles; ++i) {
      lifetimeInput_->setVertex(i, -1.0);
    }
    setInput(lifetimeInput_);
  }

  updateState_ = ref_ptr<ShaderState>::alloc();
  vaoFeedback_ = ref_ptr<VAO>::alloc();
  vao_ = ref_ptr<VAO>::alloc();
}

ShaderInputList::const_iterator Particles::setInput(
    const ref_ptr<ShaderInput> &in, const string &name)
{
  if(in->isVertexAttribute()) {
    attributes_.push_front(in);
    // add shader defines for attribute
    GLuint counter = attributes_.size()-1;
    shaderDefine(
        REGEN_STRING("PARTICLE_ATTRIBUTE"<<counter<<"_TYPE"),
        glenum::glslDataType(in->dataType(), in->valsPerElement()) );
    shaderDefine(
        REGEN_STRING("PARTICLE_ATTRIBUTE"<<counter<<"_NAME"),
        in->name() );
    REGEN_DEBUG("Particle attribute '" << in->name() << "' added.");
  }
  return Mesh::setInput(in);
}

void Particles::createBuffer()
{
  GLuint bufferSize = VBO::attributeSize(attributes_);
  feedbackRef_ = feedbackBuffer_->alloc(bufferSize);
  particleRef_ = inputBuffer_->allocInterleaved(attributes_);
  bufferRange_.size_ = bufferSize;

  shaderDefine("NUM_PARTICLE_ATTRIBUTES", REGEN_STRING(attributes_.size()));

  createUpdateShader();
  createVAO(vaoFeedback_, feedbackRef_);
  createVAO(vao_, particleRef_);

  REGEN_DEBUG("Particle buffers created size="<<bufferSize<<".");
}

void Particles::createUpdateShader()
{
  StateConfigurer shaderConfigurer;
  shaderConfigurer.addState(this);

  StateConfig &shaderCfg = shaderConfigurer.cfg();
  shaderCfg.feedbackAttributes_.clear();
  for(list< ref_ptr<ShaderInput> >::const_iterator
      it=attributes_.begin(); it!=attributes_.end(); ++it)
  {
    shaderCfg.feedbackAttributes_.push_back((*it)->name());
  }
  shaderCfg.feedbackMode_ = GL_INTERLEAVED_ATTRIBS;
  shaderCfg.feedbackStage_ = GL_VERTEX_SHADER;
  updateState_->createShader(shaderCfg, updateShaderKey_);
  shaderCfg.feedbackAttributes_.clear();
}

void Particles::createVAO(ref_ptr<VAO> &vao, VBOReference &ref)
{
  GLuint currOffset = ref->address();

  RenderState::get()->vao().push(vao->id());

  // Note: This call does not influence RenderState the Target
  // is the VAO state.
  glBindBuffer(GL_ARRAY_BUFFER, ref->bufferID());
  // note:setInput adds attribute to front of list.
  for(list< ref_ptr<ShaderInput> >::const_iterator
      it=attributes_.begin(); it!=attributes_.end(); ++it)
  {
    ShaderInput *att = it->get();
    att->set_buffer(ref->bufferID(), ref);
    att->set_offset(currOffset);
    currOffset += att->elementSize();

    GLint loc = updateState_->shader()->attributeLocation(att->name());
    if(loc!=-1) att->enableAttribute(loc);
  }

  RenderState::get()->vao().pop();
}

void Particles::glAnimate(RenderState *rs, GLdouble dt)
{
  GL_ERROR_LOG();

#define DEBUG_FIRST_PARTICLE
#ifdef DEBUG_FIRST_PARTICLE
  REGEN_INFO("Debugging first particle...")
  for(list< ref_ptr<ShaderInput> >::iterator
      it=attributes_.begin(); it!=attributes_.end(); ++it)
  {
    ref_ptr<ShaderInput> in = *it;

    rs->arrayBuffer().push(in->buffer());
    GLvoid *data = glMapBufferRange(
        GL_ARRAY_BUFFER,
        in->offset(),
        in->elementSize(),
        GL_MAP_READ_BIT);
    stringstream ss;
    ss << "    " << in->name() << ": ";
    switch(in->dataType()) {
    case GL_FLOAT:
      for(GLuint i=0; i<in->valsPerElement(); ++i)
        ss << ((GLfloat*)data)[i] << " ";
      break;
    case GL_UNSIGNED_INT:
      for(GLuint i=0; i<in->valsPerElement(); ++i)
        ss << ((GLuint*)data)[i] << " ";
      break;
    default:
      ss << "Unhandled type.";
      break;
    }
    string infoStr = ss.str();
    REGEN_INFO(infoStr);
    glUnmapBuffer(GL_ARRAY_BUFFER);
  }
  GL_ERROR_LOG();
#endif

  deltaT_->setVertex(0,dt);

  rs->toggles().push(RenderState::RASTARIZER_DISCARD, GL_TRUE);
  updateState_->enable(rs);
  rs->vao().push(vao_->id());

  bufferRange_.buffer_ = feedbackRef_->bufferID();
  bufferRange_.offset_ = feedbackRef_->address();
  rs->feedbackBufferRange().push(0, bufferRange_);
  rs->beginTransformFeedback(feedbackPrimitive_);

  glDrawArrays(primitive_, 0, inputContainer_->numVertices());

  rs->endTransformFeedback();
  rs->feedbackBufferRange().pop(0);

  rs->vao().pop();
  updateState_->disable(rs);
  rs->toggles().pop(RenderState::RASTARIZER_DISCARD);

  // ping pong buffers
  {
    ref_ptr<VBO> buf = inputBuffer_;
    inputBuffer_ = feedbackBuffer_;
    feedbackBuffer_ = buf;
  }
  {
    ref_ptr<VAO> buf = vao_;
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

const ref_ptr<ShaderInput3f>& Particles::gravity() const
{ return gravity_; }
const ref_ptr<ShaderInput1f>& Particles::dampingFactor() const
{ return dampingFactor_; }
const ref_ptr<ShaderInput1f>& Particles::noiseFactor() const
{ return noiseFactor_; }
