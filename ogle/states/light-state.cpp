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

Light::Light()
: State()
{
  // TODO: Light: use UBO
#define NAME(x) getUniformName(x)
  lightPositionUniform_ = ref_ptr<UniformVec4>::manage(
      new UniformVec4(NAME("lightPosition"), 1, Vec4f(-4.0, 4.0, -4.0, 0.0)));
  joinUniform( ref_ptr<Uniform>::cast(lightPositionUniform_) );

  lightAmbientUniform_ = ref_ptr<UniformVec4>::manage(
      new UniformVec4(NAME("lightAmbient"), 1, Vec4f(0.2, 0.2, 0.2, 1.0)));
  joinUniform( ref_ptr<Uniform>::cast(lightAmbientUniform_) );

  lightDiffuseUniform_ = ref_ptr<UniformVec4>::manage(
      new UniformVec4(NAME("lightDiffuse"), 1, Vec4f(1.0, 1.0, 1.0, 1.0)));
  joinUniform( ref_ptr<Uniform>::cast(lightDiffuseUniform_) );

  lightSpecularUniform_ = ref_ptr<UniformVec4>::manage(
      new UniformVec4(NAME("lightSpecular"), 1, Vec4f(1.0, 1.0, 1.0, 1.0)));
  joinUniform( ref_ptr<Uniform>::cast(lightSpecularUniform_) );

  lightInnerConeAngleUniform_ = ref_ptr<UniformFloat>::manage(
      new UniformFloat(NAME("lightInnerConeAngle"), 1, cos( 0.4*M_PI)));
  joinUniform( ref_ptr<Uniform>::cast(lightInnerConeAngleUniform_) );

  lightOuterConeAngleUniform_ = ref_ptr<UniformFloat>::manage(
      new UniformFloat(NAME("lightOuterConeAngle"), 1, cos( 0.6*M_PI )));
  joinUniform( ref_ptr<Uniform>::cast(lightOuterConeAngleUniform_) );

  lightSpotDirectionUniform_ = ref_ptr<UniformVec3>::manage(
      new UniformVec3(NAME("lightSpotDirection"), 1, Vec3f(1.0, 1.0, 1.0)));
  lightSpotExponentUniform_ = ref_ptr<UniformFloat>::manage(
      new UniformFloat(NAME("lightSpotExponent"), 1, 0.0f));
  joinUniform( ref_ptr<Uniform>::cast(lightSpotDirectionUniform_) );
  joinUniform( ref_ptr<Uniform>::cast(lightSpotExponentUniform_) );

  lightConstantAttenuationUniform_ = ref_ptr<UniformFloat>::manage(
      new UniformFloat(NAME("lightConstantAttenuation"), 1, 0.0002f));
  lightLinearAttenuationUniform_ = ref_ptr<UniformFloat>::manage(
      new UniformFloat(NAME("lightLinearAttenuation"), 1, 0.002f));
  lightQuadricAttenuationUniform_ = ref_ptr<UniformFloat>::manage(
      new UniformFloat(NAME("lightQuadricAttenuation"), 1, 0.002f));

  updateType(DIRECTIONAL);
#undef NAME
}

string Light::name()
{
  return FORMAT_STRING("Light");
}

string Light::getUniformName(const string &uni)
{
  return FORMAT_STRING(uni << this);
}

void Light::updateType(LightType oldType)
{
  LightType newType = getLightType();
  if(oldType == newType) { return; }
  switch(oldType) {
  case DIRECTIONAL:
  case SPOT:
    disjoinUniform( ref_ptr<Uniform>::cast(lightSpotDirectionUniform_) );
    disjoinUniform( ref_ptr<Uniform>::cast(lightSpotExponentUniform_) );
    // fall through
  case POINT:
    disjoinUniform( ref_ptr<Uniform>::cast(lightConstantAttenuationUniform_) );
    disjoinUniform( ref_ptr<Uniform>::cast(lightLinearAttenuationUniform_) );
    disjoinUniform( ref_ptr<Uniform>::cast(lightQuadricAttenuationUniform_) );
  }
  switch(newType) {
  case DIRECTIONAL:
  case SPOT:
    joinUniform( ref_ptr<Uniform>::cast(lightSpotDirectionUniform_) );
    joinUniform( ref_ptr<Uniform>::cast(lightSpotExponentUniform_) );
    // fall through
  case POINT:
    joinUniform( ref_ptr<Uniform>::cast(lightConstantAttenuationUniform_) );
    joinUniform( ref_ptr<Uniform>::cast(lightLinearAttenuationUniform_) );
    joinUniform( ref_ptr<Uniform>::cast(lightQuadricAttenuationUniform_) );
  }
}

