/*
 * model-transformation-state.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef MODEL_TRANSFORMATION_STATE_H_
#define MODEL_TRANSFORMATION_STATE_H_

#include <ogle/states/shader-input-state.h>
#include <ogle/av/audio-source.h>
#include <ogle/algebra/quaternion.h>

namespace ogle {

/**
 * Provides a model transformation matrix that
 * is applied to the vertex coordinates before the view
 * matrix is applied.
 */
class ModelTransformation : public ShaderInputState
{
public:
  ModelTransformation();
  /**
   * Translate model matrix.
   * dt in milliseconds for velocity calculation.
   */
  void translate(const Vec3f &translation, GLdouble dt);
  /**
   * Set translation components of model matrix.
   * dt in milliseconds for velocity calculation.
   */
  void setTranslation(const Vec3f &translation, GLdouble dt);
  Vec3f translation() const;

  /**
   * Scale model matrix.
   * dt in milliseconds for velocity calculation.
   */
  void scale(const Vec3f &scaling, GLdouble dt);

  /**
   * Rotate model matrix.
   * dt in milliseconds for velocity calculation.
   */
  void rotate(const Quaternion &rotation, GLdouble dt);

  /**
   * Sets the model matrix.
   * dt in milliseconds for velocity calculation.
   */
  void set_modelMat(const Mat4f &m, GLdouble dt);

  /**
   * Sets the model matrix.
   * dt in milliseconds for velocity calculation.
   */
  void set_modelMat(const Vec3f &translation, const Quaternion &rotation, GLdouble dt);

  /**
   * Sets the model matrix.
   * dt in milliseconds for velocity calculation.
   */
  void set_modelMat(
      const Vec3f &translation,
      const Quaternion &rotation,
      const Vec3f &scaling, GLdouble dt);

  /**
   * The model matrix used to transform from object space to world space.
   */
  const ref_ptr<ShaderInputMat4>& modelMat() const;
  /**
   * The audio source associated to the world position
   * of the transformation.
   */
  void set_audioSource(const ref_ptr<AudioSource> &audioSource);
  /**
   * The audio source associated to the world position
   * of the transformation.
   */
  GLboolean isAudioSource() const;

protected:
  // model matrix
  ref_ptr<ShaderInputMat4> modelMat_;
  ref_ptr<ShaderInput3f> velocity_;
  Mat4f *lastModelMat_;

  ref_ptr<AudioSource> audioSource_;

  Vec3f lastPosition_;

  void updateVelocity(GLdouble);
  void updateAudioSource();
};

} // end ogle namespace

#endif /* MODEL_TRANSFORMATION_STATE_H_ */
