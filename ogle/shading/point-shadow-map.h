/*
 * point-shadow-map.h
 *
 *  Created on: 20.11.2012
 *      Author: daniel
 */

#ifndef POINT_SHADOW_MAP_H_
#define POINT_SHADOW_MAP_H_

#include <ogle/algebra/frustum.h>
#include <ogle/states/light-state.h>
#include <ogle/states/camera.h>
#include <ogle/states/state-node.h>
#include <ogle/gl-types/fbo.h>
#include <ogle/shading/shadow-map.h>

namespace ogle {

/**
 * Simple implementation of omnidirectional shadow mapping using
 * a depth cubemap.
 * Added geometry is processed 6 times (by 6 draw calls or by a geometry
 * shader with 6 invocations) to update the cube faces.
 * The layer ordering is +x,-x,+y,-y,+z,-z .
 */
class PointShadowMap : public ShadowMap
{
public:
  PointShadowMap(
      const ref_ptr<PointLight> &light,
      const ref_ptr<PerspectiveCamera> &sceneCamera,
      GLuint shadowMapSize,
      GLenum internalFormat,
      GLenum pixelType);
  ~PointShadowMap();

  /**
   * Discard specified cube faces.
   */
  void set_isFaceVisible(GLenum face, GLboolean visible);
  /**
   * Discard specified cube faces?
   */
  GLboolean isFaceVisible(GLenum face);

  /**
   * Point light attenuation is used to optimize z precision.
   * farAttenuation is the attenuation threshold that is used
   * to compute the far value.
   */
  void set_farAttenuation(GLfloat farAttenuation);
  /**
   * Point light attenuation is used to optimize z precision.
   * farAttenuation is the attenuation threshold that is used
   * to compute the far value.
   */
  GLfloat farAttenuation() const;

  /**
   * Hard limit for the far value used to optimize z precision.
   */
  void set_farLimit(GLfloat farLimit);
  /**
   * Hard limit for the far value used to optimize z precision.
   */
  GLfloat farLimit() const;

  void set_near(GLfloat near);
  const ref_ptr<ShaderInput1f>& near() const;
  const ref_ptr<ShaderInput1f>& far() const;

  // override
  virtual void update();
  virtual void computeDepth(RenderState *rs);
  virtual void computeMoment(RenderState *rs);
  virtual GLenum samplerType() { return GL_TEXTURE_CUBE_MAP; }

protected:
  // shadow casting light
  ref_ptr<PointLight> pointLight_;
  // main camera
  ref_ptr<PerspectiveCamera> sceneCamera_;

  ref_ptr<ShaderInput1f> shadowFarUniform_;
  ref_ptr<ShaderInput1f> shadowNearUniform_;

  GLfloat farAttenuation_;
  GLfloat farLimit_;

  // shadow map update uniforms
  Mat4f projectionMatrix_;
  Mat4f *viewMatrices_;
  Mat4f viewProjectionMatrices_[6];

  GLboolean isFaceVisible_[6];

  GLuint lightPosStamp_;
  GLuint lightRadiusStamp_;
  void updateLight();
};

} // end ogle namespace

#endif /* POINT_SHADOW_MAP_H_ */
