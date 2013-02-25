/*
 * shading.h
 *
 *  Created on: 08.02.2013
 *      Author: daniel
 */

#ifndef SHADING_H_
#define SHADING_H_

#include <ogle/states/state.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/light-state.h>
#include <ogle/shadows/directional-shadow-map.h>
#include <ogle/shadows/point-shadow-map.h>
#include <ogle/shadows/spot-shadow-map.h>

class DeferredDirLight;
class DeferredPointLight;
class DeferredSpotLight;
class DeferredEnvLight;
class DeferredAmbientLight;
class AmbientOcclusion;
class ShadingPostProcessing;

/**
 * Accumulates PerPixel Lighting in Image Space using a pre generated
 * diffuse/specular/normal texture.
 * For each light the contribution is computed individually and
 * then accumulated using add blending.
 * The output is written to the currently bound target.
 */
class DeferredShading : public State
{
public:
  DeferredShading();
  void createShader(ShaderConfig &cfg);

  /**
   * Input buffers for deferred shading.
   */
  void set_gBuffer(
      const ref_ptr<Texture> &depthTexture,
      const ref_ptr<Texture> &norWorldTexture,
      const ref_ptr<Texture> &diffuseTexture,
      const ref_ptr<Texture> &specularTexture
  );

  void setUseAmbientLight();

  void addLight(const ref_ptr<DirectionalLight> &l);
  void addLight(const ref_ptr<DirectionalLight> &l, const ref_ptr<DirectionalShadowMap> &sm);

  void addLight(const ref_ptr<PointLight> &l);
  void addLight(const ref_ptr<PointLight> &l, const ref_ptr<PointShadowMap> &sm);

  void addLight(const ref_ptr<SpotLight> &l);
  void addLight(const ref_ptr<SpotLight> &l, const ref_ptr<SpotShadowMap> &sm);

  void removeLight(DirectionalLight *l);
  void removeLight(PointLight *l);
  void removeLight(SpotLight *l);

  const ref_ptr<DeferredDirLight>& dirState() const;
  const ref_ptr<DeferredDirLight>& dirShadowState() const;
  const ref_ptr<DeferredPointLight>& pointState() const;
  const ref_ptr<DeferredPointLight>& pointShadowState() const;
  const ref_ptr<DeferredSpotLight>& spotState() const;
  const ref_ptr<DeferredSpotLight>& spotShadowState() const;
  const ref_ptr<DeferredAmbientLight>& ambientState() const;

protected:
  ref_ptr<TextureState> gDepthTexture_;
  ref_ptr<TextureState> gDiffuseTexture_;
  ref_ptr<TextureState> gSpecularTexture_;
  ref_ptr<TextureState> gNorWorldTexture_;

  ref_ptr<StateSequence> lightSequence_;
  ref_ptr<DeferredDirLight> dirState_;
  ref_ptr<DeferredDirLight> dirShadowState_;
  ref_ptr<DeferredPointLight> pointState_;
  ref_ptr<DeferredPointLight> pointShadowState_;
  ref_ptr<DeferredSpotLight> spotState_;
  ref_ptr<DeferredSpotLight> spotShadowState_;
  ref_ptr<DeferredAmbientLight> ambientState_;
  GLboolean hasAmbient_;
};

class AmbientOcclusion : public State
{
public:
  AmbientOcclusion(GLfloat sizeScale);
  void createFilter(const ref_ptr<Texture> &input);
  void createShader(ShaderConfig &cfg);
  void resize();

  const ref_ptr<Texture>& aoTexture() const;

  const ref_ptr<ShaderInput1f>& blurSigma() const;
  const ref_ptr<ShaderInput1f>& blurNumPixels() const;
  const ref_ptr<ShaderInput1f>& aoSamplingRadius() const;
  const ref_ptr<ShaderInput1f>& aoBias() const;
  const ref_ptr<ShaderInput2f>& aoAttenuation() const;

protected:
  ref_ptr<FilterSequence> filter_;
  ref_ptr<ShaderInput1f> blurSigma_;
  ref_ptr<ShaderInput1f> blurNumPixels_;
  ref_ptr<ShaderInput1f> aoSamplingRadius_;
  ref_ptr<ShaderInput1f> aoBias_;
  ref_ptr<ShaderInput2f> aoAttenuation_;
  GLfloat sizeScale_;
};

