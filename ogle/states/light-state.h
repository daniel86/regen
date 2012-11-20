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
   * Default constructor.
   */
  Light();

  long id();
  /**
   * Diffuse light color.
   */
  ref_ptr<ShaderInput3f>& diffuse();
  /**
   * Diffuse light color.
   */
  void set_diffuse(const Vec3f&);
  /**
   * Ambient light color.
   */
  ref_ptr<ShaderInput3f>& ambient();
  /**
   * Ambient light color.
   */
  void set_ambient(const Vec3f&);

  /**
   * Specular light color.
   */
  ref_ptr<ShaderInput3f>& specular();
  /**
   * Specular light color.
   */
  void set_specular(const Vec3f&);

  // override
  virtual void configureShader(ShaderConfig *cfg);

protected:
  static long idCounter_;
  long id_;

  ref_ptr<ShaderInput3f> lightAmbient_;
  ref_ptr<ShaderInput3f> lightDiffuse_;
  ref_ptr<ShaderInput3f> lightSpecular_;
};

class DirectionalLight : public Light
{
public:
  DirectionalLight();
  /**
   * The light position in the scene.
   */
  ref_ptr<ShaderInput3f>& direction();
  /**
   * The light position in the scene.
   */
  void set_direction(const Vec3f&);
protected:
  ref_ptr<ShaderInput3f> lightDirection_;
};

class AttenuatedLight : public Light
{
public:
  AttenuatedLight();
  /**
   * Constant attenuation factor.
   */
  ref_ptr<ShaderInput3f>& attenuation();
  /**
   * Constant attenuation factor.
   */
  void set_constantAttenuation(GLfloat);
  /**
   * Linear attenuation factor.
   */
  void set_linearAttenuation(GLfloat);
  /**
   * Quadric attenuation factor.
   */
  void set_quadricAttenuation(GLfloat);
protected:
  ref_ptr<ShaderInput3f> lightAttenuation_;
};

class PointLight : public AttenuatedLight
{
public:
  PointLight();
  /**
   * The light position in the scene.
   */
  ref_ptr<ShaderInput3f>& position();
  /**
   * The light position in the scene.
   */
  void set_position(const Vec3f&);
protected:
  ref_ptr<ShaderInput3f> lightPosition_;
};

class SpotLight : public AttenuatedLight
{
public:
  SpotLight();
  /**
   * The light position in the scene.
   */
  ref_ptr<ShaderInput3f>& position();
  /**
   * The light position in the scene.
   */
  void set_position(const Vec3f&);
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
  ref_ptr<ShaderInput2f>& coneAngle();
  /**
   */
  void set_innerConeAngle(GLfloat);
  /**
   */
  void set_outerConeAngle(GLfloat);
protected:
  ref_ptr<ShaderInput3f> lightPosition_;
  ref_ptr<ShaderInput2f> lightConeAngle_;
  ref_ptr<ShaderInput3f> lightSpotDirection_;
  ref_ptr<ShaderInput1f> lightSpotExponent_;
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
  virtual void update(GLdouble dt)=0;
protected:
  ref_ptr<Light> light_;
  ref_ptr<AnimationNode> animNode_;
  Vec3f untransformedPos_;
};

class SpotLightNode : public LightNode
{
public:
  SpotLightNode(
      const ref_ptr<SpotLight> &light,
      const ref_ptr<AnimationNode> &animNode,
      const Vec3f &untransformedPos);
  virtual void update(GLdouble dt);
protected:
  ref_ptr<SpotLight> spotLight_;
};

#endif /* _LIGHT_H_ */
