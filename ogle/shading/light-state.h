/*
 * light.h
 *
 *  Created on: 28.01.2011
 *      Author: daniel
 */

#ifndef _LIGHT_H_
#define _LIGHT_H_

#include <ogle/states/shader-input-state.h>
#include <ogle/states/model-transformation.h>
#include <ogle/algebra/vector.h>
#include <ogle/meshes/cone.h>
#include <ogle/animations/animation-node.h>

namespace ogle {

/**
 * \brief Provides light related uniforms.
 */
class Light : public ShaderInputState
{
public:
  /**
   * Default constructor.
   */
  Light();

  /**
   * Diffuse light color.
   */
  const ref_ptr<ShaderInput3f>& diffuse() const;
  /**
   * Diffuse light color.
   */
  void set_diffuse(const Vec3f&);

  /**
   * Specular light color.
   */
  const ref_ptr<ShaderInput3f>& specular() const;
  /**
   * Specular light color.
   */
  void set_specular(const Vec3f&);

  /**
   * Sets whether the light is distance attenuated.
   */
  void set_isAttenuated(GLboolean isAttenuated);
  /**
   * @return is the light distance attenuated.
   */
  GLboolean isAttenuated() const;
  /**
   * Constant attenuation factor.
   */
  const ref_ptr<ShaderInput2f>& radius() const;
  void set_innerRadius(GLfloat);
  void set_outerRadius(GLfloat);

protected:
  GLboolean isAttenuated_;

  ref_ptr<ShaderInput3f> lightDiffuse_;
  ref_ptr<ShaderInput3f> lightSpecular_;
  ref_ptr<ShaderInput2f> lightRadius_;
};

/**
 * \brief Light that is infinite far away.
 */
class DirectionalLight : public Light
{
public:
  DirectionalLight();
  /**
   * The light position in the scene.
   */
  const ref_ptr<ShaderInput3f>& direction() const;
  /**
   * The light position in the scene.
   */
  void set_direction(const Vec3f&);
protected:
  ref_ptr<ShaderInput3f> lightDirection_;
};

/**
 * \brief Point lights shine in all directions.
 */
class PointLight : public Light
{
public:
  PointLight();
  /**
   * The light position in the scene.
   */
  const ref_ptr<ShaderInput3f>& position() const;
  /**
   * The light position in the scene.
   */
  void set_position(const Vec3f&);
protected:
  ref_ptr<ShaderInput3f> lightPosition_;
};

/**
 * \brief Spot lights shine in one directions in a cone shape.
 */
class SpotLight : public Light
{
public:
  SpotLight();
  /**
   * The light position in the scene.
   */
  const ref_ptr<ShaderInput3f>& position() const;
  /**
   * The light position in the scene.
   */
  void set_position(const Vec3f&);
  /**
   * Direction of the spot.
   */
  const ref_ptr<ShaderInput3f>& spotDirection() const;
  /**
   * Direction of the spot.
   */
  void set_spotDirection(const Vec3f&);

  const ref_ptr<ShaderInput2f>& coneAngle() const;
  void set_innerConeAngle(GLfloat deg);
  void set_outerConeAngle(GLfloat deg);

  const ref_ptr<ShaderInputMat4>& coneMatrix();

protected:
  ref_ptr<ShaderInput3f> lightPosition_;
  ref_ptr<ShaderInput2f> lightConeAngles_;
  ref_ptr<ShaderInput3f> lightSpotDirection_;
  ref_ptr<ModelTransformation> coneMatrix_;
  GLuint coneMatrixStamp_;

  void updateConeMatrix();
};

/////

/**
 * \brief Animates Light position using an AnimationNode.
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

class PointLightNode : public LightNode
{
public:
  PointLightNode(
      const ref_ptr<PointLight> &light,
      const ref_ptr<AnimationNode> &animNode,
      const Vec3f &untransformedPos);
  virtual void update(GLdouble dt);
protected:
  ref_ptr<PointLight> pointLight_;
};

class DirectionalLightNode : public LightNode
{
public:
  DirectionalLightNode(
      const ref_ptr<DirectionalLight> &light,
      const ref_ptr<AnimationNode> &animNode,
      const Vec3f &untransformedPos);
  virtual void update(GLdouble dt);
protected:
  ref_ptr<DirectionalLight> dirLight_;
};

} // end ogle namespace

#endif /* _LIGHT_H_ */
