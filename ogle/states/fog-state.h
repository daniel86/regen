/*
 * fog.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef FOG_H_
#define FOG_H_

#include <ogle/states/shader-input-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/light-state.h>
#include <ogle/states/fbo-state.h>
#include <ogle/states/texture-state.h>
#include <ogle/meshes/particles.h>
#include <ogle/meshes/rectangle.h>

class FogState : public State
{
public:
  FogState();

  void createBuffer(const Vec2ui &size);
  void resizeBuffer(const Vec2ui &size);

  void createShaders(ShaderConfig &cfg);

  void set_useSkyScattering(GLboolean v);

  /////////////
  /////////////

  void set_sceneFramebuffer(const ref_ptr<FrameBufferObject> &fbo);
  void set_colorTexture(const ref_ptr<Texture> &tex);
  void set_depthTexture(const ref_ptr<Texture> &tex);

  const ref_ptr<ShaderInput1f>& fogStart() const;
  const ref_ptr<ShaderInput1f>& fogEnd() const;
  const ref_ptr<ShaderInput1f>& fogExposure() const;
  const ref_ptr<ShaderInput1f>& fogScale() const;
  const ref_ptr<ShaderInput3f>& constFogColor() const;
  const ref_ptr<ShaderInput1f>& constFogDensity() const;

  /////////////
  /////////////

  /**
   * Add a light as fog color source.
   */
  void addLight(const ref_ptr<DirectionalLight> &l);
  /**
   * Add a light as fog color source.
   */
  void addLight(const ref_ptr<PointLight> &l);
  /**
   * Add a light as fog color source.
   */
  void addLight(const ref_ptr<SpotLight> &l);

  // controls the overall intensity of constant color contribution
  const ref_ptr<ShaderInput1f>& sunDiffuseExposure() const;
  // controls the overall intensity of the post-process
  const ref_ptr<ShaderInput1f>& sunScatteringExposure() const;
  // controls the intensity of each sample
  // exponential decay factor practically allows each light
  // shaft to fall off smoothly away from the light source
  const ref_ptr<ShaderInput1f>& sunScatteringDecay() const;
  // dissipates each sample's contribution as the ray progresses away from the light source [0, 1]
  const ref_ptr<ShaderInput1f>& sunScatteringWeight() const;
  // control over the separation between samples for cases in which we wish
  // to reduce the overall number of sample iterations while retaining
  // a sufficiently alias-free sampling density
  const ref_ptr<ShaderInput1f>& sunScatteringDensity() const;
  const ref_ptr<ShaderInput1f>& sunScatteringSamples() const;

  const ref_ptr<ShaderInput1f>& lightDiffuseExposure() const;

  /////////////
  /////////////

  /**
   * Creates density particles, else constant density is used.
   */
  void setDensityParticles(GLuint numParticles);

  const ref_ptr<ParticleState>& densityParticles() const;

  const ref_ptr<ShaderInput1f>& densityScale() const;
  const ref_ptr<ShaderInput1f>& intensityBias() const;
  const ref_ptr<ShaderInput1f>& emitVelocity() const;
  const ref_ptr<ShaderInput1f>& emitterRadius() const;
  const ref_ptr<ShaderInput2f>& emitRadius() const;
  const ref_ptr<ShaderInput2f>& emitDensity() const;
  const ref_ptr<ShaderInput2f>& emitLifetime() const;

  /////////////
  /////////////

  virtual void enable(RenderState *rs);
  virtual void disable(RenderState *rs);

protected:
  ref_ptr<Rectangle> quad_;
  GLboolean useConstFogColor_;

  // fog color/density pass target
  ref_ptr<FBOState> fogBuffer_;
  ref_ptr<TextureState> fogColorTexture_;
  ref_ptr<TextureState> fogDensityTexture_;
  // scene as seen from camera
  ref_ptr<FrameBufferObject> sceneFramebuffer_;
  ref_ptr<TextureState> depthTexture_;
  ref_ptr<TextureState> colorTexture_;

  ref_ptr<ShaderState> accumulateShader_;
  ref_ptr<ShaderInput1f> fogStart_;
  ref_ptr<ShaderInput1f> fogEnd_;
  ref_ptr<ShaderInput1f> fogExposure_;
  ref_ptr<ShaderInput1f> fogScale_;
  ref_ptr<ShaderInput3f> constFogColor_;
  ref_ptr<ShaderInput1f> constFogDensity_;

  /////////////
  /////////////

  list< ref_ptr<DirectionalLight> > dirLights_;
  ref_ptr<ShaderState> dirShader_;
  GLint sunLightDirLoc_;
  GLint sunLightDiffuseLoc_;
  ref_ptr<ShaderInput1f> sunDiffuseExposure_;
  ref_ptr<ShaderInput1f> sunScatteringExposure_;
  ref_ptr<ShaderInput1f> sunScatteringDecay_;
  ref_ptr<ShaderInput1f> sunScatteringWeight_;
  ref_ptr<ShaderInput1f> sunScatteringDensity_;
  ref_ptr<ShaderInput1f> sunScatteringSamples_;

  list< ref_ptr<SpotLight> > spotLights_;
  list< ref_ptr<PointLight> > pointLights_;
  ref_ptr<ShaderState> spotShader_;
  GLint spotLightPosLoc_;
  GLint spotLightDirLoc_;
  GLint spotLightDiffuseLoc_;
  GLint spotLightAttenuationLoc_;
  GLint spotConeMatLoc_;
  GLint spotPosLoc_;
  ref_ptr<ShaderState> pointShader_;
  GLint pointLightPosLoc_;
  GLint pointLightDiffuseLoc_;
  GLint pointLightAttenuationLoc_;
  ref_ptr<ShaderInput1f> lightDiffuseExposure_;

  /////////////
  /////////////

  ref_ptr<ParticleState> densityParticles_;
  ref_ptr<ShaderInput1f> densityScale_;
  ref_ptr<ShaderInput1f> densityBias_;
  ref_ptr<ShaderInput1f> emitVelocity_;
  ref_ptr<ShaderInput1f> emitterRadius_;
  ref_ptr<ShaderInput2f> emitRadius_;
  ref_ptr<ShaderInput2f> emitDensity_;
  ref_ptr<ShaderInput2f> emitLifetime_;
};

#endif /* FOG_H_ */
