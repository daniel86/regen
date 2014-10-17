/*
 * camera.h
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <regen/states/state.h>
#include <regen/utility/ref-ptr.h>
#include <regen/math/matrix.h>
#include <regen/math/frustum.h>
#include <regen/meshes/mesh-state.h>
#include <regen/states/model-transformation.h>
#include <regen/gl-types/shader-input-container.h>

namespace regen {
  /**
   * \brief Camera with perspective projection.
   */
  class Camera : public HasInputState
  {
  public:
    /**
     * @param initializeMatrices if false matrix computation is skipped.
     */
    Camera(GLboolean initializeMatrices=GL_TRUE);

    /**
     * Update frustum and projection matrix.
     * @param aspect the apect ratio.
     * @param fov field of view.
     * @param near distance to near plane.
     * @param far distance to far plane.
     * @param near distance to near plane.
     * @param updateMatrices if false matrix computation is skipped.
     */
    void updateFrustum(
        GLfloat aspect, GLfloat fov, GLfloat near, GLfloat far,
        GLboolean updateMatrices=GL_TRUE);

    /**
     * @return specifies the field of view angle, in degrees, in the y direction.
     */
    const ref_ptr<ShaderInput1f>& fov() const;
    /**
     * @return specifies the aspect ratio that determines the field of view in the x direction.
     */
    const ref_ptr<ShaderInput1f>& aspect() const;

    /**
     * @return specifies the distance from the viewer to the near clipping plane (always positive).
     */
    const ref_ptr<ShaderInput1f>& near() const;
    /**
     * @return specifies the distance from the viewer to the far clipping plane (always positive).
     */
    const ref_ptr<ShaderInput1f>& far() const;

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
     * Computes the 8 points forming the camera frustum.
     */
    void updateFrustumPoints();
    /**
     * @return the 8 points forming this Frustum.
     */
    const Frustum& frustum() const;

    /**
     * @param useAudio true if this camera is the OpenAL audio listener.
     */
    void set_isAudioListener(GLboolean useAudio);
    /**
     * @return true if this camera is the OpenAL audio listener.
     */
    GLboolean isAudioListener() const;
    /**
     * @return true if the sphere intersects with the frustum of this camera.
     */
    virtual GLboolean hasIntersectionWithSphere(const Vec3f &center, GLfloat radius);
    /**
     * @return true if the box intersects with the frustum of this camera.
     */
    virtual GLboolean hasIntersectionWithBox(const Vec3f &center, const Vec3f *points);

    // Override
    virtual void enable(RenderState *rs);

  protected:
    ref_ptr<ShaderInputContainer> inputs_;
    ref_ptr<ShaderInput1f> fov_;
    ref_ptr<ShaderInput1f> aspect_;
    ref_ptr<ShaderInput1f> far_;
    ref_ptr<ShaderInput1f> near_;

    ref_ptr<ShaderInput3f> position_;
    ref_ptr<ShaderInput3f> direction_;
    ref_ptr<ShaderInput3f> vel_;

    ref_ptr<ShaderInputMat4> view_;
    ref_ptr<ShaderInputMat4> viewInv_;
    ref_ptr<ShaderInputMat4> proj_;
    ref_ptr<ShaderInputMat4> projInv_;
    ref_ptr<ShaderInputMat4> viewproj_;
    ref_ptr<ShaderInputMat4> viewprojInv_;

    Frustum frustum_;
    GLboolean isAudioListener_;

    void updateProjection();
    void updateLookAt();
    void updateViewProjection(GLuint i=0u, GLuint j=0u);
  };

  /**
   * A camera with 180° field of view or 360° field of view.
   */
  class OmniDirectionalCamera : public Camera {
  public:
    OmniDirectionalCamera(
        GLboolean hasBackFace=GL_FALSE,
        GLboolean updateMatrices=GL_TRUE);
    // Override
    virtual GLboolean hasIntersectionWithSphere(const Vec3f &center, GLfloat radius);
    virtual GLboolean hasIntersectionWithBox(const Vec3f &center, const Vec3f *points);
  protected:
    GLboolean hasBackFace_;
  };
} // namespace

#endif /* _CAMERA_H_ */
