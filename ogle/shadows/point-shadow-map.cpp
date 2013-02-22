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
: ShadowMap(ref_ptr<Light>::cast(light), shadowMapSize),
  pointLight_(light),
  sceneCamera_(sceneCamera),
  farAttenuation_(0.01f),
  farLimit_(200.0f)
{
  // stores depth values from light perspective
  ref_ptr<Texture> depthTexture = ref_ptr<Texture>::manage(new CubeMapDepthTexture);
  depthTexture->set_internalFormat(depthFormat);
  depthTexture->set_pixelType(depthType);
  depthTexture->set_targetType(GL_TEXTURE_CUBE_MAP);
  set_depthTexture(depthTexture, "samplerCubeShadow");
  depthTexture_->set_compare(GL_COMPARE_R_TO_TEXTURE, GL_LEQUAL);

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

void PointShadowMap::computeDepth()
{
  Mat4f &view = sceneCamera_->viewUniform()->getVertex16f(0);
  Mat4f &proj = sceneCamera_->projectionUniform()->getVertex16f(0);
  Mat4f &viewproj = sceneCamera_->viewProjectionUniform()->getVertex16f(0);
  Mat4f sceneView = view;
  Mat4f sceneProj = proj;
  Mat4f sceneViewProj = viewproj;
  proj = projectionMatrix_;

  for(register GLuint i=0; i<6; ++i)
  {
    if(!isFaceVisible_[i]) { continue; }
    glFramebufferTexture2D(GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,
        depthTexture_->id(), 0);
    handleGLError("computeDepth ~glFramebufferTexture2D");
    view = viewMatrices_[i];
    viewproj = viewProjectionMatrices_[i];
    traverse(&depthRenderState_);
  }

  view = sceneView;
  proj = sceneProj;
  viewproj = sceneViewProj;
}

void PointShadowMap::computeMoment()
{
  momentsCompute_->enable(&filteringRenderState_);
  shadowNearUniform_->enableUniform(momentsNear_);
  shadowFarUniform_->enableUniform(momentsFar_);
  textureQuad_->draw(1);
  momentsCompute_->disable(&filteringRenderState_);

  if(momentsBlur_.get()) {
    momentsBlur_->enable(&filteringRenderState_);
    momentsBlur_->disable(&filteringRenderState_);
  }
}
