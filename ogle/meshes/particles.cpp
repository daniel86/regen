/*
 * particle-state.cpp
 *
 *  Created on: 03.11.2012
 *      Author: daniel
 */

#include <ctime>    // For time()
#include <cstdlib>  // For srand() and rand()

#include <boost/algorithm/string.hpp>

#include <ogle/utility/gl-error.h>
#include <ogle/utility/logging.h>
#include <ogle/utility/string-util.h>
#include <ogle/states/render-state.h>
#include <ogle/states/blend-state.h>
#include <ogle/states/depth-state.h>

#include "particles.h"

///////////

ParticleState::Emitter::Emitter(GLuint numParticles)
: numParticles_(numParticles)
{
}

ParticleState::ParticleState(GLuint numParticles)
: MeshState(GL_POINTS)
{
  numVertices_ = numParticles;

  { // initialize default attributes
    posInput_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("pos"));
    posInput_->setVertexData(numParticles, NULL);
    setInput(ref_ptr<ShaderInput>::cast(posInput_));

    velocityInput_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("velocity"));
    velocityInput_->setVertexData(numParticles, NULL);
    setInput(ref_ptr<ShaderInput>::cast(velocityInput_));

    // initially set lifetime to zero so that particles
    // get emitted in the first step
    GLfloat zeroLifetimeData[numParticles];
    memset(zeroLifetimeData, 0, sizeof(zeroLifetimeData));
    lifetimeInput_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("lifetime"));
    lifetimeInput_->setVertexData(numParticles, (byte*)zeroLifetimeData);
    setInput(ref_ptr<ShaderInput>::cast(lifetimeInput_));

    // get a random seed for each particle
    GLuint initialSeedData[numParticles];
    srand(time(0));
    for(GLuint i=0u; i<numParticles; ++i) {
      initialSeedData[i] = rand();
    }
    ref_ptr<ShaderInput1ui> randomSeed_ = ref_ptr<ShaderInput1ui>::manage(new ShaderInput1ui("randomSeed"));
    randomSeed_->setVertexData(numParticles, (byte*)initialSeedData);
    setInput(ref_ptr<ShaderInput>::cast(randomSeed_));
  }

  shaderState_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(shaderState_));
}

void ParticleState::addEmitter(const ParticleState::Emitter &emitter)
{
  particleEmitter_.push_back(emitter);
}

void ParticleState::addUpdater(const string &name, const string &code)
{
  particleUpdater_[name] = code;
}
void ParticleState::addUpdater(const string &key)
{
  list<string> path;
  boost::split(path, key, boost::is_any_of("."));
  particleUpdater_[*path.rbegin()] = key;
}

string ParticleState::createEmitShader(
    ParticleState::Emitter &emitter,
    GLuint emitterIndex)
{
  stringstream ss;
  ss << "void emit" << emitterIndex << "(float dt, inout uint randomSeed) {" << endl;
  for(list<FuzzyShaderValue>::iterator
      it=emitter.values_.begin(); it!=emitter.values_.end(); ++it)
  {
    FuzzyShaderValue &val = *it;
    ss << "    out_" << val.name << " = " << val.value;
    if(!val.variance.empty()) {
      ss << " + variance(" << val.variance << ", randomSeed)";
    }
    ss << ";" << endl;
  }
  ss << "}" << endl;
  return ss.str();
}

