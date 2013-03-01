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

PointShadowMap::PointShadowMap(
    const ref_ptr<PointLight> &light,
    const ref_ptr<PerspectiveCamera> &sceneCamera,
    GLuint shadowMapSize,
    GLenum depthFormat,
    GLenum depthType)
: ShadowMap(ref_ptr<Light>::cast(light), GL_TEXTURE_CUBE_MAP,
    shadowMapSize, 1, depthFormat, depthType),
  pointLight_(light),
  sceneCamera_(sceneCamera),
  farAttenuation_(0.01f),
  farLimit_(200.0f)
{
  shadowFarUniform_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f(
      FORMAT_STRING("shadowFar"<<light->id())));
  shadowFarUniform_->setUniformData(200.0f);

  shadowNearUniform_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f(
      FORMAT_STRING("shadowNear"<<light->id())));
  shadowNearUniform_->setUniformData(0.1f);

  for(GLuint i=0; i<6; ++i) { isFaceVisible_[i] = GL_TRUE; }

  updateLight();
}
PointShadowMap::~PointShadowMap()
{
  if(viewMatrices_) { delete []viewMatrices_; }
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
void PointShadowMap::update()
{
  if(lightPosStamp_ != pointLight_->position()->stamp() ||
      lightRadiusStamp_ != pointLight_->radius()->stamp())
  {
    updateLight();
  }
}

void PointShadowMap::computeDepth(RenderState *rs)
{
  sceneCamera_->projectionUniform()->pushData((byte*)projectionMatrix_.x);
  for(register GLuint i=0; i<6; ++i)
  {
    if(!isFaceVisible_[i]) { continue; }
    glFramebufferTexture2D(GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,
        depthTexture_->id(), 0);
    glClear(GL_DEPTH_BUFFER_BIT);

    sceneCamera_->viewUniform()->pushData((byte*)viewMatrices_[i].x);
    sceneCamera_->viewProjectionUniform()->pushData((byte*)viewProjectionMatrices_[i].x);

    traverse(rs);

    sceneCamera_->viewProjectionUniform()->popData();
    sceneCamera_->viewUniform()->popData();
  }
  sceneCamera_->projectionUniform()->popData();
}

void PointShadowMap::computeMoment(RenderState *rs)
{
  momentsCompute_->enable(rs);
  shadowNearUniform_->enableUniform(momentsNear_);
  shadowFarUniform_->enableUniform(momentsFar_);
  textureQuad_->draw(1);
  momentsCompute_->disable(rs);
}
