/*
 * light-state.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include "light-state.h"
using namespace ogle;

Light::Light(Light::Type lightType)
: ShaderInputState(), lightType_(lightType)
{
  switch(lightType_) {
  case DIRECTIONAL:
    set_isAttenuated(GL_FALSE);
    break;
  default:
    set_isAttenuated(GL_TRUE);
    break;
  }

  lightPosition_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("lightPosition"));
  lightPosition_->setUniformData(Vec3f(1.0, 1.0, 1.0));
  setInput(ref_ptr<ShaderInput>::cast(lightPosition_));

  lightDirection_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("lightDirection"));
  lightDirection_->setUniformData(Vec3f(1.0, 1.0, -1.0));
  setInput(ref_ptr<ShaderInput>::cast(lightDirection_));

  lightDiffuse_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("lightDiffuse"));
  lightDiffuse_->setUniformData(Vec3f(0.7f));
  setInput(ref_ptr<ShaderInput>::cast(lightDiffuse_));

  lightSpecular_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("lightSpecular"));
  lightSpecular_->setUniformData(Vec3f(1.0f));
  setInput(ref_ptr<ShaderInput>::cast(lightSpecular_));

  lightRadius_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("lightRadius"));
  lightRadius_->setUniformData(Vec2f(999999.9,999999.9));
  setInput(ref_ptr<ShaderInput>::cast(lightRadius_));

  coneMatrix_ = ref_ptr<ModelTransformation>::manage(new ModelTransformation);
  lightConeAngles_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("lightConeAngles"));
  lightConeAngles_->setUniformData(Vec2f(0.0f));
  setInput(ref_ptr<ShaderInput>::cast(lightConeAngles_));
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
  Vec2f a = lightConeAngles_->getVertex2f(0);
  lightConeAngles_->setVertex2f(0,
      Vec2f(cos( 2.0f*M_PI*deg/360.0f ),a.y));
}
void Light::set_outerConeAngle(GLfloat deg)
{
  Vec2f a = lightConeAngles_->getVertex2f(0);
  lightConeAngles_->setVertex2f(0,
      Vec2f( a.x, cos( 2.0f*M_PI*deg/360.0f )));
}

void Light::updateConeMatrix()
{
  // Note: cone opens in positive z direction.
  Vec3f dir = lightDirection_->getVertex3f(0);
  dir.normalize();
  GLfloat angleCos = dir.dot(Vec3f(0.0,0.0,1.0));

  if(isApprox( abs(angleCos), 1.0 )) {
    coneMatrix_->set_modelMat(Mat4f::identity(), 0.0);
  }
  else {
    Vec3f axis = dir.cross(Vec3f(0.0,0.0,1.0));
    axis.normalize();

    Quaternion q;
    q.setAxisAngle(axis, acos(angleCos));
    coneMatrix_->set_modelMat(q.calculateMatrix(), 0.0);
  }
}
const ref_ptr<ShaderInputMat4>& Light::coneMatrix()
{
  // updating the cone matrix lazy....
  // TODO: do something better. input could be joined once and then
  // never be queried here again....
  if(coneMatrixStamp_ != lightDirection_->stamp())
  {
    coneMatrixStamp_ = lightDirection_->stamp();
    updateConeMatrix();
  }
  return coneMatrix_->modelMat();
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

//////////

LightNode::LightNode(
    const ref_ptr<Light> &light,
    const ref_ptr<AnimationNode> &n)
: State(), light_(light), animNode_(n)
{}
void LightNode::update(GLdouble dt)
{
  Vec3f v = animNode_->localTransform().transform(
      light_->position()->getVertex3f(0));
  light_->position()->setVertex3f(0,v);
}