void ParticleState::createResources(ShaderConfig &cfg, const string &effectName)
{
  list< ref_ptr<VertexAttribute> > attributes;
  GLuint bufferSize = 0, counter;

  // find the buffer size and add each vertex attribute
  // to the transform feedback list
  for(map< string, ref_ptr<ShaderInput> >::const_iterator
      it=cfg.inputs_.begin(); it!=cfg.inputs_.end(); ++it)
  {
    ref_ptr<ShaderInput> in = it->second;
    if(in->numVertices()!=numVertices()) { continue; }
    bufferSize += in->elementSize();
    attributes.push_back(ref_ptr<VertexAttribute>::cast(in));
  }
  bufferSize *= numVertices();

  for(list< ref_ptr<VertexAttribute> >::const_iterator
      it=attributes.begin(); it!=attributes.end(); ++it)
  {
    cfg.transformFeedbackAttributes_.push_back((*it)->name());
  }
  cfg.transformFeedbackMode_ = GL_INTERLEAVED_ATTRIBS;

  DEBUG_LOG("Creating particle resources. " <<
      "Number of particles: " << numVertices() << ". " <<
      "Number of attributes: " << attributes.size() << ". " <<
      "Buffer size: " << bufferSize << "."
  );

  particleBuffer_ = ref_ptr<VertexBufferObject>::manage(
      new VertexBufferObject(VertexBufferObject::USAGE_DYNAMIC, bufferSize));
  feedbackBuffer_ = ref_ptr<VertexBufferObject>::manage(
      new VertexBufferObject(VertexBufferObject::USAGE_DYNAMIC, bufferSize));
  particleBuffer_->allocateInterleaved(attributes);

  // setup particle updater
  cfg.defines_["NUM_PARTICLE_UPDATER"] = FORMAT_STRING(particleUpdater_.size());
  counter = 0;
  for(map<string,string>::iterator it=particleUpdater_.begin(); it!=particleUpdater_.end(); ++it) {
    cfg.defines_[FORMAT_STRING("PARTICLE_UPDATER"<<counter<<"_NAME")] = it->first;
    cfg.functions_[it->first] = it->second;
    ++counter;
  }

  // setup emitter
  if(particleEmitter_.empty()) {
    WARN_LOG("no particle emitter added.");
  }
  cfg.defines_["NUM_PARTICLE_EMITTER"] = FORMAT_STRING(particleEmitter_.size());
  counter = 0;
  GLuint emitterStop = 0;
  for(list<ParticleState::Emitter>::iterator
      it=particleEmitter_.begin(); it!=particleEmitter_.end(); ++it)
  {
    ParticleState::Emitter &emit = *it;
    string name = FORMAT_STRING("emit" << counter);
    cfg.defines_[FORMAT_STRING("PARTICLE_EMITTER"<<counter<<"_NAME")] = name;
    cfg.functions_[name] = createEmitShader(emit,counter);

    emitterStop += emit.numParticles_;
    cfg.defines_[FORMAT_STRING("PARTICLE_EMITTER"<<counter<<"_STOP")] = FORMAT_STRING(emitterStop);

    ++counter;
  }

  // setup particle attributes
  cfg.defines_["NUM_PARTICLE_ATTRIBUTES"] = FORMAT_STRING(attributes.size());
  counter = 0;
  for(list< ref_ptr<VertexAttribute> >::const_iterator
      it=attributes.begin(); it!=attributes.end(); ++it)
  {
    const ref_ptr<VertexAttribute> &att = *it;
    cfg.defines_[FORMAT_STRING("PARTICLE_ATTRIBUTE"<<counter<<"_TYPE")] = att->shaderDataType();
    cfg.defines_[FORMAT_STRING("PARTICLE_ATTRIBUTE"<<counter<<"_NAME")] = att->name();
    ++counter;
  }
  cfg.defines_["HAS_GEOMETRY_SHADER"] = "TRUE";

  shaderState_->createShader(cfg, effectName);
}

void ParticleState::draw(GLuint numInstances)
{
  // bind target buffer for new particle data
  glBindBufferRange(
      GL_TRANSFORM_FEEDBACK_BUFFER,
      0, feedbackBuffer_->id(),
      0, feedbackBuffer_->bufferSize()
  );
  glBeginTransformFeedback(GL_POINTS);
  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_POINT_SPRITE);

  glDrawArrays(primitive_, 0, numVertices_);

  glDisable(GL_POINT_SPRITE);
  glDisable(GL_PROGRAM_POINT_SIZE);
  glEndTransformFeedback();
}

void ParticleState::enable(RenderState *state)
{
  // bind buffer containing current particle data
  state->pushVBO(particleBuffer_.get());
  MeshState::enable(state);
}

void ParticleState::disable(RenderState *state)
{
  MeshState::disable(state);
  // disable particle buffer
  state->popVBO();
  // switch feedback and particle buffer,
  // the feedback buffer contains new particle data
  ref_ptr<VertexBufferObject> buf = particleBuffer_;
  particleBuffer_ = feedbackBuffer_;
  feedbackBuffer_ = buf;
}
