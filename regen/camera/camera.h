#ifndef REGEN_CAMERA_H
#define REGEN_CAMERA_H

#include <span>
#include <regen/states/state.h>
#include <regen/utility/ref-ptr.h>
#include <regen/math/matrix.h>
#include <regen/shapes/frustum.h>
#include <regen/objects/mesh.h>
#include <regen/states/model-transformation.h>
#include "regen/buffer/ubo.h"
#include "regen/objects/lod/lod-level.h"
#include "regen/scene/screen.h"

namespace regen {
	/**
	 * \brief Projection parameters for a camera.
	 * Contains near and far plane distances, aspect ratio, and field of view.
	 */
	struct ProjectionParams {
		float near = 0.1f; // near plane distance
		float far = 100.0f; // far plane distance
		float aspect = 1.0f; // aspect ratio
		float fov = 60.0f; // field of view in degrees
		ProjectionParams() = default;
		ProjectionParams(float near, float far, float aspect, float fov)
			: near(near), far(far), aspect(aspect), fov(fov) {}

		Vec4f& asVec4() { return *reinterpret_cast<Vec4f*>(&near); }
		const Vec4f& asVec4() const { return *reinterpret_cast<const Vec4f*>(&near); }
	};

	/**
	 * \brief Camera with projection and view matrix.
	 */
	class Camera : public State {
	public:
		static constexpr const char *TYPE_NAME = "Camera";

		/**
		 * Default constructor.
		 * @param numLayer the number of layers.
		 */
		explicit Camera(unsigned int numLayer,
				const BufferUpdateFlags &updateFlags = BufferUpdateFlags::PARTIAL_PER_FRAME);

		~Camera() override = default;

		static ref_ptr<Camera> load(LoadingContext &ctx, scene::SceneInputNode &input);

