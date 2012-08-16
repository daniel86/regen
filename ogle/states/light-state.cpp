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
  // TODO: set uniforms constant by default
#define NAME(x) getUniformName(x)
  lightPositionUniform_ = ref_ptr<ShaderInput4f>::manage(
      new ShaderInput4f(NAME("lightPosition")));
  lightPositionUniform_->setUniformData(Vec4f(4.0, 4.0, 4.0, 0.0));
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightPositionUniform_) );

  lightAmbientUniform_ = ref_ptr<ShaderInput4f>::manage(
      new ShaderInput4f(NAME("lightAmbient")));
  lightAmbientUniform_->setUniformData(Vec4f(0.2, 0.2, 0.2, 1.0));
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightAmbientUniform_) );

  lightDiffuseUniform_ = ref_ptr<ShaderInput4f>::manage(
      new ShaderInput4f(NAME("lightDiffuse")));
  lightDiffuseUniform_->setUniformData(Vec4f(1.0));
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightDiffuseUniform_) );

  lightSpecularUniform_ = ref_ptr<ShaderInput4f>::manage(
      new ShaderInput4f(NAME("lightSpecular")));
  lightSpecularUniform_->setUniformData(Vec4f(1.0));
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightSpecularUniform_) );

  lightInnerConeAngleUniform_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f(NAME("lightInnerConeAngle")));
  lightInnerConeAngleUniform_->setUniformData(cos( 0.4*M_PI));
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightInnerConeAngleUniform_) );

  lightOuterConeAngleUniform_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f(NAME("lightOuterConeAngle")));
  lightOuterConeAngleUniform_->setUniformData(cos( 0.6*M_PI));
  joinShaderInput( ref_ptr<ShaderInput>::cast(lightOuterConeAngleUniform_) );

  lightSpotDirectionUniform_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f(NAME("lightSpotDirection")));
  lightSpotDirectionUniform_->setUniformData(Vec3f(1.0));

  lightSpotExponentUniform_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f(NAME("lightSpotExponent")));
  lightSpotExponentUniform_->setUniformData(0.0f);

  lightConstantAttenuationUniform_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f(NAME("lightConstantAttenuation")));
  lightConstantAttenuationUniform_->setUniformData(0.0002f);

  lightLinearAttenuationUniform_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f(NAME("lightLinearAttenuation")));
  lightLinearAttenuationUniform_->setUniformData(0.002f);

  lightQuadricAttenuationUniform_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f(NAME("lightQuadricAttenuation")));
  lightQuadricAttenuationUniform_->setUniformData(0.002f);

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
    break;
  case SPOT:
    disjoinShaderInput( ref_ptr<ShaderInput>::cast(lightSpotDirectionUniform_) );
    disjoinShaderInput( ref_ptr<ShaderInput>::cast(lightSpotExponentUniform_) );
    // fall through
  case POINT:
    disjoinShaderInput( ref_ptr<ShaderInput>::cast(lightConstantAttenuationUniform_) );
    disjoinShaderInput( ref_ptr<ShaderInput>::cast(lightLinearAttenuationUniform_) );
    disjoinShaderInput( ref_ptr<ShaderInput>::cast(lightQuadricAttenuationUniform_) );
    break;
  }
  switch(newType) {
  case DIRECTIONAL:
    break;
  case SPOT:
    joinShaderInput( ref_ptr<ShaderInput>::cast(lightSpotDirectionUniform_) );
    joinShaderInput( ref_ptr<ShaderInput>::cast(lightSpotExponentUniform_) );
    // fall through
  case POINT:
    joinShaderInput( ref_ptr<ShaderInput>::cast(lightConstantAttenuationUniform_) );
    joinShaderInput( ref_ptr<ShaderInput>::cast(lightLinearAttenuationUniform_) );
    joinShaderInput( ref_ptr<ShaderInput>::cast(lightQuadricAttenuationUniform_) );
    break;
  }
}

const Vec4f& Light::position() const
{
  return lightPositionUniform_->getVertex4f(0);
}
void Light::set_position(const Vec4f &position)
{
  LightType oldType = getLightType();
  lightPositionUniform_->setUniformData( position );
  updateType(oldType);
}

const Vec4f& Light::diffuse() const
{
  return lightDiffuseUniform_->getVertex4f(0);
}
void Light::set_diffuse(const Vec4f &diffuse)
{
  lightDiffuseUniform_->setUniformData( diffuse );
}

const Vec4f& Light::ambient() const
{
  return lightAmbientUniform_->getVertex4f(0);
}
void Light::set_ambient(const Vec4f &ambient)
{
  lightAmbientUniform_->setUniformData( ambient );
}

const Vec4f& Light::specular() const
{
  return lightSpecularUniform_->getVertex4f(0);
}
void Light::set_specular(const Vec4f &specular)
{
  lightSpecularUniform_->setUniformData( specular );
}

GLfloat Light::constantAttenuation() const
{
  return lightConstantAttenuationUniform_->getVertex1f(0);
}
void Light::set_constantAttenuation(GLfloat constantAttenuation)
{
  lightConstantAttenuationUniform_->setUniformData( constantAttenuation );
}

GLfloat Light::linearAttenuation() const
{
  return lightLinearAttenuationUniform_->getVertex1f(0);
}
void Light::set_linearAttenuation(GLfloat linearAttenuation)
{
  lightLinearAttenuationUniform_->setUniformData( linearAttenuation );
}

GLfloat Light::quadricAttenuation() const
{
  return lightQuadricAttenuationUniform_->getVertex1f(0);
}
void Light::set_quadricAttenuation(float quadricAttenuation)
{
  lightQuadricAttenuationUniform_->setUniformData( quadricAttenuation );
}

const Vec3f& Light::spotDirection() const
{
  return lightSpotDirectionUniform_->getVertex3f(0);
}
void Light::set_spotDirection(const Vec3f &spotDirection)
{
  lightSpotDirectionUniform_->setUniformData( spotDirection );
}

GLfloat Light::spotExponent() const
{
  return lightSpotExponentUniform_->getVertex1f(0);
}
void Light::set_spotExponent(GLfloat spotExponent)
{
  lightSpotExponentUniform_->setUniformData( spotExponent );
}

GLfloat Light::innerConeAngle() const
{
  return lightInnerConeAngleUniform_->getVertex1f(0);
}
void Light::set_innerConeAngle(GLfloat v)
{
  LightType oldType = getLightType();
  lightInnerConeAngleUniform_->setUniformData( cos( 2.0f*M_PI*v/360.0f ) );
  updateType(oldType);
}

GLfloat Light::outerConeAngle() const
{
  return lightOuterConeAngleUniform_->getVertex1f(0);
}
void Light::set_outerConeAngle(GLfloat v)
{
  lightOuterConeAngleUniform_->setUniformData( cos( 2.0f*M_PI*v/360.0f ) );
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
