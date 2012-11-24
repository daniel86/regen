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
#include <ogle/render-tree/state-node.h>
#include <ogle/gl-types/fbo.h>
#include <ogle/gl-types/volume-texture.h>
#include <ogle/shadows/shadow-map.h>

/**
 * Implements Parallel Split Shadow Mapping / Cascade Shadow Mapping
 * @see http://http.developer.nvidia.com/GPUGems3/gpugems3_ch10.html
 */
class DirectionalShadowMap : public ShadowMap
{
public:
  DirectionalShadowMap(
      ref_ptr<DirectionalLight> &light,
      ref_ptr<Frustum> &sceneFrustum,
      ref_ptr<PerspectiveCamera> &sceneCamera,
      GLuint shadowMapSize,
      GLdouble splitWeight=0.75);
  ~DirectionalShadowMap();

  static void set_numSplits(GLuint numSplits);
  static GLuint numSplits();

  void set_splitWeight(GLdouble splitWeight);
  GLuint splitWeight();

  /**
   * Should be called when the light direction changed.
   */
  void updateLightDirection();
  /**
   * Should be called when the scene projection matrix changed.
   */
  void updateProjection();
  void updateCamera();

  void drawDebugHUD();

  ref_ptr<ShaderInputMat4>& shadowMatUniform();
  ref_ptr<ShaderInput1f>& shadowFarUniform();
  ref_ptr<TextureState>& shadowMap();

  // override
  virtual void updateShadow();

protected:
  // shadow casting light
  ref_ptr<DirectionalLight> light_;
  // main camera
  ref_ptr<PerspectiveCamera> sceneCamera_;

  GLdouble splitWeight_;
  // number of frustum splits
  static GLuint numSplits_;

  // scene frustum and splits
  ref_ptr<Frustum> sceneFrustum_;
  vector<Frustum*> shadowFrusta_;

  // render target
  GLuint fbo_;
  ref_ptr<DepthTexture3D> texture_;

  // shadow map update uniforms
  Mat4f viewMatrix_;
  Mat4f *projectionMatrices_;
  // sampling uniforms
  ref_ptr<ShaderInputMat4> shadowMatUniform_;
  ref_ptr<ShaderInput1f> shadowFarUniform_;
  ref_ptr<TextureState> shadowMap_;
};

#endif /* DIRECTIONAL_SHADOW_MAP_H_ */
