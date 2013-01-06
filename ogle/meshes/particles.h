/*
 * particle-state.h
 *
 *  Created on: 03.11.2012
 *      Author: daniel
 */

#ifndef PARTICLE_STATE_H_
#define PARTICLE_STATE_H_

#include <ogle/states/mesh-state.h>
#include <ogle/states/shader-state.h>

/**
 * Initial particle value with variance.
 */
struct FuzzyShaderValue {
  FuzzyShaderValue(const string &_name, const string &_value)
  : name(_name), value(_value), variance("") {}
  FuzzyShaderValue(const string &_name, const string &_value, const string &_variance)
  : name(_name), value(_value), variance(_variance) {}
  string name;
  string value;
  string variance;
};

/**
 * Point sprite particle system using the geometry shader for updating
 * and emitting particles and using transform feedback to
 * stream updated particle attributes to a ping pong VBO.
 */
class ParticleState : public MeshState
{
public:
  struct Emitter {
    // fuzzy initial values for emitted particles
    list<FuzzyShaderValue> values_;
    // number of particles managed by this emitter
    GLuint numParticles_;
    Emitter(GLuint numParticles);
  };

  ParticleState(GLuint numParticles);

  /**
   * Add an emitter to the particle system.
   */
  void addEmitter(const Emitter &emitter);

  /**
   * Adds updater with GLSL code specified.
   */
  void addUpdater(const string &name, const string &code);
  /**
   * Adds updater that can be included with the given key.
   */
  void addUpdater(const string &key);

  /**
   * Creates shader and VBO's used by particles.
   */
  void createResources(ShaderConfig &cfg, const string &effectName="particles");

  // override
  virtual void draw(GLuint numInstances);
  virtual void enable(RenderState *state);
  virtual void disable(RenderState *state);

protected:
  ref_ptr<ShaderState> shaderState_;
  ref_ptr<VertexBufferObject> particleBuffer_;
  ref_ptr<VertexBufferObject> feedbackBuffer_;

  ref_ptr<ShaderInput3f> posInput_;
  ref_ptr<ShaderInput3f> velocityInput_;
  ref_ptr<ShaderInput1f> lifetimeInput_;

  map<string,string> particleUpdater_;
  list<Emitter> particleEmitter_;

  string createEmitShader(
      ParticleState::Emitter &emitter,
      GLuint emitterIndex);
};


#endif /* PARTICLE_STATE_H_ */
