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

//#define DEBUG_SHADOW_MAPS
//#define USE_LAYERED_SHADER

SpotShadowMap::SpotShadowMap(
    ref_ptr<SpotLight> &light,
    ref_ptr<PerspectiveCamera> &sceneCamera,
    GLuint shadowMapSize,
    GLenum internalFormat,
    GLenum pixelType)
: ShadowMap(ref_ptr<Light>::cast(light), ref_ptr<Texture>::manage(new DepthTexture2D)),
  spotLight_(light),
  sceneCamera_(sceneCamera),
  compareMode_(GL_COMPARE_R_TO_TEXTURE),
  farAttenuation_(0.01f),
  farLimit_(200.0f),
  near_(0.1f)
{
  // on nvidia linear filtering gives 2x2 PCF for 'free'
  texture_->set_filter(GL_LINEAR,GL_LINEAR);
  texture_->set_internalFormat(internalFormat);
  texture_->set_pixelType(pixelType);
  texture_->set_size(shadowMapSize, shadowMapSize);
  texture_->set_compare(compareMode_, GL_LEQUAL);
  texture_->set_samplerType("sampler2DShadow");
  texture_->texImage();
  shadowMapSize_->setUniformData((float)shadowMapSize);

  // uniforms for shadow sampling
  shadowMatUniform_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4(
      FORMAT_STRING("shadowMatrix"<<light->id())));
  shadowMatUniform_->setInstanceData(1, 1, NULL);

#ifdef USE_LAYERED_SHADER
  rs_ = new LayeredShadowRenderState(ref_ptr<Texture>::cast(texture_), maxNumBones, 1);
#else
  rs_ = new ShadowRenderState(ref_ptr<Texture>::cast(texture_));
#endif

  light_->joinShaderInput(ref_ptr<ShaderInput>::cast(shadowMatUniform()));

  updateLight();
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

ref_ptr<ShaderInputMat4>& SpotShadowMap::shadowMatUniform()
{
  return shadowMatUniform_;
}

void SpotShadowMap::updateLight()
{
  const Vec3f &pos = spotLight_->position()->getVertex3f(0);
  const Vec3f &dir = spotLight_->spotDirection()->getVertex3f(0);
  viewMatrix_ = getLookAtMatrix(pos, dir, UP_VECTOR);

  // adjust far value for better precision
  GLfloat far;
  {
    // find far value where light attenuation reaches threshold,
    // insert farAttenuation in the attenuation equation and solve
    // equation for distance
    const Vec3f &a = light_->attenuation()->getVertex3f(0);
    GLdouble p2 = a.y/(2.0*a.z);
    far = -p2 + sqrt(p2*p2 - (a.x/farAttenuation_ - 1.0/(farAttenuation_*a.z)));
  }
  // hard limit z range
  if(farLimit_>0.0 && far>farLimit_) far=farLimit_;

  const Vec2f &coneAngle = spotLight_->coneAngle()->getVertex2f(0);
  projectionMatrix_ = projectionMatrix(
      2.0*360.0*acos(coneAngle.x)/(2.0*M_PI), 1.0f, near_, far);
  // transforms world space coordinates to homogenous light space
  shadowMatUniform_->getVertex16f(0) = viewMatrix_ * projectionMatrix_ * biasMatrix_;
}

void SpotShadowMap::updateGraphics(GLdouble dt)
{
  enable(rs_);
  rs_->enable();

#ifdef USE_LAYERED_SHADER
  rs_->set_shadowViewProjectionMatrices(viewProjectionMatrices_);
  traverse(rs_);
#else
  ShaderInputMat4 *u_view = sceneCamera_->viewUniform().get();
  ShaderInputMat4 *u_proj = sceneCamera_->projectionUniform();
  byte *sceneView = u_view->dataPtr();
  byte *sceneProj = u_proj->dataPtr();
  u_view->set_dataPtr((byte*)viewMatrix_.x);
  u_proj->set_dataPtr((byte*)projectionMatrix_.x);
  traverse(rs_);
  u_view->set_dataPtr(sceneView);
  u_proj->set_dataPtr(sceneProj);
#endif

  disable(rs_);

#ifdef DEBUG_SHADOW_MAPS
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  drawDebugHUD(
      GL_TEXTURE_2D,
      GL_COMPARE_R_TO_TEXTURE,
      1u,
      texture_->id(),
      "shadow-mapping.debugSpot.fs");
#endif
}