const  Vec4f& Light::position() const
{
  return lightPositionUniform_->value();
}
void Light::set_position(const Vec4f &position)
{
  LightType oldType = getLightType();
  lightPositionUniform_->set_value( position );
  updateType(oldType);
}

const Vec4f& Light::diffuse() const
{
  return lightDiffuseUniform_->value();
}
void Light::set_diffuse(const Vec4f &diffuse)
{
  lightDiffuseUniform_->set_value( diffuse );
}

const Vec4f& Light::ambient() const
{
  return lightAmbientUniform_->value();
}
void Light::set_ambient(const Vec4f &ambient)
{
  lightAmbientUniform_->set_value( ambient );
}

const Vec4f& Light::specular() const
{
  return lightSpecularUniform_->value();
}
void Light::set_specular(const Vec4f &specular)
{
  lightSpecularUniform_->set_value( specular );
}

float Light::constantAttenuation() const
{
  return lightConstantAttenuationUniform_->value();
}
void Light::set_constantAttenuation(float constantAttenuation)
{
  lightConstantAttenuationUniform_->set_value( constantAttenuation );
}

float Light::linearAttenuation() const
{
  return lightLinearAttenuationUniform_->value();
}
void Light::set_linearAttenuation(float linearAttenuation)
{
  lightLinearAttenuationUniform_->set_value( linearAttenuation );
}

float Light::quadricAttenuation() const
{
  return lightQuadricAttenuationUniform_->value();
}
void Light::set_quadricAttenuation(float quadricAttenuation)
{
  lightQuadricAttenuationUniform_->set_value( quadricAttenuation );
}

const Vec3f& Light::spotDirection() const
{
  return lightSpotDirectionUniform_->value();
}
void Light::set_spotDirection(const Vec3f &spotDirection)
{
  lightSpotDirectionUniform_->set_value( spotDirection );
}

float Light::spotExponent() const
{
  return lightSpotExponentUniform_->value();
}
void Light::set_spotExponent(float spotExponent)
{
  lightSpotExponentUniform_->set_value( spotExponent );
}

float Light::innerConeAngle() const
{
  return lightInnerConeAngleUniform_->value();
}
void Light::set_innerConeAngle(float v)
{
  LightType oldType = getLightType();
  lightInnerConeAngleUniform_->set_value( cos( 2.0f*M_PI*v/360.0f ) );
  updateType(oldType);
}

float Light::outerConeAngle() const
{
  return lightOuterConeAngleUniform_->value();
}
void Light::set_outerConeAngle(float v)
{
  lightOuterConeAngleUniform_->set_value( cos( 2.0f*M_PI*v/360.0f ) );
}

Light::LightType Light::getLightType() const
{
  if(position().w==0.0) {
    return Light::DIRECTIONAL;
  } else if(innerConeAngle()+0.001>=M_PI) {
    return Light::POINT;
  } else {
    return Light::SPOT;
  }
}

void Light::configureShader(ShaderConfiguration *cfg)
{
  State::configureShader(cfg);
  cfg->addLight(this);
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
string LightNode::name()
{
  return "LightNode";
}

void LightNode::update(GLdouble dt)
{
  Vec3f lightPos = transformVec3(animNode_->localTransform(), untransformedPos_);
  light_->set_position( Vec4f(
      lightPos.x, lightPos.y, lightPos.z,
      light_->position().w
  ));
}
