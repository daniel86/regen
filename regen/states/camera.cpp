/*
 * camera.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include <GL/glew.h>

#include <cfloat>

#include <regen/states/light-state.h>
#include <regen/states/atomic-states.h>
#include <regen/math/vector.h>
#include <regen/math/matrix.h>
#include <regen/av/audio.h>

#include "camera.h"
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

Camera::Camera(GLboolean initializeMatrices)
: HasInputState(VBO::USAGE_DYNAMIC),
  isAudioListener_(GL_FALSE)
{
  position_ = ref_ptr<ShaderInput3f>::alloc("cameraPosition");
  position_->setUniformData(Vec3f( 0.0, 1.0, 4.0 ));
  setInput(position_);

  direction_ = ref_ptr<ShaderInput3f>::alloc("cameraDirection");
  direction_->setUniformData(Vec3f( 0, 0, -1 ));
  setInput(direction_);

  vel_ = ref_ptr<ShaderInput3f>::alloc("cameraVelocity");
  vel_->setUniformData(Vec3f(0.0f));
  setInput(vel_);

  const GLfloat fov    = 45.0;
  const GLfloat aspect = 8.0/6.0;
  const GLfloat near   = 1.0;
  const GLfloat far    = 200.0;
  frustum_ = ref_ptr<Frustum>::alloc();
  frustum_->setProjection(fov, aspect, near, far);
  setInput(frustum_->fov());
  setInput(frustum_->near());
  setInput(frustum_->far());
  setInput(frustum_->aspect());

  view_ = ref_ptr<ShaderInputMat4>::alloc("viewMatrix");
  viewInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseViewMatrix");
  proj_ = ref_ptr<ShaderInputMat4>::alloc("projectionMatrix");
  projInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseProjectionMatrix");
  viewproj_ = ref_ptr<ShaderInputMat4>::alloc("viewProjectionMatrix");
  viewprojInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseViewProjectionMatrix");
  setInput(view_);
  setInput(viewInv_);
  setInput(proj_);
  setInput(projInv_);
  setInput(viewproj_);
  setInput(viewprojInv_);

  if(initializeMatrices) {
    view_->setUniformData(Mat4f::lookAtMatrix(
        position_->getVertex(0),
        direction_->getVertex(0),
        Vec3f::up()));
    proj_->setUniformData(Mat4f::projectionMatrix(
        frustum_->fov()->getVertex(0),
        frustum_->aspect()->getVertex(0),
        frustum_->near()->getVertex(0),
        frustum_->far()->getVertex(0))
    );
    viewInv_->setUniformData(view_->getVertex(0).lookAtInverse());
    projInv_->setUniformData(proj_->getVertex(0).projectionInverse());
    viewproj_->setUniformData(view_->getVertex(0) * proj_->getVertex(0));
    viewprojInv_->setUniformData(projInv_->getVertex(0) * viewInv_->getVertex(0));
  }
}

void Camera::updateFrustum(
    const Vec2i viewport,
    GLfloat fov,
    GLfloat near,
    GLfloat far)
{
  frustum_->setProjection(
      fov,
      ((GLfloat)viewport.x)/((GLfloat)viewport.y),
      near,
      far);
  proj_->setVertex(0, Mat4f::projectionMatrix(
      fov, frustum_->aspect()->getVertex(0), near, far));
  projInv_->setVertex(0,
      proj_->getVertex(0).projectionInverse());
  viewproj_->setVertex(0,
      view_->getVertex(0) * proj_->getVertex(0));
  viewprojInv_->setVertex(0,
      projInv_->getVertex(0) * viewInv_->getVertex(0));
}

const ref_ptr<Frustum>& Camera::frustum() const
{ return frustum_; }

const ref_ptr<ShaderInput3f>& Camera::position() const
{ return position_; }
const ref_ptr<ShaderInput3f>& Camera::velocity() const
{ return vel_; }
const ref_ptr<ShaderInput3f>& Camera::direction() const
{ return direction_; }

const ref_ptr<ShaderInputMat4>& Camera::view() const
{ return view_; }
const ref_ptr<ShaderInputMat4>& Camera::viewInverse() const
{ return viewInv_; }

const ref_ptr<ShaderInputMat4>& Camera::projection() const
{ return proj_; }
const ref_ptr<ShaderInputMat4>& Camera::projectionInverse() const
{ return projInv_; }

const ref_ptr<ShaderInputMat4>& Camera::viewProjection() const
{ return viewproj_; }
const ref_ptr<ShaderInputMat4>& Camera::viewProjectionInverse() const
{ return viewprojInv_; }

void Camera::set_isAudioListener(GLboolean isAudioListener)
{
  isAudioListener_ = isAudioListener;
  if(isAudioListener_) {
    AudioListener::set3f(AL_POSITION, position_->getVertex(0));
    AudioListener::set3f(AL_VELOCITY, vel_->getVertex(0));
    AudioListener::set6f(AL_ORIENTATION, Vec6f(direction_->getVertex(0), Vec3f::up()));
  }
}
GLboolean Camera::isAudioListener() const
{ return isAudioListener_; }

/////////////////////////////
/////////////////////////////

ReflectionCamera::ReflectionCamera(
    const ref_ptr<Camera> &userCamera,
    const ref_ptr<Mesh> &mesh,
    GLuint vertexIndex)
: Camera(),
  userCamera_(userCamera),
  vertexIndex_(vertexIndex),
  projStamp_(userCamera->projection()->stamp()-1),
  camPosStamp_(userCamera->position()->stamp()-1),
  camDirStamp_(userCamera->direction()->stamp()-1),
  cameraChanged_(GL_TRUE),
  isFront_(GL_TRUE),
  hasMesh_(GL_TRUE)
{
  frustum_->setProjection(
      userCamera->frustum()->fov()->getVertex(0),
      userCamera->frustum()->aspect()->getVertex(0),
      userCamera->frustum()->near()->getVertex(0),
      userCamera->frustum()->far()->getVertex(0));

  clipPlane_ = ref_ptr<ShaderInput4f>::alloc("clipPlane");
  clipPlane_->setUniformData(Vec4f(0.0f));
  setInput(clipPlane_);

  pos_ = mesh->positions();
  nor_ = mesh->normals();
  isReflectorValid_ = (pos_.get()!=NULL) && (nor_.get()!=NULL);
  if(isReflectorValid_) {
    posStamp_ = pos_->stamp()-1;
    norStamp_ = nor_->stamp()-1;
  }

  transform_ = mesh->findShaderInput("modelMatrix");
  if(transform_.get()!=NULL) {
    transformStamp_ = transform_->stamp()-1;
  }
}

ReflectionCamera::ReflectionCamera(
    const ref_ptr<Camera> &userCamera,
    const Vec3f &reflectorNormal,
    const Vec3f &reflectorPoint)
: Camera(),
  userCamera_(userCamera),
  projStamp_(userCamera->projection()->stamp()-1),
  camPosStamp_(userCamera->position()->stamp()-1),
  camDirStamp_(userCamera->direction()->stamp()-1),
  cameraChanged_(GL_TRUE),
  isFront_(GL_TRUE),
  hasMesh_(GL_FALSE)
{
  frustum_->setProjection(
      userCamera->frustum()->fov()->getVertex(0),
      userCamera->frustum()->aspect()->getVertex(0),
      userCamera->frustum()->near()->getVertex(0),
      userCamera->frustum()->far()->getVertex(0));

  clipPlane_ = ref_ptr<ShaderInput4f>::alloc("clipPlane");
  clipPlane_->setUniformData(Vec4f(0.0f));
  setInput(clipPlane_);

  vertexIndex_ = 0;
  transformStamp_ = 0;
  posStamp_ = 0;
  norStamp_ = 0;
  posWorld_ = reflectorPoint;
  norWorld_ = reflectorNormal;
  isReflectorValid_ = GL_TRUE;

  clipPlane_->setVertex(0, Vec4f(
      norWorld_.x,norWorld_.y,norWorld_.z,
      norWorld_.dot(posWorld_)));
  reflectionMatrix_ = Mat4f::reflectionMatrix(posWorld_,norWorld_);
}

void ReflectionCamera::enable(RenderState *rs)
{
  if(!isReflectorValid_) {
    REGEN_WARN("Reflector has no position/normal attribute.");
  }
  else if(!isHidden()) {
    updateReflection();
    Camera::enable(rs);
  }
}

void ReflectionCamera::updateReflection()
{
  GLboolean reflectorChanged = GL_FALSE;
  if(hasMesh_) {
    if(transform_.get()!=NULL && transform_->stamp()!=transformStamp_) {
      reflectorChanged = GL_TRUE;
      transformStamp_ = transform_->stamp();
    }
    if(nor_->stamp()!=norStamp_) {
      reflectorChanged = GL_TRUE;
      norStamp_ = nor_->stamp();
    }
    if(pos_->stamp()!=posStamp_) {
      reflectorChanged = GL_TRUE;
      posStamp_ = pos_->stamp();
    }
    // Compute plane parameters...
    if(reflectorChanged) {
      if(!pos_->hasClientData()) pos_->readServerData();
      if(!nor_->hasClientData()) nor_->readServerData();
      posWorld_ = ((Vec3f*)pos_->clientData())[vertexIndex_];
      norWorld_ = ((Vec3f*)nor_->clientData())[vertexIndex_];

      if(transform_.get()!=NULL) {
        if(!transform_->hasClientData()) transform_->readServerData();
        Mat4f transform = (((Mat4f*)transform_->clientData())[0]).transpose();
        posWorld_ = transform.transformVector(posWorld_);
        norWorld_ = transform.rotateVector(norWorld_);
        norWorld_.normalize();
      }
    }
  }

  // Switch normal if viewer is behind reflector.
  // TODO: also allow to skip computation in this case.
  const Vec3f &camPos = userCamera_->position()->getVertex(0);
  GLboolean isFront = norWorld_.dot(camPos-posWorld_)>0.0;
  if(isFront != isFront_) {
    isFront_ = isFront;
    reflectorChanged = GL_TRUE;
  }
  // Compute reflection matrix...
  if(reflectorChanged) {
    if(isFront_) {
      clipPlane_->setVertex(0, Vec4f(
          norWorld_.x,norWorld_.y,norWorld_.z,
          norWorld_.dot(posWorld_)));
      reflectionMatrix_ = Mat4f::reflectionMatrix(posWorld_,norWorld_);
    }
    else {
      // flip reflector normal
      Vec3f n = -norWorld_;
      clipPlane_->setVertex(0, Vec4f(n.x,n.y,n.z, n.dot(posWorld_)));
      reflectionMatrix_ = Mat4f::reflectionMatrix(posWorld_,n);
    }
  }

  // Compute reflection camera direction
  if(reflectorChanged || userCamera_->direction()->stamp() != camDirStamp_) {
    camDirStamp_ = userCamera_->direction()->stamp();
    Vec3f dir = reflectionMatrix_.rotateVector(userCamera_->direction()->getVertex(0));
    dir.normalize();
    direction_->setVertex(0,dir);

    reflectorChanged = GL_TRUE;
  }
  // Compute reflection camera position
  if(reflectorChanged || userCamera_->position()->stamp() != camPosStamp_) {
    camPosStamp_ = userCamera_->position()->stamp();
    Vec3f reflected = reflectionMatrix_.transformVector(
        userCamera_->position()->getVertex(0));
    position_->setVertex(0, reflected);

    reflectorChanged = GL_TRUE;
  }

  // Compute view matrix
  if(reflectorChanged) {
    view_->setUniformData(Mat4f::lookAtMatrix(
        position_->getVertex(0),
        direction_->getVertex(0),
        Vec3f::up()));
    viewInv_->setUniformData(view_->getVertex(0).lookAtInverse());
    cameraChanged_ = GL_TRUE;
  }

  // Compute projection matrix
  if(userCamera_->projection()->stamp() != projStamp_) {
    projStamp_ = userCamera_->projection()->stamp();
    proj_->setUniformData(
        userCamera_->projection()->getVertex(0));
    projInv_->setUniformData(
        userCamera_->projectionInverse()->getVertex(0));
    cameraChanged_ = GL_TRUE;
  }

  // Compute view-projection matrix
  if(cameraChanged_) {
    viewproj_->setVertex(0,
        view_->getVertex(0) * proj_->getVertex(0));
    viewprojInv_->setVertex(0,
        projInv_->getVertex(0) * viewInv_->getVertex(0));
    cameraChanged_ = GL_FALSE;
  }
}

//////////
//////////
//////////

CubeCamera::CubeCamera(
    const ref_ptr<Mesh> &mesh,
    const ref_ptr<Camera> &userCamera)
: Camera(GL_FALSE),
  userCamera_(userCamera)
{
  shaderDefine("RENDER_TARGET", "CUBE");
  shaderDefine("RENDER_LAYER", "6");

  view_->set_elementCount(6);
  view_->setUniformDataUntyped(NULL);
  viewInv_->set_elementCount(6);
  viewInv_->setUniformDataUntyped(NULL);
  proj_->setUniformDataUntyped(NULL);
  projInv_->setUniformDataUntyped(NULL);
  viewproj_->set_elementCount(6);
  viewproj_->setUniformDataUntyped(NULL);
  viewprojInv_->set_elementCount(6);
  viewprojInv_->setUniformDataUntyped(NULL);

  frustum_->setProjection(
      userCamera_->frustum()->fov()->getVertex(0),
      userCamera_->frustum()->aspect()->getVertex(0),
      userCamera_->frustum()->near()->getVertex(0),
      userCamera_->frustum()->far()->getVertex(0));

  modelMatrix_ = ref_ptr<ShaderInputMat4>::upCast(
      mesh->findShaderInput("modelMatrix"));
  pos_ = ref_ptr<ShaderInput3f>::upCast(mesh->positions());

  positionStamp_ = 0;
  matrixStamp_ = 0;
  for(GLuint i=0; i<6; ++i) isCubeFaceVisible_[i] = GL_TRUE;

  // initially update shadow maps
  update();
}

void CubeCamera::set_isCubeFaceVisible(GLenum face, GLboolean visible)
{ isCubeFaceVisible_[face - GL_TEXTURE_CUBE_MAP_POSITIVE_X] = visible; }

void CubeCamera::update()
{
  GLuint positionStamp = (pos_.get() == NULL ? 1 : pos_->stamp());
  GLuint matrixStamp = (modelMatrix_.get() == NULL ? 1 : modelMatrix_->stamp());
  if(positionStamp_ == positionStamp && matrixStamp_ == matrixStamp)
  { return; }

  Vec3f pos = Vec3f::zero();
  if(modelMatrix_.get() != NULL) {
    pos = modelMatrix_->getVertex(0).transpose().transformVector(pos);
  }
  position_->setVertex(0,pos);
  // XXX
  direction_->setVertex(0,Vec3f(0.0,0.0,1.0));

  GLfloat near = userCamera_->frustum()->near()->getVertex(0);
  GLfloat far = userCamera_->frustum()->far()->getVertex(0);

  proj_->setVertex(0, Mat4f::projectionMatrix(90.0f, 1.0f, near, far));
  projInv_->setVertex(0, proj_->getVertex(0).projectionInverse());
  Mat4f::cubeLookAtMatrices(pos, (Mat4f*)view_->clientDataPtr());

  for(register GLuint i=0; i<6; ++i) {
    if(!isCubeFaceVisible_[i]) { continue; }
    viewInv_->setVertex(i, view_->getVertex(i).lookAtInverse());
    viewproj_->setVertex(i, view_->getVertex(i)*proj_->getVertex(0));
    viewprojInv_->setVertex(i, projInv_->getVertex(0)*viewInv_->getVertex(i));
  }

  positionStamp_ = positionStamp;
  matrixStamp_ = matrixStamp;
}

void CubeCamera::enable(RenderState *rs)
{
  update();
  Camera::enable(rs);
}

//////////
//////////
//////////

LightCamera::LightCamera(
    const ref_ptr<Light> &light,
    const ref_ptr<Camera> &userCamera,
    Vec2f extends, GLuint numLayer, GLdouble splitWeight)
: Camera(GL_FALSE),
  light_(light),
  userCamera_(userCamera),
  splitWeight_(splitWeight)
{
  lightFar_ = ref_ptr<ShaderInput1f>::alloc("lightFar");
  lightNear_ = ref_ptr<ShaderInput1f>::alloc("lightNear");
  lightMatrix_ = ref_ptr<ShaderInputMat4>::alloc("lightMatrix");

  switch(light_->lightType()) {
  case Light::DIRECTIONAL:
    numLayer_ = numLayer;
    update_ = &LightCamera::updateDirectional;
    proj_->set_elementCount(numLayer_);
    projInv_->set_elementCount(numLayer_);
    viewproj_->set_elementCount(numLayer_);
    viewprojInv_->set_elementCount(numLayer_);
    lightNear_->set_elementCount(numLayer_);
    lightNear_->set_forceArray(GL_TRUE);
    lightFar_->set_elementCount(numLayer_);
    lightFar_->set_forceArray(GL_TRUE);
    lightMatrix_->set_elementCount(numLayer_);
    lightMatrix_->set_forceArray(GL_TRUE);
    shaderDefine("RENDER_TARGET", "2D_ARRAY");
    break;

  case Light::POINT:
    numLayer_ = 6;
    update_ = &LightCamera::updatePoint;
    view_->set_elementCount(numLayer_);
    viewInv_->set_elementCount(numLayer_);
    viewproj_->set_elementCount(numLayer_);
    viewprojInv_->set_elementCount(numLayer_);
    lightMatrix_->set_elementCount(numLayer_);
    lightMatrix_->set_forceArray(GL_TRUE);
    shaderDefine("RENDER_TARGET", "CUBE");
    break;

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
  lightNear_->setUniformDataUntyped(NULL);
  lightFar_->setUniformDataUntyped(NULL);
  lightMatrix_->setUniformDataUntyped(NULL);

  lightNear_->setVertex(0, extends.x);
  lightFar_->setVertex(0, extends.y);
  setInput(lightFar_);
  setInput(lightMatrix_);

  // XXX: camera baseclass has a frustum, conflicts with light far
  frustum_->fov()->setVertex(0,90.0);
  frustum_->aspect()->setVertex(0,1.0);
  frustum_->near()->setVertex(0,extends.x);
  frustum_->far()->setVertex(0,extends.y);

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
const ref_ptr<ShaderInput1f>& LightCamera::lightFar() const
{ return lightFar_; }
const ref_ptr<ShaderInput1f>& LightCamera::lightNear() const
{ return lightNear_; }

void LightCamera::updateDirectional()
{
  Mat4f *shadowMatrices = (Mat4f*)lightMatrix_->clientDataPtr();
  lightMatrix_->nextStamp();

  // update near/far values when projection changed
  if(projectionStamp_ != userCamera_->projection()->stamp())
  {
    const Mat4f &proj = userCamera_->projection()->getVertex(0);
    // update frustum splits
    for(vector<Frustum*>::iterator
        it=shadowFrusta_.begin(); it!=shadowFrusta_.end(); ++it)
    { delete *it; }
    shadowFrusta_ = userCamera_->frustum()->split(numLayer_, splitWeight_);
    // update near/far values
    GLfloat *farValues = (GLfloat*)lightFar_->clientDataPtr();
    GLfloat *nearValues = (GLfloat*)lightNear_->clientDataPtr();
    lightFar_->nextStamp();
    lightNear_->nextStamp();
    for(GLuint i=0; i<numLayer_; ++i)
    {
      Frustum *frustum = shadowFrusta_[i];
      const GLfloat &n = frustum->near()->getVertex(0);
      const GLfloat &f = frustum->far()->getVertex(0);
      // frustum_->far() is originally in eye space - tell's us how far we can see.
      // Here we compute it in camera homogeneous coordinates. Basically, we calculate
      // proj * (0, 0, far, 1)^t and then normalize to [0; 1]
      farValues[i]  = 0.5*(-f  * proj(2,2) + proj(3,2)) / f + 0.5;
      nearValues[i] = 0.5*(-n * proj(2,2) + proj(3,2)) / n + 0.5;
    }
    projectionStamp_ = userCamera_->projection()->stamp();
  }

  // update view matrix when light direction changed
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
    lightDirStamp_ = light_->direction()->stamp();
  }

  // update view and view-projection matrices
  for(register GLuint i=0; i<numLayer_; ++i)
  {
    Frustum *frustum = shadowFrusta_[i];
    // update frustum points in world space
    frustum->computePoints(
        userCamera_->position()->getVertex(0),
        userCamera_->direction()->getVertex(0));
    const Vec3f *frustumPoints = frustum->points();

    // get the projection matrix with the new z-bounds
    // note the inversion because the light looks at the neg. z axis
    Vec2f zRange = findZRange(view_->getVertex(0), frustumPoints);

    proj_->setVertex(i, Mat4f::orthogonalMatrix(
        -1.0, 1.0, -1.0, 1.0, -zRange.y, -zRange.x));
    // find the extends of the frustum slice as projected in light's homogeneous coordinates
    Vec2f xRange(FLT_MAX,FLT_MIN);
    Vec2f yRange(FLT_MAX,FLT_MIN);
    Mat4f mvpMatrix = (view_->getVertex(0) * proj_->getVertex(i)).transpose();
    for(register GLuint j=0; j<8; ++j)
    {
        Vec4f transf = mvpMatrix * frustumPoints[j];
        transf.x /= transf.w;
        transf.y /= transf.w;
        if (transf.x > xRange.y) { xRange.y = transf.x; }
        if (transf.x < xRange.x) { xRange.x = transf.x; }
        if (transf.y > yRange.y) { yRange.y = transf.y; }
        if (transf.y < yRange.x) { yRange.x = transf.y; }
    }
    proj_->setVertex(i, proj_->getVertex(i)*Mat4f::cropMatrix(
        xRange.x, xRange.y, yRange.x, yRange.y));
    // TODO slow inverse
    projInv_->setVertex(i, proj_->getVertex(i).inverse());

    viewproj_->setVertex(i, view_->getVertex(0)*proj_->getVertex(i));
    viewprojInv_->setVertex(i, projInv_->getVertex(i)*viewInv_->getVertex(0));
    // transforms world space coordinates to homogeneous light space
    shadowMatrices[i] = viewproj_->getVertex(i) * Mat4f::bias();
  }
}

void LightCamera::updatePoint()
{
  if(lightPosStamp_    == light_->position()->stamp() &&
     lightRadiusStamp_ == light_->radius()->stamp())
  { return; }
  lightMatrix_->nextStamp();

  const Vec3f &pos = light_->position()->getVertex(0);
  GLfloat far = light_->radius()->getVertex(0).y;
  lightFar_->setVertex(0, far);

  proj_->setVertex(0, Mat4f::projectionMatrix(
      90.0, 1.0f, lightNear_->getVertex(0), far));
  projInv_->setVertex(0, proj_->getVertex(0).projectionInverse());
  Mat4f::cubeLookAtMatrices(pos, (Mat4f*)view_->clientDataPtr());

  for(register GLuint i=0; i<6; ++i) {
    if(!isCubeFaceVisible_[i]) { continue; }
    viewInv_->setVertex(i, view_->getVertex(i).lookAtInverse());
    viewproj_->setVertex(i, view_->getVertex(i)*proj_->getVertex(0));
    viewprojInv_->setVertex(i, projInv_->getVertex(0)*viewInv_->getVertex(i));
  }

  lightPosStamp_ = light_->position()->stamp();
  lightRadiusStamp_ = light_->radius()->stamp();
}

void LightCamera::updateSpot()
{
  if(lightPosStamp_    == light_->position()->stamp() &&
     lightDirStamp_    == light_->direction()->stamp() &&
     lightRadiusStamp_ == light_->radius()->stamp())
  { return; }

  const Vec3f &pos = light_->position()->getVertex(0);
  const Vec3f &dir = light_->direction()->getVertex(0);
  const Vec2f &a = light_->radius()->getVertex(0);
  lightFar_->setVertex(0, a.y);

  view_->setVertex(0, Mat4f::lookAtMatrix(pos,dir,Vec3f::up()));
  viewInv_->setVertex(0, view_->getVertex(0).lookAtInverse());

  const Vec2f &coneAngle = light_->coneAngle()->getVertex(0);
  proj_->setVertex(0, Mat4f::projectionMatrix(
      2.0*acos(coneAngle.y)*RAD_TO_DEGREE,
      1.0f,
      lightNear_->getVertex(0),
      lightFar_->getVertex(0)));
  projInv_->setVertex(0, proj_->getVertex(0).projectionInverse());

  viewproj_->setVertex(0,view_->getVertex(0) * proj_->getVertex(0));
  viewprojInv_->setVertex(0,projInv_->getVertex(0) * viewInv_->getVertex(0));
  // transforms world space coordinates to homogenous light space
  lightMatrix_->setVertex(0, viewproj_->getVertex(0) * Mat4f::bias());

  lightPosStamp_ = light_->position()->stamp();
  lightDirStamp_ = light_->direction()->stamp();
  lightRadiusStamp_ = light_->radius()->stamp();
}

void LightCamera::enable(RenderState *rs)
{
  (this->*update_)();
  Camera::enable(rs);
}
