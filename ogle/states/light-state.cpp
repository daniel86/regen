/*
 * light.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include <sstream>

#include <ogle/states/light-state.h>
#include <ogle/states/material-state.h>
#include <ogle/algebra/frustum.h>
#include <ogle/utility/ref-ptr.h>
#include <ogle/utility/string-util.h>

GLint Light::idCounter_ = 0;

#define __LIGHT_NAME(x) FORMAT_STRING(x << id_)

Light::Light()
: ShaderInputState(),
  id_(++idCounter_),
  isAttenuated_(GL_TRUE)
{
  lightDiffuse_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f(__LIGHT_NAME("lightDiffuse")));
  lightDiffuse_->setUniformData(Vec3f(0.7f));
  setInput(ref_ptr<ShaderInput>::cast(lightDiffuse_));

  lightSpecular_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f(__LIGHT_NAME("lightSpecular")));
  lightSpecular_->setUniformData(Vec3f(1.0f));
  setInput(ref_ptr<ShaderInput>::cast(lightSpecular_));

  lightRadius_ = ref_ptr<ShaderInput2f>::manage(
      new ShaderInput2f(__LIGHT_NAME("lightRadius")));
  lightRadius_->setUniformData(Vec2f(999999.9,999999.9));
  setInput(ref_ptr<ShaderInput>::cast(lightRadius_));
}

GLint Light::id() const
{
  return id_;
}

const ref_ptr<ShaderInput3f>& Light::diffuse() const
{
  return lightDiffuse_;
}
void Light::set_diffuse(const Vec3f &diffuse)
{
  lightDiffuse_->setVertex3f( 0, diffuse );
}

void Light::set_isAttenuated(GLboolean isAttenuated)
{
  if(isAttenuated) {
    shaderDefine(__LIGHT_NAME("LIGHT_IS_ATTENUATED"),"TRUE");
  } else {
    shaderDefine(__LIGHT_NAME("LIGHT_IS_ATTENUATED"),"FALSE");
  }
}
const ref_ptr<ShaderInput2f>& Light::radius() const
{
  return lightRadius_;
}
void Light::set_innerRadius(GLfloat v)
{
  lightRadius_->getVertex2f(0).x = v;
}
void Light::set_outerRadius(GLfloat v)
{
  lightRadius_->getVertex2f(0).y = v;
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
  lightDirection_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f(__LIGHT_NAME("lightDirection")));
  lightDirection_->setUniformData(Vec3f(1.0, 1.0, -1.0));
  setInput(ref_ptr<ShaderInput>::cast(lightDirection_));

  set_isAttenuated(GL_FALSE);
  shaderDefine(__LIGHT_NAME("LIGHT_TYPE"), "DIRECTIONAL");
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
  lightPosition_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f(__LIGHT_NAME("lightPosition")));
  lightPosition_->setUniformData(Vec3f(1.0, 1.0, 1.0));
  setInput(ref_ptr<ShaderInput>::cast(lightPosition_));

  set_isAttenuated(GL_TRUE);
  shaderDefine(__LIGHT_NAME("LIGHT_TYPE"), "POINT");
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
  lightPosition_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f(__LIGHT_NAME("lightPosition")));
  lightPosition_->setUniformData(Vec3f(1.0, 1.0, 1.0));
  setInput(ref_ptr<ShaderInput>::cast(lightPosition_));

  lightSpotDirection_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f(__LIGHT_NAME("lightSpotDirection")));
  lightSpotDirection_->setUniformData(Vec3f(1.0));
  setInput(ref_ptr<ShaderInput>::cast(lightSpotDirection_));

  lightConeAngles_ = ref_ptr<ShaderInput2f>::manage(
      new ShaderInput2f(__LIGHT_NAME("lightConeAngles")));
  lightConeAngles_->setUniformData(Vec2f(0.0f));
  setInput(ref_ptr<ShaderInput>::cast(lightConeAngles_));

  coneMatrix_ = ref_ptr<ModelTransformation>::manage(new ModelTransformation);

  set_innerConeAngle(50.0f);
  set_outerConeAngle(55.0f);
  set_isAttenuated(GL_TRUE);
  shaderDefine(__LIGHT_NAME("LIGHT_TYPE"), "SPOT");
}

const ref_ptr<ShaderInput3f>& SpotLight::spotDirection() const
{
  return lightSpotDirection_;
}
void SpotLight::set_spotDirection(const Vec3f &spotDirection)
{
  lightSpotDirection_->setVertex3f( 0, spotDirection );
  lightSpotDirection_->getVertex3f( 0 ).normalize();
}

const ref_ptr<ShaderInput2f>& SpotLight::coneAngle() const
{
  return lightConeAngles_;
}
void SpotLight::set_innerConeAngle(GLfloat deg)
{
  lightConeAngles_->getVertex2f(0).x = cos( 2.0f*M_PI*deg/360.0f );
}
void SpotLight::set_outerConeAngle(GLfloat deg)
{
  lightConeAngles_->getVertex2f(0).y = cos( 2.0f*M_PI*deg/360.0f );
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

SpotLightNode::SpotLightNode(
    const ref_ptr<SpotLight> &light,
    const ref_ptr<AnimationNode> &animNode,
    const Vec3f &untransformedPos)
: LightNode(ref_ptr<Light>::cast(light),animNode,untransformedPos),
  spotLight_(light)
{
}
PointLightNode::PointLightNode(
    const ref_ptr<PointLight> &light,
    const ref_ptr<AnimationNode> &animNode,
    const Vec3f &untransformedPos)
: LightNode(ref_ptr<Light>::cast(light),animNode,untransformedPos),
  pointLight_(light)
{
}
DirectionalLightNode::DirectionalLightNode(
    const ref_ptr<DirectionalLight> &light,
    const ref_ptr<AnimationNode> &animNode,
    const Vec3f &untransformedPos)
: LightNode(ref_ptr<Light>::cast(light),animNode,untransformedPos),
  dirLight_(light)
{
}

void SpotLightNode::update(GLdouble dt)
{
  Vec3f lightPos = animNode_->localTransform().transform(untransformedPos_);
  spotLight_->set_position(lightPos);
}
void PointLightNode::update(GLdouble dt)
{
  Vec3f lightPos = animNode_->localTransform().transform(untransformedPos_);
  pointLight_->set_position(lightPos);
}
void DirectionalLightNode::update(GLdouble dt)
{
  Vec3f lightPos = animNode_->localTransform().transform(untransformedPos_);
  dirLight_->set_direction(lightPos);
}
