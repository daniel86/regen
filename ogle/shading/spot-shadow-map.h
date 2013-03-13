/*
 * spot-shadow-map.h
 *
 *  Created on: 20.11.2012
 *      Author: daniel
 */

#ifndef SPOT_SHADOW_MAP_H_
#define SPOT_SHADOW_MAP_H_

#include <ogle/shading/light-state.h>
#include <ogle/states/camera.h>
#include <ogle/states/state-node.h>
#include <ogle/gl-types/fbo.h>
#include <ogle/gl-types/texture.h>
#include <ogle/shading/shadow-map.h>

namespace ogle {

class SpotShadowMap : public ShadowMap
{
public:
  SpotShadowMap(
      const ref_ptr<Light> &light,
      const ref_ptr<Camera> &sceneCamera,
      GLuint shadowMapSize,
      GLenum internalFormat,
      GLenum pixelType);

  const ref_ptr<ShaderInput1f>& near() const;
  const ref_ptr<ShaderInput1f>& far() const;

  const ref_ptr<ShaderInputMat4>& shadowMatUniform() const;

  // override
  virtual void update();
  virtual void computeDepth(RenderState *rs);
  virtual void computeMoment(RenderState *rs);
  virtual GLenum samplerType() { return GL_TEXTURE_2D; }

protected:
  // shadow casting light
  ref_ptr<Light> spotLight_;
  // main camera
  ref_ptr<Camera> sceneCamera_;

  ref_ptr<ShaderInput1f> shadowFarUniform_;
  ref_ptr<ShaderInput1f> shadowNearUniform_;

  // shadow map update uniforms
  Mat4f viewMatrix_;
  Mat4f projectionMatrix_;
  Mat4f viewProjectionMatrix_;
  // sampling uniforms
  ref_ptr<ShaderInputMat4> shadowMatUniform_;

  GLuint lightPosStamp_;
  GLuint lightDirStamp_;
  GLuint lightRadiusStamp_;
  void updateLight();
};

} // end ogle namespace

#endif /* SPOT_SHADOW_MAP_H_ */
