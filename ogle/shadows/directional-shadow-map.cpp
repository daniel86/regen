/*
 * directional-shadow-map.cpp
 *
 *  Created on: 20.11.2012
 *      Author: daniel
 */

#include <ogle/utility/string-util.h>

#include "directional-shadow-map.h"

static Vec2f findZRange(
    const Mat4f &mat, const Vec3f *frustumPoints)
{
  Vec2f range;
  // find the z-range of the current frustum as seen from the light
  // in order to increase precision
#define TRANSFORM_Z(vec) mat.x[2]*vec.x + mat.x[6]*vec.y + mat.x[10]*vec.z + mat.x[14]
  // note that only the z-component is needed and thus
  // the multiplication can be simplified from mat*vec4f(frustumPoints[0], 1.0f) to..
  GLfloat buf = TRANSFORM_Z(frustumPoints[0]);
  range.x = buf;
  range.y = buf;
  for(GLint i=1; i<8; ++i)
  {
    buf = TRANSFORM_Z(frustumPoints[i]);
    if(buf > range.y) { range.y = buf; }
    if(buf < range.x) { range.x = buf; }
  }
#undef TRANSFORM_Z
  return range;
}

DirectionalShadowMap::DirectionalShadowMap(
    ref_ptr<DirectionalLight> &light,
    ref_ptr<Frustum> &sceneFrustum,
    ref_ptr<PerspectiveCamera> &sceneCamera,
    GLuint numSplits,
    GLuint shadowMapSize)
: Animation(),
  light_(light),
  sceneCamera_(sceneCamera),
  sceneFrustum_(sceneFrustum),
  numSplits_(numSplits)
{
  // create a 3d depth texture - each frustum slice gets one layer
  texture_ = ref_ptr<DepthTexture3D>::manage(new DepthTexture3D(
      1, GL_DEPTH_COMPONENT24, GL_FLOAT, GL_TEXTURE_2D_ARRAY));
  texture_->set_numTextures(numSplits_);
  texture_->bind();
  texture_->set_size(shadowMapSize, shadowMapSize);
  texture_->set_filter(GL_LINEAR,GL_LINEAR);
  texture_->set_wrapping(GL_CLAMP_TO_EDGE);
  texture_->set_compare(GL_COMPARE_R_TO_TEXTURE, GL_LEQUAL);
  texture_->texImage();
  // create depth only render target for updating the shadow maps
  glGenFramebuffers(1, &fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glDrawBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // uniforms for updating shadow maps
  mvpMatrixUniform_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4(
      "modelViewMatrix", numSplits_));
  // uniforms for shadow sampling
  shadowMatUniform_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4(
      FORMAT_STRING("shadowMatrix"<<light->id()), numSplits_));
  shadowFarUniform_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f(
      FORMAT_STRING("shadowFar"<<light->id()), numSplits_));

  updateLightDirection();
  updateProjection();
}
DirectionalShadowMap::~DirectionalShadowMap()
{
  for(vector<Frustum*>::iterator
      it=shadowFrusta_.begin(); it!=shadowFrusta_.end(); ++it) { delete *it; }
  shadowFrusta_.clear();
}

void DirectionalShadowMap::updateLightDirection()
{
  const Vec3f &dir = light_->direction()->getVertex3f(0);
  Vec3f f(-dir.x, -dir.y, -dir.z);
  normalize(f);
  Vec3f s( 0.0f, -f.z, f.y );
  normalize(s);
  // Equivalent to getLookAtMatrix((0,0,0), -dir, (-1,0,0))
  modelViewMatrix_ = Mat4f(
      0.0f, s.y*f.z - s.z*f.y, -f.x, 0.0f,
       s.y,           s.z*f.x, -f.y, 0.0f,
       s.z,          -s.y*f.x, -f.z, 0.0f,
      0.0f,              0.0f, 0.0f, 1.0f
  );
}

