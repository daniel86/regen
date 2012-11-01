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

  long id();

  /**
   * The light position in the scene.
   */
  ref_ptr<ShaderInput4f>& position();
  /**
   * The light position in the scene.
   */
  void set_position(const Vec4f&);

  /**
   * Diffuse light color.
   */
  ref_ptr<ShaderInput4f>& diffuse();
  /**
   * Diffuse light color.
   */
  void set_diffuse(const Vec4f&);

  /**
   * Ambient light color.
   */
  ref_ptr<ShaderInput4f>& ambient();
  /**
   * Ambient light color.
   */
  void set_ambient(const Vec4f&);

  /**
   * Specular light color.
   */
  ref_ptr<ShaderInput4f>& specular();
  /**
   * Specular light color.
   */
  void set_specular(const Vec4f&);

  /**
   * Constant attenuation factor.
   */
  ref_ptr<ShaderInput1f>& constantAttenuation();
  /**
   * Constant attenuation factor.
   */
  void set_constantAttenuation(GLfloat);

  /**
   * Linear attenuation factor.
   */
  ref_ptr<ShaderInput1f>& linearAttenuation();
  /**
   * Linear attenuation factor.
   */
  void set_linearAttenuation(GLfloat);

  /**
   * Quadric attenuation factor.
   */
  ref_ptr<ShaderInput1f>& quadricAttenuation();
  /**
   * Quadric attenuation factor.
   */
  void set_quadricAttenuation(GLfloat);

  /**
   * Direction of the spot.
   */
  ref_ptr<ShaderInput3f>& spotDirection();
  /**
   * Direction of the spot.
   */
  void set_spotDirection(const Vec3f&);

  /**
   * Exponent for spotlights.
   */
  ref_ptr<ShaderInput1f>& spotExponent();
  /**
   * Exponent for spotlights.
   */
  void set_spotExponent(GLfloat);

  /**
   */
  ref_ptr<ShaderInput1f>& innerConeAngle();
  /**
   */
  void set_innerConeAngle(GLfloat);

  /**
   */
  ref_ptr<ShaderInput1f>& outerConeAngle();
  /**
   */
  void set_outerConeAngle(GLfloat);

  /**
   * @return the light type.
   */
  LightType getLightType() const;

  // override
  virtual void configureShader(ShaderConfig *cfg);

protected:
  static long idCounter_;
  long id_;

  ref_ptr<ShaderInput4f> lightPosition_;
  ref_ptr<ShaderInput4f> lightAmbient_;
  ref_ptr<ShaderInput4f> lightDiffuse_;
  ref_ptr<ShaderInput4f> lightSpecular_;
  ref_ptr<ShaderInput1f> lightInnerConeAngle_;
  ref_ptr<ShaderInput1f> lightOuterConeAngle_;
  ref_ptr<ShaderInput3f> lightSpotDirection_;
  ref_ptr<ShaderInput1f> lightSpotExponent_;
  ref_ptr<ShaderInput1f> lightConstantAttenuation_;
  ref_ptr<ShaderInput1f> lightLinearAttenuation_;
  ref_ptr<ShaderInput1f> lightQuadricAttenuation_;

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
