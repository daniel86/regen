/*
 * particle-state.h
 *
 *  Created on: 03.11.2012
 *      Author: daniel
 */

#ifndef PARTICLE_STATE_H_
#define PARTICLE_STATE_H_

#include <ogle/meshes/mesh-state.h>
#include <ogle/animations/animation.h>
#include <ogle/states/shader-state.h>

/**
 * Point sprite particle system using the geometry shader for updating
 * and emitting particles and using transform feedback to
 * stream updated particle attributes to a ping pong VBO.
 */
class ParticleState : public MeshState, public Animation
{
public:
  ParticleState(GLuint numParticles);

  void addParticleAttribute(const ref_ptr<ShaderInput> &in);

  void createBuffer();
  void createShader(ShaderConfig &shaderCfg, const string &updateKey, const string &drawKey);

  const ref_ptr<ShaderInput1f>& softScale() const;
  const ref_ptr<ShaderInput3f>& gravity() const;
  const ref_ptr<ShaderInput1f>& dampingFactor() const;
  const ref_ptr<ShaderInput1f>& noiseFactor() const;

  // override
  virtual void animate(GLdouble dt);
  virtual void glAnimate(GLdouble dt);
  virtual GLboolean useGLAnimation() const;
  virtual GLboolean useAnimation() const;
protected:
  ref_ptr<VertexBufferObject> particleBuffer_;
  list< ref_ptr<VertexAttribute> > attributes_;
  ref_ptr<ShaderInput1f> softScale_;
  ref_ptr<ShaderInput3f> gravity_;
  ref_ptr<ShaderInput1f> dampingFactor_;
  ref_ptr<ShaderInput1f> noiseFactor_;

  ref_ptr<ShaderState> updateShaderState_;
  ref_ptr<ShaderState> drawShaderState_;
};


#endif /* PARTICLE_STATE_H_ */
