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

class PointShadowMap : public Animation
{
public:
  PointShadowMap(
      ref_ptr<PointLight> &light,
      ref_ptr<PerspectiveCamera> &sceneCamera,
      GLuint shadowMapSize);

  void updateLight();

  void addCaster(ref_ptr<StateNode> &caster);
  void removeCaster(StateNode *caster);

  void drawDebugHUD();

  ref_ptr<ShaderInputMat4>& shadowMatUniform();
  ref_ptr<TextureState>& shadowMap();

  // override
  virtual void animate(GLdouble dt);
  virtual void updateGraphics(GLdouble dt);

protected:
  // shadow casting light
  ref_ptr<PointLight> light_;
  // main camera
  ref_ptr<PerspectiveCamera> sceneCamera_;
  GLenum compareMode_;

  list< ref_ptr<StateNode> > caster_;

  // render target
  GLuint fbo_;
  ref_ptr<CubeMapDepthTexture> texture_;

  // shadow map update uniforms
  Mat4f *viewMatrices_;
  Mat4f projectionMatrix_;
  // sampling uniforms
  ref_ptr<ShaderInputMat4> shadowMatUniform_;
  ref_ptr<TextureState> shadowMap_;
};

#endif /* POINT_SHADOW_MAP_H_ */