class DeferredAmbientLight : public State
{
public:
  DeferredAmbientLight();
  void createShader(ShaderConfig &cfg);
  const ref_ptr<ShaderInput3f>& ambientLight() const;
protected:
  ref_ptr<ShaderState> shader_;
  ref_ptr<ShaderInput3f> ambientLight_;
};

/**
 * Base class for deferred lights.
 */
class DeferredLight : public State
{
public:
  DeferredLight();

  GLboolean useShadowMoments();
  GLboolean useShadowSampler();

  void setShadowFiltering(ShadowMap::FilterMode mode);

  void addLight(const ref_ptr<Light> &l, const ref_ptr<ShadowMap> &sm);
  void removeLight(Light *l);

protected:
  friend class DeferredShading;
  ref_ptr<MeshState> mesh_;

  ref_ptr<ShaderState> shader_;
  GLint shadowMapLoc_;

  ShadowMap::FilterMode shadowFiltering_;

  struct DLight {
    DLight(
        const ref_ptr<Light> &light,
        const ref_ptr<ShadowMap> &shadowMap)
    : l(light), sm(shadowMap)
    {}
    ref_ptr<Light> l;
    ref_ptr<ShadowMap> sm;
  };
  list<DLight> lights_;
  map< Light*, list<DLight>::iterator > lightIterators_;


  void activateShadowMap(ShadowMap *sm, GLuint channel);
};

class DeferredDirLight : public DeferredLight
{
public:
  DeferredDirLight();
  void createShader(ShaderConfig &cfg);

  GLuint numShadowLayer() const;
  void set_numShadowLayer(GLuint numLayer);

  // override
  virtual void enable(RenderState *rs);
  virtual void addLight(const ref_ptr<Light> &l, const ref_ptr<ShadowMap> &sm);

protected:
  GLuint numShadowLayer_;

  GLint dirLoc_;
  GLint diffuseLoc_;
  GLint specularLoc_;
  GLint shadowMatricesLoc_;
  GLint shadowFarLoc_;
};

class DeferredPointLight : public DeferredLight
{
public:
  DeferredPointLight();
  void createShader(ShaderConfig &cfg);
  // override
  virtual void enable(RenderState *rs);

protected:
  GLint posLoc_;
  GLint radiusLoc_;
  GLint diffuseLoc_;
  GLint specularLoc_;
  GLint shadowFarLoc_;
  GLint shadowNearLoc_;
};

class DeferredSpotLight : public DeferredLight
{
public:
  DeferredSpotLight();
  void createShader(ShaderConfig &cfg);
  // override
  virtual void enable(RenderState *rs);

protected:
  GLint dirLoc_;
  GLint posLoc_;
  GLint radiusLoc_;
  GLint diffuseLoc_;
  GLint specularLoc_;
  GLint coneAnglesLoc_;
  GLint coneMatLoc_;
  GLint shadowMatLoc_;
  GLint shadowNearLoc_;
  GLint shadowFarLoc_;
};

class ShadingPostProcessing : public State
{
public:
  ShadingPostProcessing();
  void createShader(ShaderConfig &cfg);
  void resize();

  void setUseAmbientOcclusion();
  const ref_ptr<AmbientOcclusion>& ambientOcclusionState() const;

  void set_gBuffer(
      const ref_ptr<Texture> &depthTexture,
      const ref_ptr<Texture> &norWorldTexture,
      const ref_ptr<Texture> &diffuseTexture);
  void set_tBuffer(const ref_ptr<Texture> &colorTexture);
  void set_aoBuffer(const ref_ptr<Texture> &aoTexture);

protected:
  ref_ptr<TextureState> gDiffuseTexture_;
  ref_ptr<TextureState> gDepthTexture_;
  ref_ptr<TextureState> gNorWorldTexture_;
  ref_ptr<TextureState> tColorTexture_;
  ref_ptr<TextureState> aoTexture_;

  ref_ptr<StateSequence> stateSequence_;
  ref_ptr<ShaderState> shader_;
  ref_ptr<AmbientOcclusion> updateAOState_;
  GLboolean hasAO_;
};

/**
 * Direct Shading computes Per Pixel Light in the same shader
 * the geometry and texturing is done.
 */
class DirectShading : public State
{
public:
  DirectShading();
  virtual void addLight(const ref_ptr<Light> &l);
  virtual void removeLight(const ref_ptr<Light> &l);
protected:
  list< ref_ptr<Light> > lights_;
};

#endif /* SHADING_H_ */
