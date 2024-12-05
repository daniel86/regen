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
	class Camera : public HasInputState {
	public:
		/**
		 * @param initializeMatrices if false matrix computation is skipped.
		 */
		explicit Camera(GLboolean initializeMatrices = GL_TRUE);

		/**
		 * @return the stamp when the camera was last updated.
		 */
		GLuint stamp() const { return camStamp_; }

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
				GLboolean updateMatrices = GL_TRUE);

		/**
		 * @return specifies the field of view angle, in degrees, in the y direction.
		 */
		auto &fov() const { return fov_; }

		/**
		 * @return specifies the aspect ratio that determines the field of view in the x direction.
		 */
		auto &aspect() const { return aspect_; }

		/**
		 * @return specifies the distance from the viewer to the near clipping plane (always positive).
		 */
		auto &near() const { return near_; }

		/**
		 * @return specifies the distance from the viewer to the far clipping plane (always positive).
		 */
		auto &far() const { return far_; }

		/**
		 * @return the camera position.
		 */
		auto &position() const { return position_; }

		/**
		 * @return the camera direction.
		 */
		auto &direction() const { return direction_; }

		/**
		 * @return the camera velocity.
		 */
		auto &velocity() const { return vel_; }

		/**
		 * Transforms world-space to view-space.
		 * @return the view matrix.
		 */
		auto &view() const { return view_; }

		/**
		 * Transforms view-space to world-space.
		 * @return the inverse view matrix.
		 */
		auto &viewInverse() const { return viewInv_; }

		/**
		 * Transforms view-space to screen-space.
		 * @return the projection matrix.
		 */
		auto &projection() const { return proj_; }

		/**
		 * Transforms screen-space to view-space.
		 * @return the inverse projection matrix.
		 */
		auto &projectionInverse() const { return projInv_; }

		/**
		 * Transforms world-space to screen-space.
		 * @return the view-projection matrix.
		 */
		auto &viewProjection() const { return viewproj_; }

		/**
		 * Transforms screen-space to world-space.
		 * @return the inverse view-projection matrix.
		 */
		auto &viewProjectionInverse() const { return viewprojInv_; }

		/**
		 * @return the 8 points forming this Frustum.
		 */
		auto &frustum() const { return frustum_; }

		/**
		 * @param useAudio true if this camera is the OpenAL audio listener.
		 */
		void set_isAudioListener(GLboolean useAudio);

		/**
		 * @return true if this camera is the OpenAL audio listener.
		 */
		auto isAudioListener() const { return isAudioListener_; }

		/**
		 * @return true if the sphere intersects with the frustum of this camera.
		 */
		virtual GLboolean hasIntersectionWithSphere(const Vec3f &center, GLfloat radius);

		/**
		 * @return true if the box intersects with the frustum of this camera.
		 */
		virtual GLboolean hasIntersectionWithBox(const Vec3f &center, const Vec3f *points);

		/**
		 * @return true if the sphere intersects with the frustum of this camera.
		 */
		GLboolean hasSpotIntersectionWithSphere(const Vec3f &center, GLfloat radius);

		/**
		 * @return true if the box intersects with the frustum of this camera.
		 */
		GLboolean hasSpotIntersectionWithBox(const Vec3f &center, const Vec3f *points);

		// Override
		void enable(RenderState *rs) override;

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
		GLuint camStamp_;
		GLuint posStamp_;
		GLuint dirStamp_;

		void updateProjection();

		void updateLookAt();

		void updateViewProjection(GLuint i = 0u, GLuint j = 0u);
	};

	/**
	 * A camera with 180° field of view or 360° field of view.
	 */
	class OmniDirectionalCamera : public Camera {
	public:
		explicit OmniDirectionalCamera(
				GLboolean hasBackFace = GL_FALSE,
				GLboolean updateMatrices = GL_TRUE);

		GLboolean hasOmniIntersectionWithSphere(const Vec3f &center, GLfloat radius);

		GLboolean hasOmniIntersectionWithBox(const Vec3f &center, const Vec3f *points);

		// Override
		GLboolean hasIntersectionWithSphere(const Vec3f &center, GLfloat radius) override;

		GLboolean hasIntersectionWithBox(const Vec3f &center, const Vec3f *points) override;

	protected:
		GLboolean hasBackFace_;
	};
} // namespace

#endif /* _CAMERA_H_ */
