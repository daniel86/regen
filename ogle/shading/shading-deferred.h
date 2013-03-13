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

#include <ogle/shading/light-pass.h>
#include <ogle/shading/shadow-map.h>
#include <ogle/shading/ambient-occlusion.h>

namespace ogle {
/**
 * \brief Implements deferred shading.
 *
 * Deferred shading is a screen-space shading technique.
 * It is called deferred because no shading is actually performed
 * in the first pass instead shading is 'deferred' until a second pass.
 *
 * On the first pass of a deferred shader,
 * data that is required for shading computation is only gathered.
 * Positions, normals, and materials for each surface
 * are rendered into the geometry buffer (G-buffer) as a series of textures.
 * After this, a pixel shader computes the direct and indirect lighting
 * at each pixel using the information of the texture buffers, in screen space.
 */
class DeferredShading : public State
{
public:
  DeferredShading();
  /**
   * @param cfg the shader configuration.
   */
  void createShader(ShaderState::Config &cfg);

  /**
   * Input buffers for deferred shading.
   */
  void set_gBuffer(
      const ref_ptr<Texture> &depthTexture,
      const ref_ptr<Texture> &norWorldTexture,
      const ref_ptr<Texture> &diffuseTexture,
      const ref_ptr<Texture> &specularTexture
  );

  /**
   * @param light a light
   */
  void addLight(const ref_ptr<Light> &light);
  /**
   * @param light a light
   * @param shadowMap a shadow map
   */
  void addLight(const ref_ptr<Light> &light, const ref_ptr<ShadowMap> &shadowMap);
  /**
   * @param l previously added light.
   */
  void removeLight(Light *l);

  /**
   * @return directional light pass.
   */
  const ref_ptr<LightPass>& dirState() const;
  /**
   * @return directional light pass using shadow mapping.
   */
  const ref_ptr<LightPass>& dirShadowState() const;
  /**
   * @return point light pass.
   */
  const ref_ptr<LightPass>& pointState() const;
  /**
   * @return point light pass using shadow mapping.
   */
  const ref_ptr<LightPass>& pointShadowState() const;
  /**
   * @return spot light pass.
   */
  const ref_ptr<LightPass>& spotState() const;
  /**
   * @return spot light pass using shadow mapping.
   */
  const ref_ptr<LightPass>& spotShadowState() const;

  /**
   * @return ambient light pass.
   */
  const ref_ptr<FullscreenPass>& ambientState() const;
  /**
   * @return the ambient light.
   */
  const ref_ptr<ShaderInput3f>& ambientLight() const;
  /**
   * Toggle on ambient light.
   */
  void setUseAmbientLight();
  /**
   * @return the ao state.
   */
  const ref_ptr<AmbientOcclusion>& ambientOcclusion() const;
  /**
   * Toggle on ambient occlusion.
   */
  void setUseAmbientOcclusion();

protected:
  ShaderState::Config shaderCfg_;
  GLboolean hasShaderConfig_;

  ref_ptr<TextureState> gDepthTexture_;
  ref_ptr<TextureState> gDiffuseTexture_;
  ref_ptr<TextureState> gSpecularTexture_;
  ref_ptr<TextureState> gNorWorldTexture_;

  ref_ptr<StateSequence> lightSequence_;
  ref_ptr<LightPass> dirState_;
  ref_ptr<LightPass> dirShadowState_;
  ref_ptr<LightPass> pointState_;
  ref_ptr<LightPass> pointShadowState_;
  ref_ptr<LightPass> spotState_;
  ref_ptr<LightPass> spotShadowState_;

  ref_ptr<FullscreenPass> ambientState_;
  ref_ptr<ShaderInput3f> ambientLight_;
  GLboolean hasAmbient_;

  ref_ptr<AmbientOcclusion> updateAOState_;
  ref_ptr<FullscreenPass> aoState_;
  GLboolean hasAO_;

  ref_ptr<LightPass> getLightState(const ref_ptr<Light>&, const ref_ptr<ShadowMap>&);
  void createLightStateShader(const ref_ptr<LightPass> &light);

  void removeLight(Light *l, const ref_ptr<LightPass> &lightState);
};
} // namespace

#endif /* __SHADING_DEFERRED_H_ */
