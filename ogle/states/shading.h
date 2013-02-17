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

  void setEnvironmentLight(
      const ref_ptr<DirectionalLight> &sunLight,
      const ref_ptr<TextureCube> &skyMap);

  void addLight(
      const ref_ptr<DirectionalLight> &l);
  void addLight(
      const ref_ptr<DirectionalLight> &l,
      const ref_ptr<DirectionalShadowMap> &sm);

  void addLight(
      const ref_ptr<PointLight> &l);
  void addLight(
      const ref_ptr<PointLight> &l,
      const ref_ptr<PointShadowMap> &sm);

  void addLight(
      const ref_ptr<SpotLight> &l);
  void addLight(
      const ref_ptr<SpotLight> &l,
      const ref_ptr<SpotShadowMap> &sm);

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
  ref_ptr<DeferredEnvLight> envState_;
};

class DeferredEnvLight : public State
{
public:
  DeferredEnvLight(const ref_ptr<TextureCube> &envCube);

  void createShader(ShaderConfig &cfg);
protected:
  friend class DeferredShading;
  ref_ptr<ShaderState> shader_;
};

class DeferredDirLight : public State
{
public:
  DeferredDirLight();

  void createShader(ShaderConfig &cfg);

  void addLight(
      const ref_ptr<DirectionalLight> &l,
      const ref_ptr<DirectionalShadowMap> &sm);
  void removeLight(Light *l);

  virtual void enable(RenderState *rs);
protected:
  friend class DeferredShading;
  struct DeferredLight {
    DeferredLight(
        const ref_ptr<DirectionalLight> &_l,
        const ref_ptr<DirectionalShadowMap> &_sm
    ) : l(_l), sm(_sm) {}
    ref_ptr<DirectionalLight> l;
    ref_ptr<DirectionalShadowMap> sm;
  };

  list<DeferredLight> lights_;
  map< Light*, list<DeferredLight>::iterator > lightIterators_;

  ref_ptr<MeshState> mesh_;
  ref_ptr<ShaderState> shader_;

  GLint dirLoc_;
  GLint diffuseLoc_;
  GLint specularLoc_;

  GLint shadowMapLoc_;
  GLint shadowMatricesLoc_;
  GLint shadowFarLoc_;
  GLint shadowMapSizeLoc_;
};

class DeferredPointLight : public State
{
public:
  DeferredPointLight();

  void createShader(ShaderConfig &cfg);

  void addLight(
      const ref_ptr<PointLight> &l,
      const ref_ptr<PointShadowMap> &sm);
  void removeLight(Light *l);

  virtual void enable(RenderState *rs);
protected:
  friend class DeferredShading;
  struct DeferredLight {
    DeferredLight(
        const ref_ptr<PointLight> &_l,
        const ref_ptr<PointShadowMap> &_sm
    ) : l(_l), sm(_sm) {}
    ref_ptr<PointLight> l;
    ref_ptr<PointShadowMap> sm;
  };

  list<DeferredLight> lights_;
  map< Light*, list<DeferredLight>::iterator > lightIterators_;

  ref_ptr<MeshState> mesh_;
  ref_ptr<ShaderState> shader_;

  GLint posLoc_;
  GLint radiusLoc_;
  GLint diffuseLoc_;
  GLint specularLoc_;

  GLint shadowMapLoc_;
  GLint shadowMapSizeLoc_;
  GLint shadowFarLoc_;
  GLint shadowNearLoc_;
};

class DeferredSpotLight : public State
{
public:
  DeferredSpotLight();

  void createShader(ShaderConfig &cfg);

  void addLight(
      const ref_ptr<SpotLight> &l,
      const ref_ptr<SpotShadowMap> &sm);
  void removeLight(Light *l);

  virtual void enable(RenderState *rs);
protected:
  friend class DeferredShading;
  struct DeferredLight {
    DeferredLight(
        const ref_ptr<SpotLight> &_l,
        const ref_ptr<SpotShadowMap> &_sm
    ) : l(_l), sm(_sm), dirStamp(0) {}
    ref_ptr<SpotLight> l;
    ref_ptr<SpotShadowMap> sm;
    GLuint dirStamp;
  };

  list<DeferredLight> lights_;
  map< Light*, list<DeferredLight>::iterator > lightIterators_;

  ref_ptr<MeshState> mesh_;
  ref_ptr<ShaderState> shader_;

  GLint dirLoc_;
  GLint posLoc_;
  GLint radiusLoc_;
  GLint diffuseLoc_;
  GLint specularLoc_;
  GLint coneAnglesLoc_;
  GLint coneMatLoc_;

  GLint shadowMapLoc_;
  GLint shadowMapSizeLoc_;
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
