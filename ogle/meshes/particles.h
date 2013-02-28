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
  ParticleState(GLuint numParticles, BlendMode blendMode);
  ParticleState(GLuint numParticles);

  void set_isShadowReceiver(GLboolean v);
  void set_softParticles(GLboolean v);
  void set_nearCameraSoftParticles(GLboolean v);

  void addParticleAttribute(const ref_ptr<ShaderInput> &in);
  void set_depthTexture(const ref_ptr<Texture> &tex);

  void createBuffer();
  void createShader(ShaderConfig &shaderCfg, const string &updateKey, const string &drawKey);

  const ref_ptr<ShaderInput3f>& gravity() const;
  const ref_ptr<ShaderInput1f>& dampingFactor() const;
  const ref_ptr<ShaderInput1f>& noiseFactor() const;
  const ref_ptr<ShaderInput1i>& maxNumParticleEmits() const;

  const ref_ptr<ShaderInput1f>& brightness() const;
  const ref_ptr<ShaderInput1f>& softScale() const;

  // override
  virtual void animate(GLdouble dt);
  virtual void glAnimate(RenderState *rs, GLdouble dt);
  virtual GLboolean useGLAnimation() const;
  virtual GLboolean useAnimation() const;
protected:
  ref_ptr<VertexBufferObject> particleBuffer_;

  list< ref_ptr<VertexAttribute> > attributes_;
  ref_ptr<ShaderInput1f> lifetimeInput_;

  ref_ptr<ShaderInput3f> gravity_;
  ref_ptr<ShaderInput1f> dampingFactor_;
  ref_ptr<ShaderInput1f> noiseFactor_;
  ref_ptr<ShaderInput1i> maxNumParticleEmits_;

  ref_ptr<ShaderInput1f> softScale_;
  ref_ptr<ShaderInput1f> brightness_;
  ref_ptr<TextureState> depthTexture_;

  ref_ptr<ShaderState> updateShaderState_;
  ref_ptr<ShaderState> drawShaderState_;

  void init(GLuint numParticles);
};


#endif /* PARTICLE_STATE_H_ */
