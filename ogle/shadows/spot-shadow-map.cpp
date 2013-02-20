/*
 * spot-shadow-map.cpp
 *
 *  Created on: 20.11.2012
 *      Author: daniel
 */

#include <ogle/utility/string-util.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/shader-state.h>

#include "spot-shadow-map.h"

SpotShadowMap::SpotShadowMap(
    const ref_ptr<SpotLight> &light,
    const ref_ptr<PerspectiveCamera> &sceneCamera,
    GLuint shadowMapSize,
    GLenum depthFormat,
    GLenum depthType)
: ShadowMap(ref_ptr<Light>::cast(light), shadowMapSize),
  spotLight_(light),
  sceneCamera_(sceneCamera),
  farAttenuation_(0.01f),
  farLimit_(200.0f),
  near_(0.1f)
{
  // stores depth values from light perspective
  ref_ptr<Texture> depthTexture = ref_ptr<Texture>::manage(new DepthTexture2D);
  depthTexture->set_internalFormat(depthFormat);
  depthTexture->set_pixelType(depthType);
  depthTexture->set_targetType(GL_TEXTURE_2D);
  set_depthTexture(depthTexture, GL_COMPARE_R_TO_TEXTURE, "sampler2DShadow");

  // uniforms for shadow sampling
  shadowMatUniform_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4(
      FORMAT_STRING("shadowMatrix"<<light->id())));
  shadowMatUniform_->setUniformDataUntyped(NULL);

  updateLight();
}

const ref_ptr<ShaderInputMat4>& SpotShadowMap::shadowMatUniform() const
{
  return shadowMatUniform_;
}

void SpotShadowMap::set_farAttenuation(GLfloat farAttenuation)
{
  farAttenuation_ = farAttenuation;
}
GLfloat SpotShadowMap::farAttenuation() const
{
  return farAttenuation_;
}
void SpotShadowMap::set_farLimit(GLfloat farLimit)
{
  farLimit_ = farLimit;
}
GLfloat SpotShadowMap::farLimit() const
{
  return farLimit_;
}
void SpotShadowMap::set_near(GLfloat near)
{
  near_ = near;
}
GLfloat SpotShadowMap::near() const
{
  return near_;
}

void SpotShadowMap::updateLight()
{
  const Vec3f &pos = spotLight_->position()->getVertex3f(0);
  const Vec3f &dir = spotLight_->spotDirection()->getVertex3f(0);
  const Vec2f &a = light_->radius()->getVertex2f(0);

  viewMatrix_ = Mat4f::lookAtMatrix(pos, dir, UP_VECTOR);

  // adjust far value for better precision
  GLfloat far = a.y;
  // hard limit z range
  if(farLimit_>0.0 && far>farLimit_) far=farLimit_;

  const Vec2f &coneAngle = spotLight_->coneAngle()->getVertex2f(0);
  projectionMatrix_ = Mat4f::projectionMatrix(
      2.0*360.0*acos(coneAngle.y)/(2.0*M_PI), 1.0f, near_, far);
  // transforms world space coordinates to homogenous light space
  shadowMatUniform_->getVertex16f(0) = viewMatrix_ * projectionMatrix_ * biasMatrix_;

  lightPosStamp_ = spotLight_->position()->stamp();
  lightDirStamp_ = spotLight_->spotDirection()->stamp();
  lightRadiusStamp_ = spotLight_->radius()->stamp();
}
void SpotShadowMap::update()
{
  if(lightPosStamp_ != spotLight_->position()->stamp() ||
      lightDirStamp_ != spotLight_->spotDirection()->stamp() ||
      lightRadiusStamp_ != spotLight_->radius()->stamp())
  {
    updateLight();
  }
}

void SpotShadowMap::computeDepth()
{
  glDrawBuffer(GL_NONE);
  glClear(GL_DEPTH_BUFFER_BIT);

  Mat4f &view = sceneCamera_->viewUniform()->getVertex16f(0);
  Mat4f &proj = sceneCamera_->projectionUniform()->getVertex16f(0);
  //Mat4f &viewproj = sceneCamera_->viewProjectionUniform()->getVertex16f(0);
  Mat4f sceneView = view;
  Mat4f sceneProj = proj;
  //Mat4f sceneViewProj = viewproj;
  view = viewMatrix_;
  proj = projectionMatrix_;
  //viewproj = viewProjectionMatrix_;

  traverse(&depthRenderState_);

  view = sceneView;
  proj = sceneProj;
  //viewproj = sceneViewProj
}

void SpotShadowMap::computeMoment()
{

}
