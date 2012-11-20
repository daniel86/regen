/*
 * directional-shadow-map.h
 *
 *  Created on: 20.11.2012
 *      Author: daniel
 */

#ifndef DIRECTIONAL_SHADOW_MAP_H_
#define DIRECTIONAL_SHADOW_MAP_H_

#include <ogle/algebra/frustum.h>
#include <ogle/animations/animation.h>
#include <ogle/states/light-state.h>
#include <ogle/states/camera.h>
#include <ogle/gl-types/fbo.h>
#include <ogle/gl-types/volume-texture.h>

/**
 * Implements Parallel Split Shadow Mapping / Cascade Shadow Mapping
 * @see ...
 */
class DirectionalShadowMap : public Animation
{
public:
  DirectionalShadowMap(
      ref_ptr<DirectionalLight> &light,
      ref_ptr<Frustum> &sceneFrustum,
      ref_ptr<PerspectiveCamera> &sceneCamera,
      GLuint numSplits,
      GLuint shadowMapSize);
  ~DirectionalShadowMap();

  /**
   * Should be called when the light direction changed.
   */
  void updateLightDirection();
  /**
   * Should be called when the scene projection matrix changed.
   */
  void updateProjection();

  // override
  virtual void animate(GLdouble dt);
  virtual void updateGraphics(GLdouble dt);

protected:
  // shadow casting light
  ref_ptr<DirectionalLight> light_;
  // main camera
  ref_ptr<PerspectiveCamera> sceneCamera_;

  // number of frustum splits
  GLuint numSplits_;

  // scene frustum and splits
  ref_ptr<Frustum> sceneFrustum_;
  vector<Frustum*> shadowFrusta_;

  // render target
  GLuint fbo_;
  ref_ptr<DepthTexture3D> texture_;

  // shadow map update uniforms
  Mat4f modelViewMatrix_;
  ref_ptr<ShaderInputMat4> mvpMatrixUniform_;
  // sampling uniforms
  ref_ptr<ShaderInputMat4> shadowMatUniform_;
  ref_ptr<ShaderInput1f> shadowFarUniform_;
};

#endif /* DIRECTIONAL_SHADOW_MAP_H_ */
