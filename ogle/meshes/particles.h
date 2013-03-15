/*
 * particle-state.h
 *
 *  Created on: 03.11.2012
 *      Author: daniel
 */

#ifndef PARTICLE_STATE_H_
#define PARTICLE_STATE_H_

#include <ogle/meshes/mesh-state.h>
#include <ogle/states/shader-state.h>

namespace ogle {
/**
 * \brief Point sprite particle system.
 *
 * Using the geometry shader for updating
 * and emitting particles and using transform feedback to
 * stream updated particle attributes to a ping pong VBO.
 */
class ParticleState : public MeshState
{
public:
  /**
   * @param numParticles particle count.
   * @param blendMode particle blend mode.
   */
  ParticleState(GLuint numParticles, BlendMode blendMode);
  /**
   * @param numParticles particle count.
   */
  ParticleState(GLuint numParticles);

  /**
   * @param v should particles receive shadows cast from other objects.
   */
  void set_isShadowReceiver(GLboolean v);
  /**
   * @param v should particles fade away where they interect the scene.
   */
  void set_softParticles(GLboolean v);
  /**
   * @param v should particles fade away where they are near to the camera.
   */
  void set_nearCameraSoftParticles(GLboolean v);

  /**
   * Adds an attribute for the particles.
   * Attributes can be modified over time.
   * @param in the particle attribute.
   */
  void addParticleAttribute(const ref_ptr<ShaderInput> &in);
  /**
   * Used for checking in if the particles intersect with the scene.
   * @param tex the scene depth texture.
   */
  void set_depthTexture(const ref_ptr<Texture> &tex);

  /**
   * Creates buffer used for transform feedback.
   * The buffer must be recreated when particle attributes are added
   * or removed.
   */
  void createBuffer();
  /**
   * Creates the particle update and draw shader.
   * @param shaderCfg the shader configuration
   * @param updateKey include key for update shader
   * @param drawKey include key for draw shader
   */
  void createShader(
      ShaderState::Config &shaderCfg,
      const string &updateKey,
      const string &drawKey);

  /**
   * @param rs the render state.
   * @param dt time difference to last call.
   */
  void update(RenderState *rs, GLdouble dt);

  /**
   * @return reference to gravity constant.
   */
  const ref_ptr<ShaderInput3f>& gravity() const;
  /**
   * @return reference to damping factor.
   */
  const ref_ptr<ShaderInput1f>& dampingFactor() const;
  /**
   * @return reference to noise factor.
   */
  const ref_ptr<ShaderInput1f>& noiseFactor() const;
  /**
   * @return reference to number of maximum particle emits per frame.
   */
  const ref_ptr<ShaderInput1i>& maxNumParticleEmits() const;

  /**
   * @return the particle brightness.
   */
  const ref_ptr<ShaderInput1f>& brightness() const;
  /**
   * Soft particles fade away where they intersect the scene.
   * @return the soft particle scale.
   */
  const ref_ptr<ShaderInput1f>& softScale() const;

protected:
  ref_ptr<VertexBufferObject> particleBuffer_;
  ref_ptr<VertexBufferObject> feedbackBuffer_;

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
} // end ogle namespace

#endif /* PARTICLE_STATE_H_ */
