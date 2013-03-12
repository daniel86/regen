/*
 * camera.h
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <ogle/states/shader-input-state.h>
#include <ogle/utility/ref-ptr.h>
#include <ogle/algebra/matrix.h>

namespace ogle {
/**
 * \brief Camera with perspective projection.
 */
class Camera : public ShaderInputState
{
public:
  Camera();

  /**
   * @return Sensitivity of movement.
   */
  GLfloat sensitivity() const;
  /**
   * @param sensitivity Sensitivity of movement.
   */
  void set_sensitivity(GLfloat sensitivity);

  /**
   * @return Speed of movement.
   */
  GLfloat walkSpeed() const;
  /**
   * @param walkSpeed Speed of movement.
   */
  void set_walkSpeed(GLfloat walkSpeed);

  /**
   * @return specifies the field of view angle, in degrees, in the y direction.
   */
  const ref_ptr<ShaderInput1f>& fov() const;
  /**
   * @return specifies the distance from the viewer to the near clipping plane (always positive).
   */
  const ref_ptr<ShaderInput1f>& near() const;
  /**
   * @return specifies the distance from the viewer to the far clipping plane (always positive).
   */
  const ref_ptr<ShaderInput1f>& far() const;
  /**
   * @return specifies the aspect ratio that determines the field of view in the x direction.
   */
  const ref_ptr<ShaderInput1f>& aspect() const;

  /**
   * @return the camera position.
   */
  const ref_ptr<ShaderInput3f>& position() const;
  /**
   * @return the camera direction.
   */
  const ref_ptr<ShaderInput3f>& direction() const;
  /**
   * @return the camera velocity.
   */
  const ref_ptr<ShaderInput3f>& velocity() const;

  /**
   * Transforms world-space to view-space.
   * @return the view matrix.
   */
  const ref_ptr<ShaderInputMat4>& view() const;
  /**
   * Transforms view-space to world-space.
   * @return the inverse view matrix.
   */
  const ref_ptr<ShaderInputMat4>& viewInverse() const;

  /**
   * Transforms view-space to screen-space.
   * @return the projection matrix.
   */
  const ref_ptr<ShaderInputMat4>& projection() const;
  /**
   * Transforms screen-space to view-space.
   * @return the inverse projection matrix.
   */
  const ref_ptr<ShaderInputMat4>& projectionInverse() const;

  /**
   * Transforms world-space to screen-space.
   * @return the view-projection matrix.
   */
  const ref_ptr<ShaderInputMat4>& viewProjection() const;
  /**
   * Transforms screen-space to world-space.
   * @return the inverse view-projection matrix.
   */
  const ref_ptr<ShaderInputMat4>& viewProjectionInverse() const;

  /**
   * @param useAudio true if this camera is the OpenAL audio listener.
   */
  void set_isAudioListener(GLboolean useAudio);
  /**
   * @return true if this camera is the OpenAL audio listener.
   */
  GLboolean isAudioListener() const;

protected:
  GLfloat sensitivity_;
  GLfloat walkSpeed_;

  GLboolean isAudioListener_;

  ref_ptr<ShaderInput3f> position_;
  ref_ptr<ShaderInput3f> direction_;
  ref_ptr<ShaderInput1f> fov_;
  ref_ptr<ShaderInput1f> near_;
  ref_ptr<ShaderInput1f> far_;
  ref_ptr<ShaderInput1f> aspect_;
  ref_ptr<ShaderInput3f> vel_;

  ref_ptr<ShaderInputMat4> view_;
  ref_ptr<ShaderInputMat4> viewInv_;
  ref_ptr<ShaderInputMat4> proj_;
  ref_ptr<ShaderInputMat4> projInv_;
  ref_ptr<ShaderInputMat4> viewproj_;
  ref_ptr<ShaderInputMat4> viewprojInv_;
};
} // end ogle namespace

#endif /* _CAMERA_H_ */
