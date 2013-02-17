/*
 * point-shadow-map.cpp
 *
 *  Created on: 20.11.2012
 *      Author: daniel
 */

#include <ogle/utility/string-util.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/shader-state.h>

#include "point-shadow-map.h"

//#define DEBUG_SHADOW_MAPS
//#define USE_LAYERED_SHADER

PointShadowMap::PointShadowMap(
    const ref_ptr<PointLight> &light,
    const ref_ptr<PerspectiveCamera> &sceneCamera,
    GLuint shadowMapSize,
    GLenum internalFormat,
    GLenum pixelType)
: ShadowMap(ref_ptr<Light>::cast(light), ref_ptr<Texture>::manage(new CubeMapDepthTexture)),
  pointLight_(light),
  sceneCamera_(sceneCamera),
  compareMode_(GL_COMPARE_R_TO_TEXTURE),
  farAttenuation_(0.01f),
  farLimit_(200.0f)
{
  shadowMap_->set_samplerType("samplerCubeShadow");
  // on nvidia linear filtering gives 2x2 PCF for 'free'
  texture_->set_filter(GL_LINEAR,GL_LINEAR);
  texture_->set_internalFormat(internalFormat);
  texture_->set_pixelType(pixelType);
  texture_->set_size(shadowMapSize, shadowMapSize);
  texture_->set_compare(compareMode_, GL_LEQUAL);
  texture_->texImage();
  shadowMapSize_->setUniformData((float)shadowMapSize);

  shadowFarUniform_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f(
      FORMAT_STRING("shadowFar"<<light->id())));
  shadowFarUniform_->setUniformData(200.0f);

  shadowNearUniform_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f(
      FORMAT_STRING("shadowNear"<<light->id())));
  shadowNearUniform_->setUniformData(0.1f);

  for(GLuint i=0; i<6; ++i) { isFaceVisible_[i] = GL_TRUE; }

  rs_ = new ShadowRenderState(ref_ptr<Texture>::cast(texture_));

  updateLight();
}
PointShadowMap::~PointShadowMap()
{
  if(viewMatrices_) { delete []viewMatrices_; }
  delete rs_;
}

void PointShadowMap::set_near(GLfloat near)
{
  shadowNearUniform_->setVertex1f(0, near);
}
const ref_ptr<ShaderInput1f>& PointShadowMap::near() const
{
  return shadowNearUniform_;
}
const ref_ptr<ShaderInput1f>& PointShadowMap::far() const
{
  return shadowFarUniform_;
}

void PointShadowMap::set_isFaceVisible(GLenum face, GLboolean visible)
{
  isFaceVisible_[face - GL_TEXTURE_CUBE_MAP_POSITIVE_X] = visible;
}
GLboolean PointShadowMap::isFaceVisible(GLenum face)
{
  return isFaceVisible_[face - GL_TEXTURE_CUBE_MAP_POSITIVE_X];
}

void PointShadowMap::set_farAttenuation(GLfloat farAttenuation)
{
  farAttenuation_ = farAttenuation;
}
GLfloat PointShadowMap::farAttenuation() const
{
  return farAttenuation_;
}
void PointShadowMap::set_farLimit(GLfloat farLimit)
{
  farLimit_ = farLimit;
}
GLfloat PointShadowMap::farLimit() const
{
  return farLimit_;
}

void PointShadowMap::updateLight()
{
  const Vec3f &pos = pointLight_->position()->getVertex3f(0);
  const Vec2f &a = light_->radius()->getVertex2f(0);

  // adjust far value for better precision
  GLfloat far = a.y;
  if(farLimit_>0.0f && far>farLimit_) far=farLimit_;
  shadowFarUniform_->setVertex1f(0, far);

  projectionMatrix_ = Mat4f::projectionMatrix(
      90.0, 1.0f, near()->getVertex1f(0), far);
  viewMatrices_ = getCubeLookAtMatrices(pos);

  for(register GLuint i=0; i<6; ++i) {
    if(!isFaceVisible_[i]) { continue; }
    viewProjectionMatrices_[i] = viewMatrices_[i] * projectionMatrix_;
  }

  lightPosStamp_ = pointLight_->position()->stamp();
  lightRadiusStamp_ = pointLight_->radius()->stamp();
}

void PointShadowMap::glAnimate(GLdouble dt)
{
  if(lightPosStamp_ != pointLight_->position()->stamp() ||
      lightRadiusStamp_ != pointLight_->radius()->stamp())
  {
    updateLight();
  }

  enable(rs_);
  rs_->enable();

  Mat4f &view = sceneCamera_->viewUniform()->getVertex16f(0);
  Mat4f &proj = sceneCamera_->projectionUniform()->getVertex16f(0);
  Mat4f &viewproj = sceneCamera_->viewProjectionUniform()->getVertex16f(0);
  Mat4f sceneView = view;
  Mat4f sceneProj = proj;
  Mat4f sceneViewProj = viewproj;
  proj = projectionMatrix_;

  for(register GLuint i=0; i<6; ++i) {
    if(!isFaceVisible_[i]) { continue; }
    glFramebufferTexture2D(GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,
        texture_->id(), 0);
    view = viewMatrices_[i];
    viewproj = viewProjectionMatrices_[i];
    traverse(rs_);
  }

  view = sceneView;
  proj = sceneProj;
  viewproj = sceneViewProj;

  disable(rs_);

#ifdef DEBUG_SHADOW_MAPS
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  drawDebugHUD(
      GL_TEXTURE_CUBE_MAP,
      GL_COMPARE_R_TO_TEXTURE,
      6,
      texture_->id(),
      "shadow-mapping.debugPoint.fs");
#endif
}
