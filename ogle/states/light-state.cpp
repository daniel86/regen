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

Light::Light()
: State(),
  id_(++idCounter_)
{
#define NAME(x) FORMAT_STRING(x << id_)
  lightPosition_ = ref_ptr<ShaderInput4f>::manage(
      new ShaderInput4f(NAME("lightPosition")));
  lightPosition_->setUniformData(Vec4f(4.0, 4.0, 4.0, 0.0));
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightPosition_) );

  lightAmbient_ = ref_ptr<ShaderInput4f>::manage(
      new ShaderInput4f(NAME("lightAmbient")));
  lightAmbient_->setUniformData(Vec4f(0.2, 0.2, 0.2, 1.0));
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightAmbient_) );

  lightDiffuse_ = ref_ptr<ShaderInput4f>::manage(
      new ShaderInput4f(NAME("lightDiffuse")));
  lightDiffuse_->setUniformData(Vec4f(1.0));
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightDiffuse_) );

  lightSpecular_ = ref_ptr<ShaderInput4f>::manage(
      new ShaderInput4f(NAME("lightSpecular")));
  lightSpecular_->setUniformData(Vec4f(1.0));
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightSpecular_) );

  lightInnerConeAngle_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f(NAME("lightInnerConeAngle")));
  lightInnerConeAngle_->setUniformData(cos( 0.4*M_PI));
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightInnerConeAngle_) );

  lightOuterConeAngle_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f(NAME("lightOuterConeAngle")));
  lightOuterConeAngle_->setUniformData(cos( 0.6*M_PI));
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightOuterConeAngle_) );

  lightSpotDirection_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f(NAME("lightSpotDirection")));
  lightSpotDirection_->setUniformData(Vec3f(1.0));

  lightSpotExponent_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f(NAME("lightSpotExponent")));
  lightSpotExponent_->setUniformData(0.0f);

  lightConstantAttenuation_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f(NAME("lightConstantAttenuation")));
  lightConstantAttenuation_->setUniformData(0.0002f);

  lightLinearAttenuation_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f(NAME("lightLinearAttenuation")));
  lightLinearAttenuation_->setUniformData(0.002f);

  lightQuadricAttenuation_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f(NAME("lightQuadricAttenuation")));
  lightQuadricAttenuation_->setUniformData(0.002f);

  updateType(DIRECTIONAL);
#undef NAME
}

string Light::name()
{
  return FORMAT_STRING("Light");
}

long Light::id()
{
  return id_;
}

void Light::updateType(LightType oldType)
{
  LightType newType = getLightType();
  if(oldType == newType) { return; }
  switch(oldType) {
  case DIRECTIONAL:
    break;
  case SPOT:
    disjoinShaderInput( ref_ptr<ShaderInput>::cast(lightSpotDirection_) );
    disjoinShaderInput( ref_ptr<ShaderInput>::cast(lightSpotExponent_) );
    // fall through
  case POINT:
    disjoinShaderInput( ref_ptr<ShaderInput>::cast(lightConstantAttenuation_) );
    disjoinShaderInput( ref_ptr<ShaderInput>::cast(lightLinearAttenuation_) );
    disjoinShaderInput( ref_ptr<ShaderInput>::cast(lightQuadricAttenuation_) );
    break;
  }
  switch(newType) {
  case DIRECTIONAL:
    break;
  case SPOT:
    joinShaderInput( ref_ptr<ShaderInput>::cast(lightSpotDirection_) );
    joinShaderInput( ref_ptr<ShaderInput>::cast(lightSpotExponent_) );
    // fall through
  case POINT:
    joinShaderInput( ref_ptr<ShaderInput>::cast(lightConstantAttenuation_) );
    joinShaderInput( ref_ptr<ShaderInput>::cast(lightLinearAttenuation_) );
    joinShaderInput( ref_ptr<ShaderInput>::cast(lightQuadricAttenuation_) );
    break;
  }
}

ref_ptr<ShaderInput4f>& Light::position()
{
  return lightPosition_;
}
void Light::set_position(const Vec4f &position)
{
  LightType oldType = getLightType();
  lightPosition_->setUniformData( position );
  updateType(oldType);
}

ref_ptr<ShaderInput4f>& Light::diffuse()
{
  return lightDiffuse_;
}
void Light::set_diffuse(const Vec4f &diffuse)
{
  lightDiffuse_->setUniformData( diffuse );
}

ref_ptr<ShaderInput4f>& Light::ambient()
{
  return lightAmbient_;
}
void Light::set_ambient(const Vec4f &ambient)
{
  lightAmbient_->setUniformData( ambient );
}

ref_ptr<ShaderInput4f>& Light::specular()
{
  return lightSpecular_;
}
void Light::set_specular(const Vec4f &specular)
{
  lightSpecular_->setUniformData( specular );
}

ref_ptr<ShaderInput1f>& Light::constantAttenuation()
{
  return lightConstantAttenuation_;
}
void Light::set_constantAttenuation(GLfloat constantAttenuation)
{
  lightConstantAttenuation_->setUniformData( constantAttenuation );
}

ref_ptr<ShaderInput1f>& Light::linearAttenuation()
{
  return lightLinearAttenuation_;
}
void Light::set_linearAttenuation(GLfloat linearAttenuation)
{
  lightLinearAttenuation_->setUniformData( linearAttenuation );
}

ref_ptr<ShaderInput1f>& Light::quadricAttenuation()
{
  return lightQuadricAttenuation_;
}
void Light::set_quadricAttenuation(float quadricAttenuation)
{
  lightQuadricAttenuation_->setUniformData( quadricAttenuation );
}

ref_ptr<ShaderInput3f>& Light::spotDirection()
{
  return lightSpotDirection_;
}
void Light::set_spotDirection(const Vec3f &spotDirection)
{
  lightSpotDirection_->setUniformData( spotDirection );
}

ref_ptr<ShaderInput1f>& Light::spotExponent()
{
  return lightSpotExponent_;
}
void Light::set_spotExponent(GLfloat spotExponent)
{
  lightSpotExponent_->setUniformData( spotExponent );
}

ref_ptr<ShaderInput1f>& Light::innerConeAngle()
{
  return lightInnerConeAngle_;
}
void Light::set_innerConeAngle(GLfloat v)
{
  LightType oldType = getLightType();
  lightInnerConeAngle_->setUniformData( cos( 2.0f*M_PI*v/360.0f ) );
  updateType(oldType);
}

ref_ptr<ShaderInput1f>& Light::outerConeAngle()
{
  return lightOuterConeAngle_;
}
void Light::set_outerConeAngle(GLfloat v)
{
  lightOuterConeAngle_->setUniformData( cos( 2.0f*M_PI*v/360.0f ) );
}

Light::LightType Light::getLightType() const
{
  if(lightPosition_->getVertex4f(0).w==0.0)
  {
    return Light::DIRECTIONAL;
  }
  else if(lightInnerConeAngle_->getVertex1f(0)+0.001>=M_PI)
  {
    return Light::POINT;
  }
  else
  {
    return Light::SPOT;
  }
}

void Light::configureShader(ShaderConfig *cfg)
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
      light_->position()->getVertex4f(0).w
  ));
}
