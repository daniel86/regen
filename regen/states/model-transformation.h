/*
 * model-transformation.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef MODEL_TRANSFORMATION_H_
#define MODEL_TRANSFORMATION_H_

#include <regen/states/shader-input-state.h>
#include <regen/av/audio-source.h>
#include <regen/algebra/quaternion.h>

namespace ogle {
/**
 * \brief matrix that transforms for model space to world space.
 *
 * Usually meshes should be defined at origin and then translated
 * and rotated to the world position.
 */
class ModelTransformation : public ShaderInputState
{
public:
  ModelTransformation();

  /**
   * @param m the model transformation matrix.
   * @param dt time difference to last call in milliseconds.
   */
  void set_modelMat(const Mat4f &m, GLdouble dt);
  /**
   * Creates model matrix using given arguments.
   * @param translation model translation.
   * @param rotation model rotation.
   * @param dt time difference to last call in milliseconds.
   */
  void set_modelMat(
      const Vec3f &translation,
      const Quaternion &rotation,
      GLdouble dt);
  /**
   * Creates model matrix using given arguments.
   * @param translation model translation.
   * @param rotation model rotation.
   * @param scaling model scaling.
   * @param dt time difference to last call in milliseconds.
   */
  void set_modelMat(
      const Vec3f &translation,
      const Quaternion &rotation,
      const Vec3f &scaling, GLdouble dt);
  /**
   * @return the model transformation matrix.
   */
  const ref_ptr<ShaderInputMat4>& modelMat() const;

  /**
   * Add a translation vector.
   * @param translation the mode translation.
   * @param dt time difference to last call in milliseconds.
   */
  void translate(const Vec3f &translation, GLdouble dt);
  /**
   * Set a translation vector.
   * @param translation the mode translation.
   * @param dt time difference to last call in milliseconds.
   */
  void setTranslation(const Vec3f &translation, GLdouble dt);
  /**
   * @return the model translation aka the world position of the model center.
   */
  Vec3f translation() const;

  /**
   * Scales the model matrix by given factors.
   * @param scaling the scale factors.
   * @param dt time difference to last call in milliseconds.
   */
  void scale(const Vec3f &scaling, GLdouble dt);
  /**
   * Rotates the model matrix by given Quaternion.
   * @param rotation the model rotation.
   * @param dt time difference to last call in milliseconds.
   */
  void rotate(const Quaternion &rotation, GLdouble dt);

  /**
   * @param audioSource the audio source attached to the world position
   * of the model.
   */
  void set_audioSource(const ref_ptr<AudioSource> &audioSource);
  /**
   * @return the audio source attached to the world position
   * of the model.
   */
  GLboolean isAudioSource() const;

protected:
  ref_ptr<ShaderInputMat4> modelMat_;
  ref_ptr<ShaderInput3f> velocity_;
  Mat4f *lastModelMat_;

  ref_ptr<AudioSource> audioSource_;

  Vec3f lastPosition_;

  void updateVelocity(GLdouble);
  void updateAudioSource();
};
} // namespace

#endif /* MODEL_TRANSFORMATION_H_ */
