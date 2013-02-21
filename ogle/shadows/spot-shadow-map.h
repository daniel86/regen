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
#include <ogle/states/state-node.h>
#include <ogle/gl-types/fbo.h>
#include <ogle/gl-types/texture.h>
#include <ogle/shadows/shadow-map.h>

class SpotShadowMap : public ShadowMap
{
public:
  SpotShadowMap(
      const ref_ptr<SpotLight> &light,
      const ref_ptr<PerspectiveCamera> &sceneCamera,
      GLuint shadowMapSize,
      GLenum internalFormat=GL_DEPTH_COMPONENT24,
      GLenum pixelType=GL_UNSIGNED_BYTE);

  const ref_ptr<ShaderInput1f>& near() const;
  const ref_ptr<ShaderInput1f>& far() const;

  const ref_ptr<ShaderInputMat4>& shadowMatUniform() const;

  // override
  virtual void update();
  virtual void computeDepth();
  virtual void computeMoment();
  virtual GLenum samplerType() { return GL_TEXTURE_2D; }

protected:
  // shadow casting light
  ref_ptr<SpotLight> spotLight_;
  // main camera
  ref_ptr<PerspectiveCamera> sceneCamera_;

  ref_ptr<ShaderInput1f> shadowFarUniform_;
  ref_ptr<ShaderInput1f> shadowNearUniform_;

  // shadow map update uniforms
  Mat4f viewMatrix_;
  Mat4f projectionMatrix_;
  // sampling uniforms
  ref_ptr<ShaderInputMat4> shadowMatUniform_;

  GLuint lightPosStamp_;
  GLuint lightDirStamp_;
  GLuint lightRadiusStamp_;
  void updateLight();
};

#endif /* SPOT_SHADOW_MAP_H_ */
