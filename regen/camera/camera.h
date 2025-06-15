/*
 * camera.h
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#ifndef REGEN_CAMERA_H
#define REGEN_CAMERA_H

#include <regen/states/state.h>
#include <regen/utility/ref-ptr.h>
#include <regen/math/matrix.h>
#include <regen/shapes/frustum.h>
#include <regen/meshes/mesh-state.h>
#include <regen/states/model-transformation.h>
#include <regen/gl-types/input-container.h>
#include "regen/gl-types/ubo.h"
#include "regen/meshes/lod/lod-level.h"

namespace regen {
	/**
	 * \brief Camera with projection and view matrix.
	 */
	class Camera : public HasInputState {
	public:
		static constexpr const char *TYPE_NAME = "Camera";

		/**
		 * Default constructor.
		 * @param numLayer the number of layers.
		 */
		explicit Camera(unsigned int numLayer);

		static ref_ptr<Camera> load(LoadingContext &ctx, scene::SceneInputNode &input);

		static ref_ptr<Camera> createCamera(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * @return the stamp when the camera was last updated.
		 */
		auto stamp() const { return camStamp_; }

		/**
		 * Increment the camera stamp.
		 * @return the new stamp.
		 */
		auto nextStamp() { return ++camStamp_; }

		/**
		 * @return the number of layers.
		 */
		auto numLayer() const { return numLayer_; }

		/**
		 * @return true if this camera is an omnidirectional camera.
		 */
		auto isOmni() const { return isOmni_; }

		/**
		 * @return true if this camera is an orthographic camera.
		 */
		auto isOrtho() const { return isOrtho_; }

		/**
		 * @return true if this camera is a perspective camera.
		 */
		auto isPerspective() const { return !isOrtho_; }

		/**
		 * Update frustum projection and projection matrix.
		 * @param aspect the aspect ratio.
		 * @param fov field of view.
		 * @param near distance to near plane.
		 * @param far distance to far plane.
		 */
		void setPerspective(float aspect, float fov, float near, float far);

		/**
		 * Update frustum projection and projection matrix.
		 * @param aspect the aspect ratio.
		 * @param fov field of view.
		 * @param near distance to near plane.
		 * @param far distance to far plane.
		 * @param layer the layer index.
		 */
		void setPerspective(float aspect, float fov, float near, float far, unsigned int layer);

		void setPerspective(const Vec4f &params);

		/**
		 * Update frustum projection and projection matrix.
		 * @param left the left vertical clipping plane.
		 * @param right the right vertical clipping plane.
		 * @param bottom the bottom horizontal clipping plane.
		 * @param top the top horizontal clipping plane.
		 * @param near distance to near plane.
		 * @param far distance to far plane.
		 */
		void setOrtho(float left, float right, float bottom, float top, float near, float far);

		/**
		 * Update frustum projection and projection matrix.
		 * @param left the left vertical clipping plane.
		 * @param right the right vertical clipping plane.
		 * @param bottom the bottom horizontal clipping plane.
		 * @param top the top horizontal clipping plane.
		 * @param near distance to near plane.
		 * @param far distance to far plane.
		 * @param layer the layer index.
		 */
		void setOrtho(float left, float right, float bottom, float top, float near, float far, unsigned int layer);

		/**
		 * @return the camera uniform block.
		 */
		auto &cameraBlock() const { return cameraBlock_; }

		/**
		 * @return the projection parameters: near, far, aspect, fov.
		 */
		auto &projParams() const { return projParams_; }

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
		auto &viewProjection() const { return viewProj_; }

		/**
		 * Transforms screen-space to world-space.
		 * @return the inverse view-projection matrix.
		 */
		auto &viewProjectionInverse() const { return viewProjInv_; }

		/**
		 * @return the 8 points forming this Frustum.
		 */
		auto &frustum() const { return frustum_; }

		/**
		 * @return the 8 points forming this Frustum.
		 */
		auto &frustum() { return frustum_; }

		/**
		 * @param useAudio true if this camera is the OpenAL audio listener.
		 */
		void set_isAudioListener(GLboolean useAudio);

		/**
		 * @return true if this camera is the OpenAL audio listener.
		 */
		auto isAudioListener() const { return isAudioListener_; }

		/**
		 * Recompute the camera parameters.
		 * @return true if the camera was updated.
		 */
		virtual bool updateCamera();

		/**
		 * @return true if the sphere intersects with the frustum of this camera.
		 */
		bool hasIntersectionWithSphere(const Vec3f &center, GLfloat radius) const;

		/**
		 * @return true if the box intersects with the frustum of this camera.
		 */
		bool hasIntersectionWithBox(const Vec3f &center, const Vec3f *points) const;

		/**
		 * @return true if the sphere intersects with the frustum of this camera.
		 */
		bool hasFrustumIntersection(const Vec3f &center, GLfloat radius) const;

		/**
		 * @return true if the box intersects with the frustum of this camera.
		 */
		bool hasFrustumIntersection(const Vec3f &center, const Vec3f *points) const;

		/**
		 * @return true if the sphere intersects with the bounding sphere of this camera.
		 */
		bool hasSphereIntersection(const Vec3f &center, GLfloat radius) const;

		/**
		 * @return true if the box intersects with the bounding sphere of this camera.
		 */
		bool hasSphereIntersection(const Vec3f &center, const Vec3f *points) const;

		/**
		 * @return true if the sphere intersects with the half bounding sphere of this camera.
		 */
		bool hasHalfSphereIntersection(const Vec3f &center, GLfloat radius) const;

		/**
		 * @return true if the box intersects with the half bounding sphere of this camera.
		 */
		bool hasHalfSphereIntersection(const Vec3f &center, const Vec3f *points) const;

		/**
		 * Attach the camera to a transform, updating the camera position and direction.
		 * @param attachedTransform the transform to attach to.
		 */
		void attachToTransform(const ref_ptr<ShaderInputMat4> &attachedTransform);

		/**
		 * Attach the camera to a position, updating the camera position.
		 * @param attachedPosition the position to attach to.
		 */
		void attachToPosition(const ref_ptr<ShaderInput4f> &attachedPosition);

		/**
		 * Attach the camera to a position, updating the camera position.
		 * @param attachedTransform the transform to attach to.
		 */
		void attachToPosition(const ref_ptr<ShaderInputMat4> &attachedTransform);

		/**
		 * Update the camera pose based on the attached transform, if any.
		 */
		void updatePose();

		/**
		 * @return true is the camera has a fixed LOD quality.
		 */
		bool hasFixedLOD() const { return hasFixedLOD_; }

		/**
		 * @return the fixed LOD quality.
		 */
		LODQuality fixedLODQuality() const { return fixedLODQuality_; }

		/**
		 * Set the fixed LOD quality.
		 * @param quality the LOD quality to set.
		 */
		void setFixedLOD(LODQuality quality) {
			hasFixedLOD_ = true;
			fixedLODQuality_ = quality;
		}

		virtual void updateViewProjection1();

	protected:
		unsigned int numLayer_ = 1;
		bool isOmni_ = false;
		bool isOrtho_ = false;
		bool isAudioListener_ = false;
		unsigned int camStamp_ = 0u;

		bool hasFixedLOD_ = false;
		LODQuality fixedLODQuality_ = LODQuality::LOW;

		std::vector<Frustum> frustum_;

		ref_ptr<UBO> cameraBlock_;
		//ref_ptr<ShaderInput1f> fov_;
		//ref_ptr<ShaderInput1f> aspect_;
		//ref_ptr<ShaderInput1f> far_;
		//ref_ptr<ShaderInput1f> near_;
		ref_ptr<ShaderInput4f> projParams_;

		ref_ptr<ShaderInput4f> position_;
		ref_ptr<ShaderInput4f> direction_;
		ref_ptr<ShaderInput4f> vel_;

		ref_ptr<ShaderInputMat4> view_;
		ref_ptr<ShaderInputMat4> viewInv_;
		ref_ptr<ShaderInputMat4> proj_;
		ref_ptr<ShaderInputMat4> projInv_;
		ref_ptr<ShaderInputMat4> viewProj_;
		ref_ptr<ShaderInputMat4> viewProjInv_;

		ref_ptr<Animation> attachedMotion_;
		ref_ptr<ShaderInputMat4> attachedTransform_;
		ref_ptr<ShaderInput4f> attachedPosition_;
		bool isAttachedToPosition_ = false;
		ref_ptr<Animation> cameraMotion_;

		virtual bool updateView();

		virtual void updateViewProjection(unsigned int projectionIndex, unsigned int viewIndex);

	private:
		unsigned int projectionStamp_ = 0u;
		unsigned int posStamp_ = 0u;
		unsigned int dirStamp_ = 0u;
		unsigned int poseStamp_ = 0u;
	};

	class ProjectionUpdater : public EventHandler {
	public:
		ProjectionUpdater(const ref_ptr<Camera> &cam,
						  const ref_ptr<ShaderInput2i> &windowViewport);

		void call(EventObject *, EventData *) override;

	protected:
		ref_ptr<Camera> cam_;
		ref_ptr<ShaderInput2i> windowViewport_;
	};

	struct ProjectionParams {
		float near = 0.1f; // near plane distance
		float far = 100.0f; // far plane distance
		float aspect = 1.0f; // aspect ratio
		float fov = 60.0f; // field of view in degrees
	};
} // namespace

#endif /* REGEN_CAMERA_H */
