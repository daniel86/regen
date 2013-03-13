/*
 * light-state.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include "light-state.h"
using namespace ogle;

Light::Light()
: ShaderInputState(),
  isAttenuated_(GL_TRUE)
{
  lightDiffuse_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("lightDiffuse"));
  lightDiffuse_->setUniformData(Vec3f(0.7f));
  setInput(ref_ptr<ShaderInput>::cast(lightDiffuse_));

  lightSpecular_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("lightSpecular"));
  lightSpecular_->setUniformData(Vec3f(1.0f));
  setInput(ref_ptr<ShaderInput>::cast(lightSpecular_));

  lightRadius_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("lightRadius"));
  lightRadius_->setUniformData(Vec2f(999999.9,999999.9));
  setInput(ref_ptr<ShaderInput>::cast(lightRadius_));
}

const ref_ptr<ShaderInput3f>& Light::diffuse() const
{
  return lightDiffuse_;
}
void Light::set_diffuse(const Vec3f &diffuse)
{
  lightDiffuse_->setVertex3f( 0, diffuse );
}

GLboolean Light::isAttenuated() const
{
  return isAttenuated_;
}
void Light::set_isAttenuated(GLboolean isAttenuated)
{
  isAttenuated_ = isAttenuated;
}

const ref_ptr<ShaderInput2f>& Light::radius() const
{
  return lightRadius_;
}
void Light::set_innerRadius(GLfloat v)
{
  Vec2f r = lightRadius_->getVertex2f(0);
  lightRadius_->setVertex2f(0, Vec2f(v,r.y));
}
void Light::set_outerRadius(GLfloat v)
{
  Vec2f r = lightRadius_->getVertex2f(0);
  lightRadius_->setVertex2f(0, Vec2f(r.x,v));
}

const ref_ptr<ShaderInput3f>& Light::specular() const
{
  return lightSpecular_;
}
void Light::set_specular(const Vec3f &specular)
{
  lightSpecular_->setVertex3f( 0, specular );
}

//////////

DirectionalLight::DirectionalLight()
: Light()
{
  lightDirection_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("lightDirection"));
  lightDirection_->setUniformData(Vec3f(1.0, 1.0, -1.0));
  setInput(ref_ptr<ShaderInput>::cast(lightDirection_));
  set_isAttenuated(GL_FALSE);
}

const ref_ptr<ShaderInput3f>& DirectionalLight::direction() const
{
  return lightDirection_;
}
void DirectionalLight::set_direction(const Vec3f &direction)
{
  lightDirection_->setVertex3f( 0, direction );
}

//////////

PointLight::PointLight()
: Light()
{
  lightPosition_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("lightPosition"));
  lightPosition_->setUniformData(Vec3f(1.0, 1.0, 1.0));
  setInput(ref_ptr<ShaderInput>::cast(lightPosition_));
  set_isAttenuated(GL_TRUE);
}

const ref_ptr<ShaderInput3f>& PointLight::position() const
{
  return lightPosition_;
}
void PointLight::set_position(const Vec3f &position)
{
  lightPosition_->setVertex3f( 0, position );
}

//////////

SpotLight::SpotLight()
: Light(), coneMatrixStamp_(0)
{
  lightPosition_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("lightPosition"));
  lightPosition_->setUniformData(Vec3f(1.0, 1.0, 1.0));
  setInput(ref_ptr<ShaderInput>::cast(lightPosition_));

  lightSpotDirection_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("lightSpotDirection"));
  lightSpotDirection_->setUniformData(Vec3f(1.0));
  setInput(ref_ptr<ShaderInput>::cast(lightSpotDirection_));

  lightConeAngles_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("lightConeAngles"));
  lightConeAngles_->setUniformData(Vec2f(0.0f));
  setInput(ref_ptr<ShaderInput>::cast(lightConeAngles_));

  coneMatrix_ = ref_ptr<ModelTransformation>::manage(new ModelTransformation);

  set_innerConeAngle(50.0f);
  set_outerConeAngle(55.0f);
  set_isAttenuated(GL_TRUE);
}

const ref_ptr<ShaderInput3f>& SpotLight::spotDirection() const
{
  return lightSpotDirection_;
}
void SpotLight::set_spotDirection(const Vec3f &spotDirection)
{
  Vec3f dir(spotDirection);
  dir.normalize();
  lightSpotDirection_->setVertex3f( 0, dir );
}

const ref_ptr<ShaderInput2f>& SpotLight::coneAngle() const
{
  return lightConeAngles_;
}
void SpotLight::set_innerConeAngle(GLfloat deg)
{
  Vec2f a = lightConeAngles_->getVertex2f(0);
  lightConeAngles_->setVertex2f(0,
      Vec2f(cos( 2.0f*M_PI*deg/360.0f ),a.y));
}
void SpotLight::set_outerConeAngle(GLfloat deg)
{
  Vec2f a = lightConeAngles_->getVertex2f(0);
  lightConeAngles_->setVertex2f(0,
      Vec2f( a.x, cos( 2.0f*M_PI*deg/360.0f )));
}

const ref_ptr<ShaderInput3f>& SpotLight::position() const
{
  return lightPosition_;
}
void SpotLight::set_position(const Vec3f &position)
{
  lightPosition_->setVertex3f( 0, position );
}

void SpotLight::updateConeMatrix()
{
  // Note: cone opens in positive z direction.
  Vec3f dir = lightSpotDirection_->getVertex3f(0);
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
const ref_ptr<ShaderInputMat4>& SpotLight::coneMatrix()
{
  // updating the cone matrix lazy....
  // TODO: do something better. input could be joined once and then
  // never be queried here again....
  if(coneMatrixStamp_ != lightSpotDirection_->stamp())
  {
    coneMatrixStamp_ = lightSpotDirection_->stamp();
    updateConeMatrix();
  }

  return coneMatrix_->modelMat();
}

//////////

LightNode::LightNode(
    const ref_ptr<Light> &light,
    const ref_ptr<AnimationNode> &animNode,
    const Vec3f &untransformedPos)
: State(),
  light_(light),
  animNode_(animNode),
  untransformedPos_(untransformedPos_)
{
}
void LightNode::update(GLdouble dt)
{
  Vec3f v = animNode_->localTransform().transform(untransformedPos_);
  if(dynamic_cast<PointLight*>(light_.get())) {
    dynamic_cast<PointLight*>(light_.get())->set_position(v);
  }
  else if(dynamic_cast<SpotLight*>(light_.get())) {
    dynamic_cast<SpotLight*>(light_.get())->set_position(v);
  }
  else if(dynamic_cast<DirectionalLight*>(light_.get())) {
    dynamic_cast<DirectionalLight*>(light_.get())->set_direction(v);
  }
}
