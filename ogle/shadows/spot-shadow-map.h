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
      GLuint shadowMapSize,
      GLenum internalFormat=GL_DEPTH_COMPONENT24,
      GLenum pixelType=GL_FLOAT);

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

  /**
   * Should be called when the light direction changed.
   */
  void updateLight();

  ref_ptr<ShaderInputMat4>& shadowMatUniform();

  // override
  virtual void updateGraphics(GLdouble dt);

protected:
  // shadow casting light
  ref_ptr<SpotLight> spotLight_;
  // main camera
  ref_ptr<PerspectiveCamera> sceneCamera_;

  GLenum compareMode_;
  GLfloat farAttenuation_;
  GLfloat farLimit_;
  GLfloat near_;

  ShadowRenderState *rs_;

  // shadow map update uniforms
  Mat4f viewMatrix_;
  Mat4f projectionMatrix_;
  // sampling uniforms
  ref_ptr<ShaderInputMat4> shadowMatUniform_;
};

#endif /* SPOT_SHADOW_MAP_H_ */
