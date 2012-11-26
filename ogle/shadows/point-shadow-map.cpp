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
 #define USE_LAYERED_SHADER

// TODO: use one cubemap for multiple lights ?
// TODO: dual parabolid shadow mapping
// TODO: allow ignoring specified cube faces

PointShadowMap::PointShadowMap(
    ref_ptr<PointLight> &light,
    ref_ptr<PerspectiveCamera> &sceneCamera,
    GLuint shadowMapSize,
    GLuint maxNumBones,
    GLenum internalFormat,
    GLenum pixelType)
: ShadowMap(ref_ptr<Light>::cast(light), ref_ptr<Texture>::manage(new CubeMapDepthTexture)),
  pointLight_(light),
  sceneCamera_(sceneCamera),
  compareMode_(GL_COMPARE_R_TO_TEXTURE),
  farLimit_(200.0f),
  farAttenuation_(0.01f),
  near_(0.1f)
{
  texture_->set_internalFormat(internalFormat);
  texture_->set_pixelType(pixelType);
  texture_->set_size(shadowMapSize, shadowMapSize);
  texture_->set_compare(compareMode_, GL_LEQUAL);
  texture_->texImage();

  viewMatrices_ = new Mat4f[6];
  viewProjectionMatrices_ = new Mat4f[6];

  // uniforms for shadow sampling
  shadowMatUniform_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4(
      FORMAT_STRING("shadowMatrices"<<light->id()), 6));
  shadowMatUniform_->setInstanceData(1, 1, NULL);

  light_->joinShaderInput(ref_ptr<ShaderInput>::cast(shadowMatUniform()));

#ifdef USE_LAYERED_SHADER
  rs_ = new LayeredShadowRenderState(ref_ptr<Texture>::cast(texture_), maxNumBones, 6);
#else
  rs_ = new ShadowRenderState(ref_ptr<Texture>::cast(texture_));
#endif

  updateLight();
}
PointShadowMap::~PointShadowMap()
{
  delete[] viewMatrices_;
  delete[] viewProjectionMatrices_;
  delete rs_;
}

ref_ptr<ShaderInputMat4>& PointShadowMap::shadowMatUniform()
{
  return shadowMatUniform_;
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
  near_ = near;
}
GLfloat PointShadowMap::near() const
{
  return near_;
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

  Mat4f *shadowMatrices = (Mat4f*)shadowMatUniform_->dataPtr();
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

  projectionMatrix_ = projectionMatrix(90.0, 1.0f, near_, far);

  for(register GLuint i=0; i<6; ++i) {
    viewMatrices_[i] = getLookAtMatrix(pos, dir[i], up[i]);
    viewProjectionMatrices_[i] = viewMatrices_[i] * projectionMatrix_;
    // transforms world space coordinates to homogeneous light space
    shadowMatrices[i] = viewProjectionMatrices_[i] * biasMatrix_;
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
