/*
 * light-camera.cpp
 *
 *  Created on: Dec 15, 2013
 *      Author: daniel
 */

#include <cfloat>

#include "light-camera.h"
using namespace regen;

static inline Vec2f findZRange(
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

LightCamera::LightCamera(
    const ref_ptr<Light> &light,
    const ref_ptr<Camera> &userCamera,
    Vec2f extends, GLuint numLayer, GLdouble splitWeight)
: OmniDirectionalCamera(GL_TRUE,GL_FALSE),
  light_(light),
  userCamera_(userCamera),
  splitWeight_(splitWeight)
{
  updateFrustum(90.0,1.0,extends.x,extends.y,GL_FALSE);
  lightMatrix_ = ref_ptr<ShaderInputMat4>::alloc("lightMatrix");

  switch(light_->lightType()) {
  case Light::DIRECTIONAL:
    numLayer_ = numLayer;
    update_ = &LightCamera::updateDirectional;
    proj_->set_elementCount(numLayer_);
    projInv_->set_elementCount(numLayer_);
    viewproj_->set_elementCount(numLayer_);
    viewprojInv_->set_elementCount(numLayer_);
    near_->set_elementCount(numLayer_);
    near_->set_forceArray(GL_TRUE);
    far_->set_elementCount(numLayer_);
    far_->set_forceArray(GL_TRUE);
    lightMatrix_->set_elementCount(numLayer_);
    lightMatrix_->set_forceArray(GL_TRUE);
    position_->setVertex(0, Vec3f(0.0f));

    shaderDefine("RENDER_TARGET", "2D_ARRAY");
    break;

  case Light::POINT: {
    numLayer_ = 6;
    update_ = &LightCamera::updatePoint;
    view_->set_elementCount(numLayer_);
    viewInv_->set_elementCount(numLayer_);
    viewproj_->set_elementCount(numLayer_);
    viewprojInv_->set_elementCount(numLayer_);

    // Initialize directions
    direction_->set_elementCount(numLayer_);
    direction_->setUniformDataUntyped(NULL);
    const Vec3f *dir = Mat4f::cubeDirections();
    for(register GLuint i=0; i<6; ++i) {
      direction_->setVertex(i, dir[i]);
    }

    // CubeMap's dont't need transformation matrix, they are accessed by positions
    lightMatrix_->set_elementCount(numLayer_);
    lightMatrix_->set_forceArray(GL_TRUE);
    shaderDefine("RENDER_TARGET", "CUBE");
    break;
  }

  case Light::SPOT:
    numLayer_ = 1;
    update_ = &LightCamera::updateSpot;
    shaderDefine("RENDER_TARGET", "2D");
    break;
  }
  shaderDefine("RENDER_LAYER", REGEN_STRING(numLayer_));

  view_->setUniformDataUntyped(NULL);
  viewInv_->setUniformDataUntyped(NULL);
  proj_->setUniformDataUntyped(NULL);
  projInv_->setUniformDataUntyped(NULL);
  viewproj_->setUniformDataUntyped(NULL);
  viewprojInv_->setUniformDataUntyped(NULL);

  near_->setUniformDataUntyped(NULL);
  near_->setVertex(0, extends.x);
  far_->setUniformDataUntyped(NULL);
  far_->setVertex(0, extends.y);

  lightMatrix_->setUniformDataUntyped(NULL);
  setInput(lightMatrix_);

  lightPosStamp_ = 0;
  lightDirStamp_ = 0;
  lightRadiusStamp_ = 0;
  projectionStamp_ = 0;

  if(light_->lightType() == Light::POINT)
  { for(GLuint i=0; i<6; ++i) isCubeFaceVisible_[i] = GL_TRUE; }

  // initially update shadow maps
  (this->*update_)();
}

void LightCamera::set_isCubeFaceVisible(GLenum face, GLboolean visible)
{ isCubeFaceVisible_[face - GL_TEXTURE_CUBE_MAP_POSITIVE_X] = visible; }

const ref_ptr<ShaderInputMat4>& LightCamera::lightMatrix() const
{ return lightMatrix_; }

void LightCamera::updateSpot()
{
  if(lightPosStamp_    == light_->position()->stamp() &&
     lightDirStamp_    == light_->direction()->stamp() &&
     lightRadiusStamp_ == light_->radius()->stamp())
  { return; }
  const Vec3f &pos = light_->position()->getVertex(0);
  const Vec3f &dir = light_->direction()->getVertex(0);
  const Vec2f &coneAngle = light_->coneAngle()->getVertex(0);
  const Vec2f &radius = light_->radius()->getVertex(0);

  position_->setVertex(0, pos);
  direction_->setVertex(0, dir);

  // Update view matrix.
  updateLookAt();
  // Update projection and recompute view-projection matrix.
  updateFrustum(
      1.0f,
      2.0*acos(coneAngle.y)*RAD_TO_DEGREE,
      near_->getVertex(0), radius.y,
      GL_TRUE);
  // Transforms world space coordinates to homogenous light space
  lightMatrix_->setVertex(0, viewproj_->getVertex(0) * Mat4f::bias());

  lightPosStamp_ = light_->position()->stamp();
  lightDirStamp_ = light_->direction()->stamp();
  lightRadiusStamp_ = light_->radius()->stamp();
}

void LightCamera::updatePoint()
{
  if(lightPosStamp_    == light_->position()->stamp() &&
     lightRadiusStamp_ == light_->radius()->stamp())
  { return; }
  const Vec3f &pos = light_->position()->getVertex(0);
  const Vec2f &radius = light_->radius()->getVertex(0);

  position_->setVertex(0, light_->position()->getVertex(0));

  // Update projection
  updateFrustum(
      1.0f, 90.0f,
      near_->getVertex(0), radius.y,
      GL_FALSE);
  updateProjection();

  // TODO: support to use parabolid mapping
  // Update view and view-projection matrix
  Mat4f::cubeLookAtMatrices(pos, (Mat4f*)view_->clientDataPtr());
  view_->nextStamp();

  for(register GLuint i=0; i<6; ++i) {
    if(!isCubeFaceVisible_[i]) { continue; }
    viewInv_->setVertex(i, view_->getVertex(i).lookAtInverse());
    updateViewProjection(0,i);
  }

  lightPosStamp_ = light_->position()->stamp();
  lightRadiusStamp_ = light_->radius()->stamp();
  lightMatrix_->nextStamp();
}

void LightCamera::updateDirectional()
{
  Mat4f *shadowMatrices = (Mat4f*)lightMatrix_->clientDataPtr();
  lightMatrix_->nextStamp();

  // Update near/far values when user camera projection changed
  if(projectionStamp_ != userCamera_->projection()->stamp())
  {
    const Mat4f &proj = userCamera_->projection()->getVertex(0);
    // update frustum splits
    for(std::vector<Frustum*>::iterator
        it=shadowFrusta_.begin(); it!=shadowFrusta_.end(); ++it)
    { delete *it; }
    shadowFrusta_ = userCamera_->frustum().split(numLayer_, splitWeight_);
    // update near/far values
    GLfloat *farValues = (GLfloat*)far_->clientDataPtr();
    GLfloat *nearValues = (GLfloat*)near_->clientDataPtr();
    far_->nextStamp();
    near_->nextStamp();
    for(GLuint i=0; i<numLayer_; ++i)
    {
      Frustum *frustum = shadowFrusta_[i];
      // frustum_->far() is originally in eye space - tell's us how far we can see.
      // Here we compute it in camera homogeneous coordinates. Basically, we calculate
      // proj * (0, 0, far, 1)^t and then normalize to [0; 1]
      farValues[i]  = 0.5*(-frustum->far  * proj(2,2) + proj(3,2)) / frustum->far + 0.5;
      nearValues[i] = 0.5*(-frustum->near * proj(2,2) + proj(3,2)) / frustum->near + 0.5;
    }
    projectionStamp_ = userCamera_->projection()->stamp();
  }

  // Update view matrix when light direction changed
  if(lightDirStamp_ != light_->direction()->stamp())
  {
    const Vec3f &dir = light_->direction()->getVertex(0);
    Vec3f f(-dir.x, -dir.y, -dir.z);
    f.normalize();
    Vec3f s( 0.0f, -f.z, f.y );
    s.normalize();
    // Equivalent to getLookAtMatrix(pos=(0,0,0), dir=f, up=(-1,0,0))
    view_->setVertex(0, Mat4f(
        0.0f, s.y*f.z - s.z*f.y, -f.x, 0.0f,
         s.y,           s.z*f.x, -f.y, 0.0f,
         s.z,          -s.y*f.x, -f.z, 0.0f,
        0.0f,              0.0f, 0.0f, 1.0f
    ));
    viewInv_->setVertex(0, view_->getVertex(0).lookAtInverse());
    direction_->setVertex(0, f);

    lightDirStamp_ = light_->direction()->stamp();
  }

  // Update projection and view-projection matrix
  for(register GLuint i=0; i<numLayer_; ++i)
  {
    Frustum *frustum = shadowFrusta_[i];
    // update frustum points in world space
    frustum->update(
        userCamera_->position()->getVertex(0),
        userCamera_->direction()->getVertex(0));

    // get the projection matrix with the new z-bounds
    // note the inversion because the light looks at the neg. z axis
    Vec2f zRange = findZRange(view_->getVertex(0), frustum->points);

    proj_->setVertex(i, Mat4f::orthogonalMatrix(
        -1.0, 1.0, -1.0, 1.0, -zRange.y, -zRange.x));
    // find the extends of the frustum slice as projected in light's homogeneous coordinates
    Vec2f xRange(FLT_MAX,FLT_MIN);
    Vec2f yRange(FLT_MAX,FLT_MIN);
    Mat4f mvpMatrix = view_->getVertex(0) * proj_->getVertex(i);
    for(register GLuint j=0; j<8; ++j)
    {
        Vec4f transf = mvpMatrix ^ frustum->points[j];
        transf.x /= transf.w;
        transf.y /= transf.w;
        if (transf.x > xRange.y) { xRange.y = transf.x; }
        if (transf.x < xRange.x) { xRange.x = transf.x; }
        if (transf.y > yRange.y) { yRange.y = transf.y; }
        if (transf.y < yRange.x) { yRange.x = transf.y; }
    }
    proj_->setVertex(i, proj_->getVertex(i)*Mat4f::cropMatrix(
        xRange.x, xRange.y, yRange.x, yRange.y));
    // TODO slow inverse. multiply special ortho/crop inverse
    projInv_->setVertex(i, proj_->getVertex(i).inverse());

    updateViewProjection(i,0);
    // transforms world space coordinates to homogeneous light space
    shadowMatrices[i] = viewproj_->getVertex(i) * Mat4f::bias();
  }
}

void LightCamera::enable(RenderState *rs)
{
  (this->*update_)();
  Camera::enable(rs);
}

GLboolean LightCamera::hasIntersectionWithSphere(const Vec3f &center, GLfloat radius)
{
  switch(light_->lightType()) {
  case Light::DIRECTIONAL:
    for(register GLuint i=0; i<numLayer_; ++i) {
      Frustum *frustum = shadowFrusta_[i];
      if(frustum->hasIntersectionWithSphere(center,radius))
        return GL_TRUE;
    }
    return GL_FALSE;
  case Light::POINT:
    return OmniDirectionalCamera::hasIntersectionWithSphere(center,radius);
  case Light::SPOT:
    return Camera::hasIntersectionWithSphere(center,radius);
  }
  return GL_FALSE;
}

GLboolean LightCamera::hasIntersectionWithBox(const Vec3f &center, const Vec3f *points)
{
  switch(light_->lightType()) {
  case Light::DIRECTIONAL:
    for(register GLuint i=0; i<numLayer_; ++i) {
      Frustum *frustum = shadowFrusta_[i];
      if(frustum->hasIntersectionWithBox(center,points))
        return GL_TRUE;
    }
    return GL_FALSE;
  case Light::POINT:
    return OmniDirectionalCamera::hasIntersectionWithBox(center,points);
  case Light::SPOT:
    return Camera::hasIntersectionWithBox(center,points);
  }
  return GL_FALSE;
}