		static ref_ptr<Camera> createCamera(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * @return the stamp when the camera was last updated.
		 */
		uint32_t stamp() const { return camStamp_; }

		/**
		 * Increment the camera stamp.
		 * @return the new stamp.
		 */
		uint32_t nextStamp() { return ++camStamp_; }

		/**
		 * @return the number of layers.
		 */
		uint32_t numLayer() const { return numLayer_; }

		/**
		 * @return true if this camera is an omnidirectional camera.
		 */
		bool isOmni() const { return isOmni_; }

		/**
		 * @return true if this camera is an orthographic camera.
		 */
		bool isOrtho() const { return isOrtho_; }

		/**
		 * @return true if this camera is a perspective camera.
		 */
		bool isPerspective() const { return !isOrtho_; }

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

		/**
		 * Update frustum projection and projection matrix.
		 * @param params the projection parameters: near, far, aspect, fov.
		 */
		void setPerspective(const ProjectionParams &params);

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
		const ref_ptr<UBO> &cameraBlock() const { return cameraBlock_; }

		/**
		 * Get vector of most recent projection parameters.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @return the projection parameters: near, far, aspect, fov.
		 */
		const std::vector<ProjectionParams> &projParams() const { return projParams_; }

		/**
		 * Get projection parameters for a specific layer.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @param idx the layer index.
		 * @return the projection parameters for the specified layer.
		 */
		const ProjectionParams& projParams(uint32_t idx) const { return projParams_[idx]; }

		/**
		 * Get the shader data for the projection parameters.
		 * Note that due to buffering, the shader data may lack behind the most recent
		 * data in projParams().
		 * @return the shader data for the projection parameters.
		 */
		const ref_ptr<ShaderInput4f>& sh_projParams() const { return sh_projParams_; }

		/**
		 * Set the projection parameters for a specific layer, and increment the stamp
		 * indicating that the projection parameters have changed.
		 * @param idx the layer index.
		 * @param params the projection parameters to set.
		 */
		void setProjParams(uint32_t idx, const ProjectionParams &params) {
			setStamped(projParams_, projStamp_, idx, params);
		}

		/**
		 * Get a vector of the most recent camera positions.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @return the camera positions.
		 */
		const std::vector<Vec4f> &position() const { return position_; }

		/**
		 * Get the camera position for a specific layer.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @param idx the layer index.
		 * @return the camera position for the specified layer.
		 */
		const Vec3f &position(uint32_t idx) const { return position_[idx].xyz(); }

		/**
		 * Get the stamp indicating when the camera position was last updated.
		 * @return the position stamp.
		 */
		uint32_t positionStamp() const { return positionStamp_; }

		/**
		 * Set the camera position for a specific layer, and increment the stamp
		 * indicating that the camera position has changed.
		 * @param idx the layer index.
		 * @param pos the camera position to set.
		 */
		void setPosition(uint32_t idx, const Vec3f &pos) {
			setStamped3(position_, positionStamp_, idx, pos);
		}

		/**
		 * Set the local up vector for the camera.
		 * @param up the local up vector.
		 */
		void setLocalUp(const Vec3f &up) {
			localUp_ = up;
			localUp_.normalize();
		}

		/**
		 * Get a vector of the most recent camera directions.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @return the camera directions.
		 */
		const std::vector<Vec4f> &direction() const { return direction_; }

		/**
		 * Get the camera direction for a specific layer.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @param idx the layer index.
		 * @return the camera direction for the specified layer.
		 */
		const Vec3f &direction(uint32_t idx) const { return direction_[idx].xyz(); }

		/**
		 * Get the stamp indicating when the camera direction was last updated.
		 * @return the direction stamp.
		 */
		uint32_t directionStamp() const { return directionStamp_; }

		/**
		 * Set the camera direction for a specific layer, and increment the stamp
		 * indicating that the camera direction has changed.
		 * @param idx the layer index.
		 * @param dir the camera direction to set.
		 */
		void setDirection(uint32_t idx, const Vec3f &dir) {
			setStamped3(direction_, directionStamp_, idx, dir);
		}

		/**
		 * Get the camera velocity.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @param idx the layer index.
		 * @return the camera velocity for the specified layer.
		 */
		const Vec3f &velocity() const { return vel_.xyz(); }

		/**
		 * Get the stamp indicating when the camera velocity was last updated.
		 * @return the velocity stamp.
		 */
		uint32_t velocityStamp() const { return velStamp_; }

		/**
		 * Get a vector of view matrices used to transform world-space to view-space.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @return the view matrices.
		 */
		const std::span<Mat4f> &view() const { return view_; }

		/**
		 * Get the view matrix for a specific layer.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @param idx the layer index.
		 * @return the view matrix for the specified layer.
		 */
		const Mat4f &view(uint32_t idx) const { return view_[idx]; }

		/**
		 * Get the stamp indicating when the view matrix was last updated.
		 * @return the view stamp.
		 */
		uint32_t viewStamp() const { return viewStamp_; }

		/**
		 * Set the view matrix for a specific layer, and increment the stamp
		 * indicating that the view matrix has changed.
		 * @param idx the layer index.
		 * @param view the view matrix to set.
		 */
		void setView(uint32_t idx, const Mat4f &view) {
			setStamped(view_, viewStamp_, idx, view);
		}

		/**
		 * Get a vector of inverse view matrices used to transform view-space to world-space.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @return the inverse view matrices.
		 */
		const std::span<Mat4f> &viewInverse() const { return viewInv_; }

		/**
		 * Get the inverse view matrix for a specific layer.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @param idx the layer index.
		 * @return the inverse view matrix for the specified layer.
		 */
		const Mat4f &viewInverse(uint32_t idx) const { return viewInv_[idx]; }

		/**
		 * Set the inverse view matrix for a specific layer, and increment the stamp
		 * indicating that the inverse view matrix has changed.
		 * @param idx the layer index.
		 * @param viewInv the inverse view matrix to set.
		 */
		void setViewInverse(uint32_t idx, const Mat4f &viewInv) {
			setStamped(viewInv_, viewStamp_, idx, viewInv);
		}

		/**
		 * Get a vector of projection matrices used to transform world-space to screen-space.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @return the projection matrices.
		 */
		const std::span<Mat4f> &projection() const { return proj_; }

		/**
		 * Get the projection matrix for a specific layer.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @param idx the layer index.
		 * @return the projection matrix for the specified layer.
		 */
		const Mat4f &projection(uint32_t idx) const { return proj_[idx]; }

		/**
		 * Get the stamp indicating when the projection matrix was last updated.
		 * @return the projection stamp.
		 */
		uint32_t projectionStamp() const { return projStamp_; }

		/**
		 * Set the projection matrix for a specific layer, and increment the stamp
		 * indicating that the projection matrix has changed.
		 * @param idx the layer index.
		 * @param proj the projection matrix to set.
		 */
		void setProjection(uint32_t idx, const Mat4f &proj) {
			setStamped(proj_, projStamp_, idx, proj);
		}

		/**
		 * Get a vector of inverse projection matrices used to transform screen-space to world-space.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @return the inverse projection matrices.
		 */
		const std::span<Mat4f> &projectionInverse() const { return projInv_; }

		/**
		 * Get the inverse projection matrix for a specific layer.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @param idx the layer index.
		 * @return the inverse projection matrix for the specified layer.
		 */
		const Mat4f &projectionInverse(uint32_t idx) const { return projInv_[idx]; }

		/**
		 * Set the inverse projection matrix for a specific layer, and increment the stamp
		 * indicating that the inverse projection matrix has changed.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @param idx the layer index.
		 * @param projInv the inverse projection matrix to set.
		 */
		void setProjectionInverse(uint32_t idx, const Mat4f &projInv) {
			setStamped(projInv_, projStamp_, idx, projInv);
		}

		/**
		 * Get a vector of view-projection matrices used to transform world-space to screen-space.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @return the view-projection matrices.
		 */
		const std::span<Mat4f> &viewProjection() const { return viewProj_; }

		/**
		 * Get the view-projection matrix for a specific layer.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @param idx the layer index.
		 * @return the view-projection matrix for the specified layer.
		 */
		const Mat4f &viewProjection(uint32_t idx) const { return viewProj_[idx]; }

		/**
		 * Get the stamp indicating when the view-projection matrix was last updated.
		 * @return the view-projection stamp.
		 */
		uint32_t viewProjectionStamp() const { return viewProjStamp_; }

		/**
		 * Set the view-projection matrix for a specific layer, and increment the stamp
		 * indicating that the view-projection matrix has changed.
		 * @param idx the layer index.
		 * @param viewProj the view-projection matrix to set.
		 */
		void setViewProjection(uint32_t idx, const Mat4f &viewProj) {
			setStamped(viewProj_, viewProjStamp_, idx, viewProj);
		}

		/**
		 * Get a vector of inverse view-projection matrices used to transform screen-space to world-space.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @return the inverse view-projection matrices.
		 */
		const std::span<Mat4f> &viewProjectionInverse() const { return viewProjInv_; }

		/**
		 * Get the inverse view-projection matrix for a specific layer.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @param idx the layer index.
		 * @return the inverse view-projection matrix for the specified layer.
		 */
		const Mat4f &viewProjectionInverse(uint32_t idx) const { return viewProjInv_[idx]; }

		/**
		 * Set the inverse view-projection matrix for a specific layer, and increment the stamp
		 * indicating that the inverse view-projection matrix has changed.
		 * @param idx the layer index.
		 * @param viewProjInv the inverse view-projection matrix to set.
		 */
		void setViewProjectionInverse(uint32_t idx, const Mat4f &viewProjInv) {
			setStamped(viewProjInv_, viewProjStamp_, idx, viewProjInv);
		}

		/**
		 * Get the clip plane for this camera.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @return the clip plane.
		 */
		const std::vector<Vec4f> &clipPlane() const { return clipPlane_; }

		/**
		 * Get the clip plane for a specific index.
		 * This returns the latest value. Only safe to call in animation thread.
		 * @param idx the index of the clip plane.
		 * @return the clip plane for the specified index.
		 */
		const Vec4f &clipPlane(uint32_t idx) const { return clipPlane_[idx]; }

		/**
		 * Set the clip plane for this camera.
		 * @param plane the clip plane to set.
		 */
		void setClipPlane(uint32_t idx, const Vec4f &plane) {
			setStamped(clipPlane_, clipPlaneStamp_, idx, plane);
		}

		/**
		 * @return the 8 points forming this Frustum.
		 */
		const std::vector<Frustum> &frustum() const { return frustum_; }

		/**
		 * @return the 8 points forming this Frustum.
		 */
		std::vector<Frustum> &frustum() { return frustum_; }

		/**
		 * @param useAudio true if this camera is the OpenAL audio listener.
		 */
		void set_isAudioListener(bool useAudio);

		/**
		 * @return true if this camera is the OpenAL audio listener.
		 */
		bool isAudioListener() const { return isAudioListener_; }

		/**
		 * Get the frustum planes as a UBO.
		 * @return the UBO containing the frustum planes.
		 */
		ref_ptr<UBO> getFrustumBuffer();

		/**
		 * Update the frustum buffer with the current frustum planes.
		 * Usually this is called after the frustum has been updated internally,
		 * but if the camera is updated with a custom mechanism, this method
		 * must be called manually to update the frustum buffer.
		 */
		void updateFrustumBuffer();

		/**
		 * Attach the camera to a position, updating the camera position.
		 * @param attached the position to attach to.
		 */
		void attachToPosition(const ref_ptr<ModelTransformation> &attached);

		/**
		 * Update the camera pose based on the attached transform, if any.
		 */
		bool updatePose();

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

		/**
		 * Recompute the camera parameters.
		 * @return true if the camera was updated.
		 */
		virtual bool updateCamera();

		virtual void updateViewProjection1();

		void updateShaderData(float dt);

	protected:
		unsigned int numLayer_ = 1;
		bool isOmni_ = false;
		bool isOrtho_ = false;
		bool isAudioListener_ = false;
		unsigned int camStamp_ = 1u;
		// flag is used to avoid multiple updates of the camera
		// from multiple threads, e.g. window resize and usual
		// camera updates from controller could interfere.
		std::atomic<bool> isUpdating_{false};

		bool hasFixedLOD_ = false;
		LODQuality fixedLODQuality_ = LODQuality::LOW;

		std::vector<Frustum> frustum_;
		ref_ptr<UBO> frustumBuffer_;
		ref_ptr<ShaderInput4f> frustumData_;

		ref_ptr<UBO> cameraBlock_;
		ref_ptr<ShaderInputMat4> sh_view_;
		ref_ptr<ShaderInputMat4> sh_viewInv_;
		ref_ptr<ShaderInputMat4> sh_viewProj_;
		ref_ptr<ShaderInputMat4> sh_viewProjInv_;
		ref_ptr<ShaderInput4f> sh_direction_;
		ref_ptr<ShaderInput4f> sh_position_;
		ref_ptr<ShaderInput4f> sh_vel_;
		ref_ptr<ShaderInput4f> sh_projParams_;
		ref_ptr<ShaderInputMat4> sh_proj_;
		ref_ptr<ShaderInputMat4> sh_projInv_;
		ref_ptr<ShaderInput4f> sh_clipPlane_;

		// note: in additional to the buffered shader inputs, we have
		// a local-only copy of the camera data, which is used to
		// provide most recent camera data to CPU computations.
		std::vector<Mat4f> viewData_;
		std::span<Mat4f> view_;
		std::span<Mat4f> viewInv_;

		std::vector<Mat4f> viewProjData_;
		std::span<Mat4f> viewProj_;
		std::span<Mat4f> viewProjInv_;

		std::vector<Vec4f> direction_;
		std::vector<Vec4f> position_;
		Vec4f vel_;
		std::vector<ProjectionParams> projParams_;
		Vec3f localUp_ = Vec3f::right();

		std::vector<Mat4f> projData_;
		std::span<Mat4f> proj_;
		std::span<Mat4f> projInv_;

		std::vector<Vec4f> clipPlane_ = { Vec4f::zero() };

		ref_ptr<Animation> attachedMotion_;
		ref_ptr<ModelTransformation> attachedTF_;
		ref_ptr<Animation> cameraMotion_;

		virtual bool updateView();

		virtual void updateViewProjection(uint32_t projectionIndex, uint32_t viewIndex);

		void createFrustumBuffer();

		template<typename T>
		inline void setStamped(
				std::vector<T> &vec,
				uint32_t &stamp,
				uint32_t idx,
				const T &value) {
			vec[idx] = value;
			stamp += 1;
			camStamp_ += 1;
		}

		template<typename T>
		inline void setStamped(
				std::span<T> &vec,
				uint32_t &stamp,
				uint32_t idx,
				const T &value) {
			vec[idx] = value;
			stamp += 1;
			camStamp_ += 1;
		}

		inline void setStamped3(
				std::vector<Vec4f> &vec,
				uint32_t &stamp,
				uint32_t idx,
				const Vec3f &value) {
			vec[idx].xyz() = value;
			stamp += 1;
			camStamp_ += 1;
		}

	private:
		uint32_t viewStamp_ = 1u;
		uint32_t viewProjStamp_ = 1u;
		uint32_t directionStamp_ = 1u;
		uint32_t positionStamp_ = 1u;
		uint32_t velStamp_ = 1u;
		uint32_t projStamp_ = 1u;
		uint32_t clipPlaneStamp_ = 1u;

		uint32_t lastViewStamp1_ = 0u;
		uint32_t lastProjStamp1_ = 0u;
		uint32_t lastDirStamp1_ = 0u;
		uint32_t lastPosStamp1_ = 0u;
		uint32_t lastClipPlaneStamp1_ = 0u;
		uint32_t lastProjStamp_ = 0u;
		uint32_t lastPosStamp_ = 0u;
		uint32_t lastDirStamp_ = 0u;
		uint32_t poseStamp_ = 0u;
		Vec3f lastPosition_ = Vec3f::zero();
	};

	class ProjectionUpdater : public EventHandler {
	public:
		ProjectionUpdater(const ref_ptr<Camera> &cam,
						  const ref_ptr<Screen> &screen);

		~ProjectionUpdater() override = default;

		ProjectionUpdater(const ProjectionUpdater &) = delete;

		ProjectionUpdater &operator=(const ProjectionUpdater &) = delete;

		void call(EventObject *, EventData *) override;

	protected:
		ref_ptr<Camera> cam_;
		ref_ptr<Screen> screen_;
	};
} // namespace

#endif /* REGEN_CAMERA_H */
