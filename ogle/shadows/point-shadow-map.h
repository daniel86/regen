/*
 * point-shadow-map.h
 *
 *  Created on: 20.11.2012
 *      Author: daniel
 */

#ifndef POINT_SHADOW_MAP_H_
#define POINT_SHADOW_MAP_H_

#include <ogle/algebra/frustum.h>
#include <ogle/animations/animation.h>
#include <ogle/states/light-state.h>
#include <ogle/states/camera.h>
#include <ogle/render-tree/state-node.h>
#include <ogle/gl-types/fbo.h>
#include <ogle/gl-types/volume-texture.h>
#include <ogle/shadows/shadow-map.h>

/**
 * Simple implementation of omnidirectional shadow mapping using
 * a depth cubemap.
 * Added geometry is processed 6 times (by 6 draw calls or by a geometry
 * shader with 6 invocations) to update the cube faces.
 */
class PointShadowMap : public ShadowMap
{
public:
  PointShadowMap(
      ref_ptr<PointLight> &light,
      ref_ptr<PerspectiveCamera> &sceneCamera,
      GLuint shadowMapSize,
      GLuint maxNumBones,
      GLenum internalFormat=GL_DEPTH_COMPONENT24,
      GLenum pixelType=GL_FLOAT);
  ~PointShadowMap();

  /**
   * Point light attenuation is used to optimize z precision.
   * farAttenuation is the attenuation threshold that is used
   * to compute the far value.
   */
  void set_farAttenuation(GLfloat farAttenuation);
  GLfloat farAttenuation() const;

  /**
   * Hard limit for the far value used to optimize z precision.
   */
  void set_farLimit(GLfloat farLimit);
  GLfloat farLimit() const;

  void set_near(GLfloat near);
  GLfloat near() const;

  void updateLight();

  ref_ptr<ShaderInputMat4>& shadowMatUniform();

  // override
  virtual void updateGraphics(GLdouble dt);

protected:
  // shadow casting light
  ref_ptr<PointLight> pointLight_;
  // main camera
  ref_ptr<PerspectiveCamera> sceneCamera_;

  GLenum compareMode_;
  GLfloat farAttenuation_;
  GLfloat farLimit_;
  GLfloat near_;

  ShadowRenderState *rs_;
  // shadow map update uniforms
  Mat4f projectionMatrix_;
  Mat4f *viewMatrices_;
  Mat4f *viewProjectionMatrices_;
  // sampling uniforms
  ref_ptr<ShaderInputMat4> shadowMatUniform_;
};

#endif /* POINT_SHADOW_MAP_H_ */
