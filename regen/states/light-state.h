/*
 * light.h
 *
 *  Created on: 28.01.2011
 *      Author: daniel
 */

#ifndef _LIGHT_H_
#define _LIGHT_H_

#include <regen/gl-types/shader-input-container.h>
#include <regen/states/model-transformation.h>
#include <regen/states/camera.h>
#include <regen/math/vector.h>
#include <regen/meshes/cone.h>
#include <regen/animations/animation-node.h>
#include <regen/animations/animation.h>

namespace regen {
  /**
   * \brief A light emitting point in space.
   */
  class Light : public State, public Animation, public HasInput
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

  ostream& operator<<(ostream &out, const Light::Type &v);
  istream& operator>>(istream &in, Light::Type &v);

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

  /**
   * \brief Configures filtering.
   */
  enum ShadowFilterMode {
    SHADOW_FILTERING_NONE,        //!< No special filtering
    SHADOW_FILTERING_PCF_GAUSSIAN,//!< PCF filtering using Gauss kernel
    SHADOW_FILTERING_VSM          //!< VSM filtering
  };
  ostream& operator<<(ostream &out, const ShadowFilterMode &mode);
  istream& operator>>(istream &in, ShadowFilterMode &mode);

  /**
   * A Camera from Light's point of view.
   * To cover the light multiple views may be used.
   */
  class LightCamera : public Camera {
  public:
    /**
     * Default constructor.
     * @param light the Light instance.
     * @param userCamera the user camera, maybe used to optimize precision.
     * @param extends near and far plane.
     * @param numLayer number of shadow map layer.
     * @param splitWeight split weight for splitting the view frustum of the user camera.
     */
    LightCamera(const ref_ptr<Light> &light,
                const ref_ptr<Camera> &userCamera,
                Vec2f extends=Vec2f(0.1f,200.0f),
                GLuint numLayer=1, GLdouble splitWeight=0.9);

    /**
     * @return A matrix used to transform world space points to
     *          texture coordinates for shadow mapping.
     */
    const ref_ptr<ShaderInputMat4>& lightMatrix() const;
    /**
     * @return Light far planes.
     */
    const ref_ptr<ShaderInput1f>& lightFar() const;
    /**
     * @return Light near planes.
     */
    const ref_ptr<ShaderInput1f>& lightNear() const;

    /**
     * Discard specified cube faces.
     */
    void set_isCubeFaceVisible(GLenum face, GLboolean visible);

    // Override
    void enable(RenderState *rs);

  protected:
    ref_ptr<Light> light_;
    ref_ptr<Camera> userCamera_;
    GLuint numLayer_;
    GLdouble splitWeight_;

    ref_ptr<ShaderInput1f> lightFar_;
    ref_ptr<ShaderInput1f> lightNear_;
    ref_ptr<ShaderInputMat4> lightMatrix_;
    vector<Frustum*> shadowFrusta_;

    GLuint lightPosStamp_;
    GLuint lightDirStamp_;
    GLuint lightRadiusStamp_;
    GLuint projectionStamp_;

    GLboolean isCubeFaceVisible_[6];

    void (LightCamera::*update_)();
    void updateDirectional();
    void updatePoint();
    void updateSpot();
  };
} // namespace

#endif /* _LIGHT_H_ */
