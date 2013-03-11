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
  typedef enum {
    DIRECTION_UP,
    DIRECTION_RIGHT,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_FRONT,
    DIRECTION_BACK
  }Direction;

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

  const ref_ptr<ShaderInput1f>& fov() const;
  const ref_ptr<ShaderInput1f>& near() const;
  const ref_ptr<ShaderInput1f>& far() const;
  const ref_ptr<ShaderInput1f>& aspect() const;

  const ref_ptr<ShaderInput3f>& position() const;
  const ref_ptr<ShaderInput3f>& direction() const;
  const ref_ptr<ShaderInput3f>& velocity() const;

  const ref_ptr<ShaderInputMat4>& view() const;
  const ref_ptr<ShaderInputMat4>& viewInverse() const;

  const ref_ptr<ShaderInputMat4>& projection() const;
  const ref_ptr<ShaderInputMat4>& projectionInverse() const;

  const ref_ptr<ShaderInputMat4>& viewProjection() const;
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
