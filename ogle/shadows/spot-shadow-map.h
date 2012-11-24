/*
 * spot-shadow-map.h
 *
 *  Created on: 20.11.2012
 *      Author: daniel
 */

#ifndef SPOT_SHADOW_MAP_H_
#define SPOT_SHADOW_MAP_H_

#include <ogle/animations/animation.h>
#include <ogle/states/light-state.h>
#include <ogle/states/camera.h>
#include <ogle/render-tree/state-node.h>
#include <ogle/gl-types/fbo.h>
#include <ogle/gl-types/texture.h>
#include <ogle/shadows/shadow-map.h>

class SpotShadowMap : public ShadowMap
{
public:
  SpotShadowMap(
      ref_ptr<SpotLight> &light,
      ref_ptr<PerspectiveCamera> &sceneCamera,
      GLuint shadowMapSize);

  /**
   * Should be called when the light direction changed.
   */
  void updateLight();

  ref_ptr<ShaderInputMat4>& shadowMatUniform();
  ref_ptr<TextureState>& shadowMap();

  // override
  virtual void updateShadow();

protected:
  // shadow casting light
  ref_ptr<SpotLight> light_;
  // main camera
  ref_ptr<PerspectiveCamera> sceneCamera_;

  GLenum compareMode_;

  // render target
  GLuint fbo_;
  ref_ptr<DepthTexture2D> texture_;

  // shadow map update uniforms
  Mat4f viewMatrix_;
  Mat4f projectionMatrix_;
  // sampling uniforms
  ref_ptr<ShaderInputMat4> shadowMatUniform_;
  ref_ptr<TextureState> shadowMap_;
};

#endif /* SPOT_SHADOW_MAP_H_ */
