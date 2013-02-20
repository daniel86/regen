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

  void setDirFiltering(ShadowMap::FilterMode mode);
  void setPointFiltering(ShadowMap::FilterMode mode);
  void setSpotFiltering(ShadowMap::FilterMode mode);

  void setAmbientLight(const Vec3f &v);

  void addLight(const ref_ptr<DirectionalLight> &l);
  void addLight(const ref_ptr<DirectionalLight> &l, const ref_ptr<DirectionalShadowMap> &sm);

  void addLight(const ref_ptr<PointLight> &l);
  void addLight(const ref_ptr<PointLight> &l, const ref_ptr<PointShadowMap> &sm);

  void addLight(const ref_ptr<SpotLight> &l);
  void addLight(const ref_ptr<SpotLight> &l, const ref_ptr<SpotShadowMap> &sm);

  void removeLight(DirectionalLight *l);
  void removeLight(PointLight *l);
  void removeLight(SpotLight *l);

protected:
  ref_ptr<TextureState> gDepthTexture_;
  ref_ptr<TextureState> gDiffuseTexture_;
  ref_ptr<TextureState> gSpecularTexture_;
  ref_ptr<TextureState> gNorWorldTexture_;

  ref_ptr<StateSequence> deferredShadingSequence_;
  ref_ptr<DeferredDirLight> dirState_;
  ref_ptr<DeferredDirLight> dirShadowState_;
  ref_ptr<DeferredPointLight> pointState_;
  ref_ptr<DeferredPointLight> pointShadowState_;
  ref_ptr<DeferredSpotLight> spotState_;
  ref_ptr<DeferredSpotLight> spotShadowState_;
  ref_ptr<DeferredAmbientLight> ambientState_;
  GLboolean hasAmbient_;
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
  GLint shadowMapSizeLoc_;

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
  // override
  virtual void enable(RenderState *rs);

protected:
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
};

/**
 * Direct Shading computes Per Pixel Light in the same shader
 * the geometry and texturing is done.
 */
class DirectShading : public State
{
public:
  DirectShading();
  void addLight(const ref_ptr<Light> &l);
  void removeLight(const ref_ptr<Light> &l);
protected:
  list< ref_ptr<Light> > lights_;
};

/**
 * Combines results of deferred shading with results from
 * direct shading using alpha blending.
 * Additional the output can be multiplied using ambient occlusion
 * single channel textures for the shading results.
 * The output is written to the currently bound target.
 */
class CombineShading : public State
{
public:
  CombineShading();

  void createShader(ShaderConfig &cfg);

  void set_gBuffer(const ref_ptr<Texture> &colorTexture);
  void set_tBuffer(const ref_ptr<Texture> &colorTexture);

  void set_aoBuffer(const ref_ptr<Texture> &aoTexture);
  void set_taoBuffer(const ref_ptr<Texture> &taoTexture);

protected:
  ref_ptr<TextureState> gColorTexture_;
  ref_ptr<TextureState> tColorTexture_;
  ref_ptr<TextureState> aoTexture_;
  ref_ptr<TextureState> taoTexture_;
  ref_ptr<ShaderState> combineShader_;
};

#endif /* SHADING_H_ */
