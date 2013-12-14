/*
 * light-state.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include "light-state.h"
using namespace regen;

#include <cfloat>

namespace regen {
  ostream& operator<<(ostream &out, const Light::Type &type)
  {
    switch(type) {
    case Light::DIRECTIONAL: return out << "DIRECTIONAL";
    case Light::SPOT:        return out << "SPOT";
    case Light::POINT:       return out << "POINT";
    }
    return out;
  }
  istream& operator>>(istream &in, Light::Type &type)
  {
    string val;
    in >> val;
    boost::to_upper(val);
    if(val == "DIRECTIONAL") type = Light::DIRECTIONAL;
    else if(val == "SPOT")   type = Light::SPOT;
    else if(val == "POINT")  type = Light::POINT;
    else {
      REGEN_WARN("Unknown light type '" << val << "'. Using SPOT light.");
      type = Light::SPOT;
    }
    return in;
  }

  ostream& operator<<(ostream &out, const ShadowFilterMode &mode)
  {
    switch(mode) {
    case SHADOW_FILTERING_NONE:         return out << "NONE";
    case SHADOW_FILTERING_PCF_GAUSSIAN: return out << "PCF_GAUSSIAN";
    case SHADOW_FILTERING_VSM:          return out << "VSM";
    }
    return out;
  }
  istream& operator>>(istream &in, ShadowFilterMode &mode)
  {
    string val;
    in >> val;
    boost::to_upper(val);
    if(val == "NONE")              mode = SHADOW_FILTERING_NONE;
    else if(val == "PCF_GAUSSIAN") mode = SHADOW_FILTERING_PCF_GAUSSIAN;
    else if(val == "VSM")          mode = SHADOW_FILTERING_VSM;
    else {
      REGEN_WARN("Unknown shadow filtering mode '" << val << "'. Using no filtering.");
      mode = SHADOW_FILTERING_NONE;
    }
    return in;
  }
}

Light::Light(Light::Type lightType)
: State(),
  Animation(GL_TRUE, GL_FALSE, lightType==SPOT),
  HasInput(VBO::USAGE_DYNAMIC),
  lightType_(lightType)
{
  switch(lightType_) {
  case DIRECTIONAL:
    set_isAttenuated(GL_FALSE);
    break;
  default:
    set_isAttenuated(GL_TRUE);
    break;
  }

  lightPosition_ = ref_ptr<ShaderInput3f>::alloc("lightPosition");
  lightPosition_->setUniformData(Vec3f(1.0, 1.0, 1.0));
  setInput(lightPosition_);

  lightDirection_ = ref_ptr<ShaderInput3f>::alloc("lightDirection");
  lightDirection_->setUniformData(Vec3f(1.0, 1.0, -1.0));
  setInput(lightDirection_);

  lightDiffuse_ = ref_ptr<ShaderInput3f>::alloc("lightDiffuse");
  lightDiffuse_->setUniformData(Vec3f(0.7f));
  setInput(lightDiffuse_);

  lightSpecular_ = ref_ptr<ShaderInput3f>::alloc("lightSpecular");
  lightSpecular_->setUniformData(Vec3f(1.0f));
  setInput(lightSpecular_);

  lightRadius_ = ref_ptr<ShaderInput2f>::alloc("lightRadius");
  lightRadius_->setUniformData(Vec2f(999999.9,999999.9));
  setInput(lightRadius_);

  coneMatrix_ = ref_ptr<ModelTransformation>::alloc();
  lightConeAngles_ = ref_ptr<ShaderInput2f>::alloc("lightConeAngles");
  lightConeAngles_->setUniformData(Vec2f(0.0f));
  setInput(lightConeAngles_);
  set_innerConeAngle(50.0f);
  set_outerConeAngle(55.0f);
}

Light::Type Light::lightType() const
{ return lightType_; }

GLboolean Light::isAttenuated() const
{ return isAttenuated_; }
void Light::set_isAttenuated(GLboolean isAttenuated)
{ isAttenuated_ = isAttenuated; }

void Light::set_innerConeAngle(GLfloat deg)
{
  Vec2f a = lightConeAngles_->getVertex(0);
  lightConeAngles_->setVertex(0,
      Vec2f(cos( 2.0f*M_PI*deg/360.0f ),a.y));
}
void Light::set_outerConeAngle(GLfloat deg)
{
  Vec2f a = lightConeAngles_->getVertex(0);
  lightConeAngles_->setVertex(0,
      Vec2f( a.x, cos( 2.0f*M_PI*deg/360.0f )));
}

void Light::updateConeMatrix()
{
  const Vec3f &pos = lightPosition_->getVertex(0);
  // Note: cone opens in positive z direction.
  Vec3f dir = lightDirection_->getVertex(0);
  dir.normalize();
  GLfloat angleCos = dir.dot(Vec3f(0.0,0.0,1.0));

  if(math::isApprox( abs(angleCos), 1.0 )) {
    coneMatrix_->set_modelMat(Mat4f::identity(), 0.0);
  }
  else {
    const GLfloat &radius = lightRadius_->getVertex(0).y;
    const GLfloat &coneAngle = lightConeAngles_->getVertex(0).y;

    // Quaternion rotates view to light direction
    Quaternion q;
    Vec3f axis = dir.cross(Vec3f(0.0,0.0,1.0));
    axis.normalize();
    q.setAxisAngle(axis, acos(angleCos));

    // scale `unit`-cone, rotate to light direction and finally translate to light position
    GLfloat x = 2.0f*radius*tan(acos(coneAngle));

    Mat4f val = q.calculateMatrix();
    val.scale(Vec3f(x,x,radius));
    coneMatrix_->set_modelMat(val, 0.0);
    coneMatrix_->translate(pos, 0.0);
  }
}

void Light::glAnimate(RenderState *rs, GLdouble dt)
{
  GLuint stamp = max(lightRadius_->stamp(), max(lightDirection_->stamp(),
      max(lightConeAngles_->stamp(), lightPosition_->stamp())));
  if(coneMatrixStamp_ != stamp)
  {
    coneMatrixStamp_ = stamp;
    updateConeMatrix();
  }
}

const ref_ptr<ShaderInput2f>& Light::radius() const
{ return lightRadius_; }
const ref_ptr<ShaderInput2f>& Light::coneAngle() const
{ return lightConeAngles_; }
const ref_ptr<ShaderInput3f>& Light::position() const
{ return lightPosition_; }
const ref_ptr<ShaderInput3f>& Light::direction() const
{ return lightDirection_; }
const ref_ptr<ShaderInput3f>& Light::diffuse() const
{ return lightDiffuse_; }
const ref_ptr<ShaderInput3f>& Light::specular() const
{ return lightSpecular_; }
const ref_ptr<ShaderInputMat4>& Light::coneMatrix()
{ return coneMatrix_->modelMat(); }

//////////
//////////
//////////

LightNode::LightNode(
    const ref_ptr<Light> &light,
    const ref_ptr<AnimationNode> &n)
: State(), light_(light), animNode_(n)
{}
void LightNode::update(GLdouble dt)
{
  Vec3f v = animNode_->localTransform().transformVector(
      light_->position()->getVertex(0));
  light_->position()->setVertex(0,v);
}

//////////
//////////
//////////

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
  projInv_->setVertex(0, view_->getVertex(0).projectionInverse());
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
  projInv_->setVertex(0, view_->getVertex(0).projectionInverse());

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
