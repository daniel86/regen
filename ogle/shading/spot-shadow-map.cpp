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
using namespace ogle;

SpotShadowMap::SpotShadowMap(
    const ref_ptr<SpotLight> &light,
    const ref_ptr<Camera> &sceneCamera,
    GLuint shadowMapSize,
    GLenum depthFormat,
    GLenum depthType)
: ShadowMap(ref_ptr<Light>::cast(light), GL_TEXTURE_2D,
    shadowMapSize, 1, depthFormat, depthType),
  spotLight_(light),
  sceneCamera_(sceneCamera)
{
  // uniforms for shadow sampling
  shadowFarUniform_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("shadowFar"));
  shadowFarUniform_->setUniformData(200.0f);
  setInput(ref_ptr<ShaderInput>::cast(shadowFarUniform_));

  shadowNearUniform_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("shadowNear"));
  shadowNearUniform_->setUniformData(0.1f);
  setInput(ref_ptr<ShaderInput>::cast(shadowNearUniform_));

  shadowMatUniform_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4("shadowMatrix"));
  shadowMatUniform_->setUniformDataUntyped(NULL);
  setInput(ref_ptr<ShaderInput>::cast(shadowMatUniform_));

  updateLight();
}

const ref_ptr<ShaderInput1f>& SpotShadowMap::near() const
{
  return shadowNearUniform_;
}
const ref_ptr<ShaderInput1f>& SpotShadowMap::far() const
{
  return shadowFarUniform_;
}
const ref_ptr<ShaderInputMat4>& SpotShadowMap::shadowMatUniform() const
{
  return shadowMatUniform_;
}

void SpotShadowMap::updateLight()
{
  const Vec3f &pos = spotLight_->position()->getVertex3f(0);
  const Vec3f &dir = spotLight_->spotDirection()->getVertex3f(0);
  const Vec2f &a = light_->radius()->getVertex2f(0);
  shadowFarUniform_->setVertex1f(0, a.y);

  viewMatrix_ = Mat4f::lookAtMatrix(pos, dir, Vec3f::up());

  const Vec2f &coneAngle = spotLight_->coneAngle()->getVertex2f(0);
  projectionMatrix_ = Mat4f::projectionMatrix(
      2.0*360.0*acos(coneAngle.y)/(2.0*M_PI), 1.0f,
      shadowNearUniform_->getVertex1f(0),
      shadowFarUniform_->getVertex1f(0));
  viewProjectionMatrix_ = viewMatrix_ * projectionMatrix_;
  // transforms world space coordinates to homogenous light space
  shadowMatUniform_->setVertex16f(0, viewProjectionMatrix_ * Mat4f::bias());

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

void SpotShadowMap::computeDepth(RenderState *rs)
{
  sceneCamera_->position()->pushData(spotLight_->position()->dataPtr());
  sceneCamera_->view()->pushData((byte*) viewMatrix_.x);
  sceneCamera_->projection()->pushData((byte*) projectionMatrix_.x);
  sceneCamera_->viewProjection()->pushData((byte*) viewProjectionMatrix_.x);

  glClear(GL_DEPTH_BUFFER_BIT);
  traverse(rs);

  sceneCamera_->view()->popData();
  sceneCamera_->projection()->popData();
  sceneCamera_->viewProjection()->popData();
  sceneCamera_->position()->popData();
}

void SpotShadowMap::computeMoment(RenderState *rs)
{
  momentsCompute_->enable(rs);
  shadowNearUniform_->enableUniform(momentsNear_);
  shadowFarUniform_->enableUniform(momentsFar_);
  textureQuad_->draw(1);
  momentsCompute_->disable(rs);
}
