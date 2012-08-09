/*
 * light.h
 *
 *  Created on: 28.01.2011
 *      Author: daniel
 */

#ifndef _LIGHT_H_
#define _LIGHT_H_

#include <ogle/states/state.h>
#include <ogle/gl-types/uniform.h>
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
  float constantAttenuation() const;
  /**
   * Constant attenuation factor.
   */
  void set_constantAttenuation(float);

  /**
   * Linear attenuation factor.
   */
  float linearAttenuation() const;
  /**
   * Linear attenuation factor.
   */
  void set_linearAttenuation(float);

  /**
   * Quadric attenuation factor.
   */
  float quadricAttenuation() const;
  /**
   * Quadric attenuation factor.
   */
  void set_quadricAttenuation(float);

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
  float spotExponent() const;
  /**
   * Exponent for spotlights.
   */
  void set_spotExponent(float);

  /**
   */
  float innerConeAngle() const;
  /**
   */
  void set_innerConeAngle(float);

  /**
   */
  float outerConeAngle() const;
  /**
   */
  void set_outerConeAngle(float);

  /**
   * @return the light type.
   */
  LightType getLightType() const;

  virtual void configureShader(ShaderConfiguration *cfg);

protected:
  ref_ptr<UniformVec4> lightPositionUniform_;
  ref_ptr<UniformVec4> lightAmbientUniform_;
  ref_ptr<UniformVec4> lightDiffuseUniform_;
  ref_ptr<UniformVec4> lightSpecularUniform_;
  ref_ptr<UniformFloat> lightInnerConeAngleUniform_;
  ref_ptr<UniformFloat> lightOuterConeAngleUniform_;
  ref_ptr<UniformVec3> lightSpotDirectionUniform_;
  ref_ptr<UniformFloat> lightSpotExponentUniform_;
  ref_ptr<UniformFloat> lightConstantAttenuationUniform_;
  ref_ptr<UniformFloat> lightLinearAttenuationUniform_;
  ref_ptr<UniformFloat> lightQuadricAttenuationUniform_;

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
protected:
  ref_ptr<Light> light_;
  ref_ptr<AnimationNode> animNode_;
  Vec3f untransformedPos_;
};

#endif /* _LIGHT_H_ */
