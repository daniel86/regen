/*
 * light.h
 *
 *  Created on: 28.01.2011
 *      Author: daniel
 */

#ifndef _LIGHT_H_
#define _LIGHT_H_

#include <regen/states/shader-input-state.h>
#include <regen/states/model-transformation.h>
#include <regen/algebra/vector.h>
#include <regen/meshes/cone.h>
#include <regen/animations/animation-node.h>
#include <regen/animations/animation.h>

namespace regen {
/**
 * \brief A light emitting point in space.
 */
class Light : public ShaderInputState, public Animation
{
public:
  /**
   * \brief defines the light type
   */
  enum Type {
    DIRECTIONAL,//!< directional light
    SPOT,       //!< spot light
    POINT       //!< point light
  };

  /**
   * @param lightType the light type.
   */
  Light(Type lightType);

  /**
   * @return the light type.
   */
  Type lightType() const;

  /**
   * Sets whether the light is distance attenuated.
   */
  void set_isAttenuated(GLboolean isAttenuated);
  /**
   * @return is the light distance attenuated.
   */
  GLboolean isAttenuated() const;

  /**
   * @return the world space light position.
   * @note undefined for directional lights.
   */
  const ref_ptr<ShaderInput3f>& position() const;
  /**
   * @return the light direction.
   * @note undefined for point lights.
   */
  const ref_ptr<ShaderInput3f>& direction() const;

  /**
   * @return diffuse light color.
   */
  const ref_ptr<ShaderInput3f>& diffuse() const;
  /**
   * @return specular light color.
   */
  const ref_ptr<ShaderInput3f>& specular() const;

  /**
   * @return inner and outer light radius.
   */
  const ref_ptr<ShaderInput2f>& radius() const;

  /**
   * @return inner and outer cone angles.
   */
  const ref_ptr<ShaderInput2f>& coneAngle() const;
  /**
   * @param deg inner angle in degree.
   */
  void set_innerConeAngle(GLfloat deg);
  /**
   * @param deg outer angle in degree.
   */
  void set_outerConeAngle(GLfloat deg);

  /**
   * @return cone rotation matrix.
   */
  const ref_ptr<ShaderInputMat4>& coneMatrix();

  // override
  void glAnimate(RenderState *rs, GLdouble dt);

protected:
  const Type lightType_;
  GLboolean isAttenuated_;

  ref_ptr<ShaderInput3f> lightPosition_;
  ref_ptr<ShaderInput3f> lightDirection_;
  ref_ptr<ShaderInput3f> lightDiffuse_;
  ref_ptr<ShaderInput3f> lightSpecular_;
  ref_ptr<ShaderInput2f> lightConeAngles_;
  ref_ptr<ShaderInput2f> lightRadius_;

  ref_ptr<ModelTransformation> coneMatrix_;
  GLuint coneMatrixStamp_;
  void updateConeMatrix();
};

/**
 * \brief Animates Light position using an AnimationNode.
 */
class LightNode : public State
{
public:
  /**
   * @param light a light.
   * @param n a animation node.
   */
  LightNode(
      const ref_ptr<Light> &light,
      const ref_ptr<AnimationNode> &n);
  /**
   * @param dt update light position using the niamtion node.
   */
  void update(GLdouble dt);
protected:
  ref_ptr<Light> light_;
  ref_ptr<AnimationNode> animNode_;
  Vec3f untransformedPos_;
};
} // namespace

#endif /* _LIGHT_H_ */
