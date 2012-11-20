/*
 * light.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include <sstream>

#include <ogle/states/light-state.h>
#include <ogle/algebra/frustum.h>
#include <ogle/utility/ref-ptr.h>
#include <ogle/utility/string-util.h>

long Light::idCounter_ = 0;

#define __LIGHT_NAME(x) FORMAT_STRING(x << id_)

Light::Light()
: State(), id_(++idCounter_)
{
  lightAmbient_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f(__LIGHT_NAME("lightAmbient")));
  lightAmbient_->setUniformData(Vec3f(0.0f));
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightAmbient_) );

  lightDiffuse_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f(__LIGHT_NAME("lightDiffuse")));
  lightDiffuse_->setUniformData(Vec3f(1.0f));
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightDiffuse_) );

  lightSpecular_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f(__LIGHT_NAME("lightSpecular")));
  lightSpecular_->setUniformData(Vec3f(1.0f));
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightSpecular_) );
}

long Light::id()
{
  return id_;
}

ref_ptr<ShaderInput3f>& Light::diffuse()
{
  return lightDiffuse_;
}
void Light::set_diffuse(const Vec3f &diffuse)
{
  lightDiffuse_->setVertex3f( 0, diffuse );
}

ref_ptr<ShaderInput3f>& Light::ambient()
{
  return lightAmbient_;
}
void Light::set_ambient(const Vec3f &ambient)
{
  lightAmbient_->setVertex3f( 0, ambient );
}

ref_ptr<ShaderInput3f>& Light::specular()
{
  return lightSpecular_;
}
void Light::set_specular(const Vec3f &specular)
{
  lightSpecular_->setVertex3f( 0, specular );
}

void Light::configureShader(ShaderConfig *cfg)
{
  State::configureShader(cfg);
  cfg->addLight(this);
}

//////////

DirectionalLight::DirectionalLight()
: Light()
{
  lightDirection_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f(__LIGHT_NAME("lightDirection")));
  lightDirection_->setUniformData(Vec3f(1.0, 1.0, 1.0));
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightDirection_) );
}

ref_ptr<ShaderInput3f>& DirectionalLight::direction()
{
  return lightDirection_;
}
void DirectionalLight::set_direction(const Vec3f &direction)
{
  lightDirection_->setVertex3f( 0, direction );
}

//////////

AttenuatedLight::AttenuatedLight()
: Light()
{
  lightAttenuation_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f(__LIGHT_NAME("lightAttenuation")));
  lightAttenuation_->setUniformData(Vec3f(0.0002f,0.002f,0.002f));
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightAttenuation_) );
}

ref_ptr<ShaderInput3f>& AttenuatedLight::attenuation()
{
  return lightAttenuation_;
}
void AttenuatedLight::set_constantAttenuation(GLfloat constantAttenuation)
{
  lightAttenuation_->getVertex3f(0).x = constantAttenuation;
}
void AttenuatedLight::set_linearAttenuation(GLfloat linearAttenuation)
{
  lightAttenuation_->getVertex3f(0).y = linearAttenuation;
}
void AttenuatedLight::set_quadricAttenuation(float quadricAttenuation)
{
  lightAttenuation_->getVertex3f(0).z = quadricAttenuation;
}

//////////

PointLight::PointLight()
: AttenuatedLight()
{
  lightPosition_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f(__LIGHT_NAME("lightPosition")));
  lightPosition_->setUniformData(Vec3f(1.0, 1.0, 1.0));
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightPosition_) );
}

ref_ptr<ShaderInput3f>& PointLight::position()
{
  return lightPosition_;
}
void PointLight::set_position(const Vec3f &position)
{
  lightPosition_->setVertex3f( 0, position );
}

//////////

SpotLight::SpotLight()
: AttenuatedLight()
{
  lightPosition_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f(__LIGHT_NAME("lightPosition")));
  lightPosition_->setUniformData(Vec3f(1.0, 1.0, 1.0));
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightPosition_) );

  lightSpotDirection_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f(__LIGHT_NAME("lightSpotDirection")));
  lightSpotDirection_->setUniformData(Vec3f(1.0));
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightSpotDirection_) );

  lightSpotExponent_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f(__LIGHT_NAME("lightSpotExponent")));
  lightSpotExponent_->setUniformData(0.0f);
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightSpotExponent_) );

  lightConeAngle_ = ref_ptr<ShaderInput2f>::manage(
      new ShaderInput2f(__LIGHT_NAME("lightConeAngle")));
  lightConeAngle_->setUniformData(Vec2f(cos( 0.4*M_PI), cos( 0.6*M_PI)));
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightConeAngle_) );
}

ref_ptr<ShaderInput3f>& SpotLight::spotDirection()
{
  return lightSpotDirection_;
}
void SpotLight::set_spotDirection(const Vec3f &spotDirection)
{
  lightSpotDirection_->setVertex3f( 0, spotDirection );
}

ref_ptr<ShaderInput1f>& SpotLight::spotExponent()
{
  return lightSpotExponent_;
}
void SpotLight::set_spotExponent(GLfloat spotExponent)
{
  lightSpotExponent_->setVertex1f( 0, spotExponent );
}

ref_ptr<ShaderInput2f>& SpotLight::coneAngle()
{
  return lightConeAngle_;
}
void SpotLight::set_innerConeAngle(GLfloat v)
{
  lightConeAngle_->getVertex2f(0).x = cos( 2.0f*M_PI*v/360.0f );
}
void SpotLight::set_outerConeAngle(GLfloat v)
{
  lightConeAngle_->getVertex2f(0).y = cos( 2.0f*M_PI*v/360.0f );
}

ref_ptr<ShaderInput3f>& SpotLight::position()
{
  return lightPosition_;
}
void SpotLight::set_position(const Vec3f &position)
{
  lightPosition_->setVertex3f( 0, position );
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

void SpotLightNode::update(GLdouble dt)
{
  Vec3f lightPos = transformVec3(animNode_->localTransform(), untransformedPos_);
  spotLight_->set_position(lightPos);
}

#undef __LIGHT_NAME
