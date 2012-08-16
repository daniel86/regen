/*
 * light.h
 *
 *  Created on: 28.01.2011
 *      Author: daniel
 */

#ifndef _LIGHT_H_
#define _LIGHT_H_

#include <ogle/states/state.h>
#include <ogle/gl-types/shader-input.h>
#include <ogle/algebra/vector.h>

/**
 * Provides light related uniforms.
 */
class Light : public State
{
public:
  /**
   * Defines the type of light.
   */
  enum LightType{ DIRECTIONAL, POINT,  SPOT };

  /**
   * Default constructor.
   */
  Light();

  /**
   * The light position in the scene.
   */
  const Vec4f& position() const;
  /**
   * The light position in the scene.
   */
  void set_position(const Vec4f&);

  /**
   * Diffuse light color.
   */
  const Vec4f& diffuse() const;
  /**
   * Diffuse light color.
   */
  void set_diffuse(const Vec4f&);

  /**
   * Ambient light color.
   */
  const Vec4f& ambient() const;
  /**
   * Ambient light color.
   */
  void set_ambient(const Vec4f&);

  /**
   * Specular light color.
   */
  const Vec4f& specular() const;
  /**
   * Specular light color.
   */
  void set_specular(const Vec4f&);

  /**
   * Constant attenuation factor.
   */
  GLfloat constantAttenuation() const;
  /**
   * Constant attenuation factor.
   */
  void set_constantAttenuation(GLfloat);

  /**
   * Linear attenuation factor.
   */
  GLfloat linearAttenuation() const;
  /**
   * Linear attenuation factor.
   */
  void set_linearAttenuation(GLfloat);

  /**
   * Quadric attenuation factor.
   */
  GLfloat quadricAttenuation() const;
  /**
   * Quadric attenuation factor.
   */
  void set_quadricAttenuation(GLfloat);

  /**
   * Direction of the spot.
   */
  const Vec3f& spotDirection() const;
  /**
   * Direction of the spot.
   */
  void set_spotDirection(const Vec3f&);

  /**
   * Exponent for spotlights.
   */
  GLfloat spotExponent() const;
  /**
   * Exponent for spotlights.
   */
  void set_spotExponent(GLfloat);

  /**
   */
  GLfloat innerConeAngle() const;
  /**
   */
  void set_innerConeAngle(GLfloat);

  /**
   */
  GLfloat outerConeAngle() const;
  /**
   */
  void set_outerConeAngle(GLfloat);

  /**
   * @return the light type.
   */
  LightType getLightType() const;

  virtual void configureShader(ShaderConfiguration *cfg);

  virtual string name();

protected:
  ref_ptr<ShaderInput4f> lightPositionUniform_;
  ref_ptr<ShaderInput4f> lightAmbientUniform_;
  ref_ptr<ShaderInput4f> lightDiffuseUniform_;
  ref_ptr<ShaderInput4f> lightSpecularUniform_;
  ref_ptr<ShaderInput1f> lightInnerConeAngleUniform_;
  ref_ptr<ShaderInput1f> lightOuterConeAngleUniform_;
  ref_ptr<ShaderInput3f> lightSpotDirectionUniform_;
  ref_ptr<ShaderInput1f> lightSpotExponentUniform_;
  ref_ptr<ShaderInput1f> lightConstantAttenuationUniform_;
  ref_ptr<ShaderInput1f> lightLinearAttenuationUniform_;
  ref_ptr<ShaderInput1f> lightQuadricAttenuationUniform_;

  string getUniformName(const string &uni);
  void updateType(LightType oldType);
};

/////

#include <ogle/animations/animation-node.h>

/**
 * Animates light position using an AnimationNode.
 */
class LightNode : public State
{
public:
  LightNode(
      const ref_ptr<Light> &light,
      const ref_ptr<AnimationNode> &animNode,
      const Vec3f &untransformedPos);

  virtual void update(GLdouble dt);
  virtual string name();
protected:
  ref_ptr<Light> light_;
  ref_ptr<AnimationNode> animNode_;
  Vec3f untransformedPos_;
};

#endif /* _LIGHT_H_ */