void DirectionalShadowMap::updateProjection()
{
  for(vector<Frustum*>::iterator
      it=shadowFrusta_.begin(); it!=shadowFrusta_.end(); ++it) { delete *it; }
  shadowFrusta_ = sceneFrustum_->split(numSplits_, 0.75);
}

void DirectionalShadowMap::animate(GLdouble dt)
{
  static Mat4f staticBiasMatrix = Mat4f(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.0,
    0.5, 0.5, 0.5, 1.0 );

  Mat4f *mvpMatrices = (Mat4f*)mvpMatrixUniform_->dataPtr();
  Mat4f *shadowMatrices = (Mat4f*)shadowMatUniform_->dataPtr();
  Mat4f &proj = sceneCamera_->projectionUniform()->getVertex16f(0);
  GLfloat *farValues = (GLfloat*)shadowFarUniform_->dataPtr();

  for(GLuint i=0; i<numSplits_; ++i)
  {
    Frustum *frustum = shadowFrusta_[i];
    // update frustum points in world space
    frustum->calculatePoints(
        sceneCamera_->position(),
        sceneCamera_->direction(),
        UP_VECTOR);
    const Vec3f *frustumPoints = frustum->points();

    // find ranges to optimize precision
    Vec2f xRange(-1000.0,1000.0);
    Vec2f yRange(-1000.0,1000.0);
    Vec2f zRange = findZRange(modelViewMatrix_, frustumPoints);

    // get the projection matrix with the new z-bounds
    // note the inversion because the light looks at the neg. z axis
    Mat4f projectionMatrix = getOrthogonalProjectionMatrix(
        -1.0, 1.0, -1.0, 1.0, -zRange.y, -zRange.x);
    // shadow model view projection matrix
    mvpMatrices[i] = transpose(modelViewMatrix_ * projectionMatrix);

    // find the extends of the frustum slice as projected in light's homogeneous coordinates
    for(GLuint j=0; j<8; ++j)
    {
        Vec4f transf = mvpMatrices[i] * frustumPoints[j];
        transf.x /= transf.w;
        transf.y /= transf.w;
        if (transf.x > xRange.y) { xRange.y = transf.x; }
        if (transf.x < xRange.x) { xRange.x = transf.x; }
        if (transf.y > yRange.y) { yRange.y = transf.y; }
        if (transf.y < yRange.x) { yRange.x = transf.y; }
    }
    // TODO: use this ?
    //Mat4f projectionCropMatrix =
    //    projectionMatrix * getCropMatrix(xRange.x, xRange.y, yRange.x, yRange.y);
    //Mat4f mvpCropMatrix = modelViewMatrix_ * projectionCropMatrix;

    // transforms world space coordinates to homogenous light space
    shadowMatrices[i] = mvpMatrices[i] * staticBiasMatrix;

    // frustum_->far() is originally in eye space - tell's us how far we can see.
    // Here we compute it in camera homogeneous coordinates. Basically, we calculate
    // proj * (0, 0, far, 1)^t and then normalize to [0; 1]
    farValues[i] = 0.5*(-frustum->far() * proj(2,2) + proj(3,2)) / frustum->far() + 0.5;
  }
}

void DirectionalShadowMap::updateGraphics(GLdouble dt)
{
  // offset the geometry slightly to prevent z-fighting
  // note that this introduces some light-leakage artifacts
  glEnable( GL_POLYGON_OFFSET_FILL );
  glPolygonOffset( 1.1, 4096.0 );
  // moves acne to back faces
  glCullFace(GL_FRONT);

  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glDrawBuffer(GL_NONE);
  glViewport(0, 0, texture_->width(), texture_->height());

  // TODO: enable modelViewMatrix_

  for(GLuint i=0; i<numSplits_; ++i) {
    // make the current depth map a rendering target
    glFramebufferTextureLayer(GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT, texture_->id(), 0, i);
    // clear the depth texture from last time
    glClear(GL_DEPTH_BUFFER_BIT);

    // TODO: enable mvpMatrix_[i]
    // TODO: tree traverse...
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glCullFace(GL_BACK);
  glDisable(GL_POLYGON_OFFSET_FILL);
}
