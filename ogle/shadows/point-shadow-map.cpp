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
    ref_ptr<PointLight> &light,
    ref_ptr<PerspectiveCamera> &sceneCamera,
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
  // on nvidia linear filtering gives 2x2 PCF for 'free'
  texture_->set_filter(GL_LINEAR,GL_LINEAR);
  texture_->set_internalFormat(internalFormat);
  texture_->set_pixelType(pixelType);
  texture_->set_size(shadowMapSize, shadowMapSize);
  texture_->set_compare(compareMode_, GL_LEQUAL);
  texture_->set_samplerType("samplerCubeShadow");
  texture_->texImage();
  shadowMapSize_->setUniformData((float)shadowMapSize);

  shadowFarUniform_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f(
      FORMAT_STRING("shadowFar"<<light->id())));
  shadowFarUniform_->setUniformData(200.0f);
  light->joinShaderInput(ref_ptr<ShaderInput>::cast(shadowFarUniform_));

  shadowNearUniform_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f(
      FORMAT_STRING("shadowNear"<<light->id())));
  shadowNearUniform_->setUniformData(0.1f);
  light->joinShaderInput(ref_ptr<ShaderInput>::cast(shadowNearUniform_));

  for(GLuint i=0; i<6; ++i) { isFaceVisible_[i] = GL_TRUE; }

#ifdef USE_LAYERED_SHADER
  rs_ = new LayeredShadowRenderState(ref_ptr<Texture>::cast(texture_), 39, 6);
#else
  rs_ = new ShadowRenderState(ref_ptr<Texture>::cast(texture_));
#endif

  updateLight();
}
PointShadowMap::~PointShadowMap()
{
  delete rs_;
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
void PointShadowMap::set_near(GLfloat near)
{
  shadowNearUniform_->setVertex1f(0, near);
}
GLfloat PointShadowMap::near() const
{
  return shadowNearUniform_->getVertex1f(0);
}

void PointShadowMap::updateLight()
{
  static const Vec3f dir[6] = {
      Vec3f( 1.0f, 0.0f, 0.0f),
      Vec3f(-1.0f, 0.0f, 0.0f),
      Vec3f( 0.0f, 1.0f, 0.0f),
      Vec3f( 0.0f,-1.0f, 0.0f),
      Vec3f( 0.0f, 0.0f, 1.0f),
      Vec3f( 0.0f, 0.0f,-1.0f)
  };
  // have to change up vector for top and bottom face
  // for getLookAtMatrix
  static const Vec3f up[6] = {
      Vec3f( 0.0f, -1.0f, 0.0f),
      Vec3f( 0.0f, -1.0f, 0.0f),
      Vec3f( 0.0f, 0.0f, -1.0f),
      Vec3f( 0.0f, 0.0f, -1.0f),
      Vec3f( 0.0f, -1.0f, 0.0f),
      Vec3f( 0.0f, -1.0f, 0.0f)
  };

  const Vec3f &pos = pointLight_->position()->getVertex3f(0);
  const Vec3f &a = light_->attenuation()->getVertex3f(0);

  // adjust far value for better precision
  GLfloat far;
  // find far value where light attenuation reaches threshold,
  // insert farAttenuation in the attenuation equation and solve
  // equation for distance
  GLdouble p2 = a.y/(2.0*a.z);
  far = -p2 + sqrt(p2*p2 - (a.x/farAttenuation_ - 1.0/(farAttenuation_*a.z)));
  // hard limit z range
  if(farLimit_>0.0f && far>farLimit_) far=farLimit_;
  shadowFarUniform_->setVertex1f(0, far);

  projectionMatrix_ = projectionMatrix(90.0, 1.0f, near(), far);

  for(register GLuint i=0; i<6; ++i) {
    if(!isFaceVisible_[i]) { continue; }
    viewMatrices_[i] = getLookAtMatrix(pos, dir[i], up[i]);
    viewProjectionMatrices_[i] = viewMatrices_[i] * projectionMatrix_;
  }
}

void PointShadowMap::updateGraphics(GLdouble dt)
{
  enable(rs_);
  rs_->enable();

#ifdef USE_LAYERED_SHADER
  rs_->set_shadowViewProjectionMatrices(viewProjectionMatrices_);
  traverse(rs_);
#else
  Mat4f sceneView = sceneCamera_->viewUniform()->getVertex16f(0);
  Mat4f sceneProjection = sceneCamera_->projectionUniform()->getVertex16f(0);
  sceneCamera_->projectionUniform()->setVertex16f(0, projectionMatrix_);

  for(register GLuint i=0; i<6; ++i) {
    if(!isFaceVisible_[i]) { continue; }
    glFramebufferTexture2D(GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,
        texture_->id(), 0);
    sceneCamera_->viewUniform()->setVertex16f(0, viewMatrices_[i]);
    traverse(rs_);
  }
  glFramebufferTexture(GL_FRAMEBUFFER,
      GL_DEPTH_ATTACHMENT, texture_->id(), 0);

  sceneCamera_->viewUniform()->setVertex16f(0, sceneView);
  sceneCamera_->projectionUniform()->setVertex16f(0, sceneProjection);
#endif

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
