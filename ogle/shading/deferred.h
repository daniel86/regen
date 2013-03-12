/*
 * deferred.h
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#ifndef __SHADING_DEFERRED_H_
#define __SHADING_DEFERRED_H_

#include <ogle/states/state.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/texture-state.h>

#include <ogle/shading/ambient-light.h>
#include <ogle/shading/point-light.h>
#include <ogle/shading/spot-light.h>
#include <ogle/shading/directional-light.h>
#include <ogle/shading/shadow-map.h>

namespace ogle {

/**
 * Main class for deferred shading.
 * The class requires a GBuffer containing textures for
 * diffuse and specular material color, a depth texture and
 * a texture that contains the normal vector.
 * Additionally the normal map texture saves a toggle in the
 * alpha component that is used to distinguish background
 * from rendered geometry.
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

  void addLight(const ref_ptr<Light> &l);
  void addLight(const ref_ptr<Light> &l, const ref_ptr<ShadowMap> &sm);
  void removeLight(Light *l);

  const ref_ptr<DeferredDirLight>& dirState() const;
  const ref_ptr<DeferredDirLight>& dirShadowState() const;
  const ref_ptr<DeferredPointLight>& pointState() const;
  const ref_ptr<DeferredPointLight>& pointShadowState() const;
  const ref_ptr<DeferredSpotLight>& spotState() const;
  const ref_ptr<DeferredSpotLight>& spotShadowState() const;
  const ref_ptr<DeferredAmbientLight>& ambientState() const;

protected:
  ShaderConfig shaderCfg_;
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
  GLboolean hasShaderConfig_;

  ref_ptr<DeferredLight> getLightState(const ref_ptr<Light>&, const ref_ptr<ShadowMap>&);
  void createLightStateShader(const ref_ptr<DeferredLight> &light);

  void removeLight(Light *l, const ref_ptr<DeferredLight> &lightState);
};

} // end ogle namespace

#endif /* __SHADING_DEFERRED_H_ */
