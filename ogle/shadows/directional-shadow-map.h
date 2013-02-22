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
#include <ogle/states/state-node.h>
#include <ogle/gl-types/fbo.h>
#include <ogle/shadows/shadow-map.h>

/**
 * Implements Parallel Split Shadow Mapping aka Cascade Shadow Mapping.
 * @see http://http.developer.nvidia.com/GPUGems3/gpugems3_ch10.html
 */
class DirectionalShadowMap : public ShadowMap
{
public:
  DirectionalShadowMap(
      const ref_ptr<DirectionalLight> &light,
      const ref_ptr<Frustum> &sceneFrustum,
      const ref_ptr<PerspectiveCamera> &sceneCamera,
      GLuint shadowMapSize,
      GLuint numShadowLayer,
      GLdouble splitWeight,
      GLenum internalFormat,
      GLenum pixelType);
  ~DirectionalShadowMap();

  /**
   * Sets the number of texture layers aka. the depth
   * of the shadow map.
   */
  void set_numShadowLayer(GLuint numSplits);
  GLuint numShadowLayer() const;

  /**
   * Weight for exponential split scheme.
   */
  void set_splitWeight(GLdouble splitWeight);
  /**
   * Weight for exponential split scheme.
   */
  GLdouble splitWeight() const;

  /**
   * Shadow camera matrix for each split.
   */
  const ref_ptr<ShaderInputMat4>& shadowMatUniform() const;
  /**
   * Far value for each split.
   */
  const ref_ptr<ShaderInput1f>& shadowFarUniform() const;

  /**
   * Should be called when the scene projection matrix changed.
   */
  void updateProjection();

  // override
  virtual void update();
  virtual void computeDepth();
  virtual void computeMoment();
  virtual GLenum samplerType() { return GL_TEXTURE_2D_ARRAY; }

protected:
  // number of frustum splits
  GLuint numShadowLayer_;

  // scene frustum and splits
  ref_ptr<Frustum> sceneFrustum_;
  vector<Frustum*> shadowFrusta_;
  GLdouble splitWeight_;

  // shadow casting light
  ref_ptr<DirectionalLight> dirLight_;
  // main camera
  ref_ptr<PerspectiveCamera> sceneCamera_;

  // shadow map update uniforms
  Mat4f viewMatrix_;
  Mat4f *projectionMatrices_;
  Mat4f *viewProjectionMatrices_;
  // sampling uniforms
  ref_ptr<ShaderInputMat4> shadowMatUniform_;
  ref_ptr<ShaderInput1f> shadowFarUniform_;

  GLuint lightDirectionStamp_;
  void updateLightDirection();

  void updateCamera();
};

#endif /* DIRECTIONAL_SHADOW_MAP_H_ */
