/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef REGEN_CAMERA_KEY_FRAME_CONTROLLER_H
#define REGEN_CAMERA_KEY_FRAME_CONTROLLER_H

#include <regen/camera/camera-controller-base.h>
#include <regen/animations/animation.h>
#include "camera-anchor.h"

namespace regen {
	/**
	 * \brief Computes the view matrix of a Camera based on user specified key frames.
	 */
	class KeyFrameController : public Animation, public CameraControllerBase {
	public:
		explicit KeyFrameController(const ref_ptr<Camera> &cam);

		~KeyFrameController() override = default;

		/**
		 * @return the current camera position.
		 */
		auto& cameraPosition() const { return camPos_; }

		/**
		 * @return the current camera direction.
		 */
		auto& cameraDirection() const { return camDir_; }

		/**
		 * Adds a frame to the list of key frames for camera animation.
		 */
		void push_back(const Vec3f &pos, const Vec3f &dir, GLdouble dt);

		/**
		 * Adds a frame to the list of key frames for camera animation.
		 */
		void push_back(const ref_ptr<CameraAnchor> &anchor, GLdouble dt);

		/**
		 * @param intensity the intensity of the ease in/out effect.
		 */
		void setEaseInOutIntensity(GLdouble intensity) { easeInOutIntensity_ = intensity; }

		/**
		 * @param pauseTime the pause time between key frames.
		 */
		void setPauseBetweenFrames(GLdouble pauseTime) { pauseTime_ = pauseTime; }

		/**
		 * @param repeat the repeat flag.
		 */
		void setRepeat(GLboolean repeat) { repeat_ = repeat; }

		// override
		void animate(GLdouble dt) override;

	protected:
		struct CameraKeyFrame {
			ref_ptr<CameraAnchor> anchor;
			GLdouble dt;
		};
		std::list<CameraKeyFrame> frames_;
		std::list<CameraKeyFrame>::iterator it_;
		CameraKeyFrame lastFrame_;
		Vec3f camPos_;
		Vec3f camDir_;
		GLdouble dt_;
		GLdouble easeInOutIntensity_;
		GLboolean repeat_;
		GLboolean skipFirstFrameOnLoop_;

		GLdouble pauseTime_;
		GLdouble currentPauseDuration_;
		GLboolean isPaused_;

		Vec3f interpolatePosition(const Vec3f &v0, const Vec3f &v1, GLdouble t) const;
		Vec3f interpolateDirection(const Vec3f &v0, const Vec3f &v1, GLdouble t) const;
	};
} // namespace

#endif //REGEN_CAMERA_KEY_FRAME_CONTROLLER_H
