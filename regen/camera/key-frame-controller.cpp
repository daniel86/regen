#include "key-frame-controller.h"

using namespace regen;

KeyFrameController::KeyFrameController(const ref_ptr<Camera> &cam)
		: Animation(false, true),
		  CameraControllerBase(cam),
		  repeat_(GL_TRUE),
		  skipFirstFrameOnLoop_(GL_TRUE),
		  pauseTime_(0.0),
		  currentPauseDuration_(0.0),
		  isPaused_(GL_FALSE) {
	setAnimationName("controller");
	camPos_ = cam->position()[0].xyz_();
	camDir_ = cam->direction()[0].xyz_();
	it_ = frames_.end();
	lastFrame_.anchor = ref_ptr<FixedCameraAnchor>::alloc(camPos_, camDir_);
	lastFrame_.dt = 0.0;
	dt_ = 0.0;
	easeInOutIntensity_ = 1.0;
	computeMatrices(camPos_, camDir_);
	updateCamera(camPos_, camDir_, 0.0);
}

void KeyFrameController::push_back(const ref_ptr<CameraAnchor> &anchor, GLdouble dt) {
	CameraKeyFrame f;
	f.anchor = anchor;
	f.dt = dt;
	frames_.push_back(f);
	if (frames_.size() == 1) {
		it_ = frames_.begin();
		lastFrame_ = *it_;
	}
}

void KeyFrameController::push_back(const Vec3f &pos, const Vec3f &dir, GLdouble dt) {
	auto anchor = ref_ptr<FixedCameraAnchor>::alloc(pos, dir);
	push_back(anchor, dt);
}

static inline auto easeInOutCubic(GLdouble t, GLdouble intensity) {
	t = pow(t, intensity);
	return t < 0.5 ? 4 * t * t * t : 1 - pow(-2 * t + 2, 3) / 2;
}

Vec3f KeyFrameController::interpolatePosition(const Vec3f &v0, const Vec3f &v1, GLdouble t) const {
	GLdouble sample;
	if (easeInOutIntensity_ > 0.0) {
		sample = easeInOutCubic(t, easeInOutIntensity_);
	} else {
		sample = t;
	}
    return math::mix(v0, v1, sample);
}

Vec3f KeyFrameController::interpolateDirection(const Vec3f &v0, const Vec3f &v1, GLdouble t) const {
	GLdouble sample;
	// skip if the direction is the same
	if (v0 == v1) {
		return v0;
	}
	if (easeInOutIntensity_ > 0.0) {
		sample = easeInOutCubic(t, easeInOutIntensity_);
	} else {
		sample = t;
	}
    return math::slerp(v0, v1, sample);
}

void KeyFrameController::animate(GLdouble dt) {
	GLdouble dtSeconds = dt / 1000.0;

	if (it_ == frames_.end()) {
		// end reached
		if (!frames_.empty() && lastFrame_.anchor->following()) {
			// we should keep following the last anchor
			it_ = frames_.end();
			--it_;
			// set dt to a very low value as we should have reached the end already
			it_->dt = dt_ + dtSeconds;
		}
		else {
			return;
		}
	}
	CameraKeyFrame &currentFrame = *it_;

	// handle state where we wait between frames.
	// dt_ will not be changed in the meantime.
	if (isPaused_) {
		currentPauseDuration_ += dtSeconds;
		if(currentPauseDuration_ >= pauseTime_) {
			isPaused_ = GL_FALSE;
			dtSeconds = currentPauseDuration_ - pauseTime_;
			currentPauseDuration_ = 0.0;
		} else {
			return;
		}
	}
	// below we are not in pause mode
	dt_ += dtSeconds;

	// reached next frame?
	if (dt_ > currentFrame.dt) {
		GLdouble dtNewFrame = dt_ - currentFrame.dt;
		// move to next frame
		++it_;
		lastFrame_ = currentFrame;
		// loop back to first frame
		if (it_ == frames_.end()) {
			if (repeat_) {
				it_ = frames_.begin();
				if (skipFirstFrameOnLoop_) {
					++it_;
					if (it_ == frames_.end()) return;
				}
			} else if (currentFrame.anchor->following()) {
				dt_ = 0.0;
				animate(dtNewFrame);
				return;
			} else {
				stopAnimation();
				return;
			}
		}
		// enter pause mode if pause time is set
		if (pauseTime_ > 0.0) {
			isPaused_ = GL_TRUE;
			currentPauseDuration_ = dtNewFrame;
			dt_ = 0.0;
			return;
		} else {
			dt_ = 0.0;
			animate(dtNewFrame);
			return;
		}
	}

	Vec3f pos0 = lastFrame_.anchor->position();
	Vec3f pos1 = currentFrame.anchor->position();
	Vec3f dir0 = lastFrame_.anchor->direction();
	Vec3f dir1 = currentFrame.anchor->direction();
	GLdouble t = currentFrame.dt > 0.0 ? dt_ / currentFrame.dt : 1.0;
	dir0.normalize();
	dir1.normalize();

	{
		camPos_ = interpolatePosition(pos0, pos1, t);
		camDir_ = interpolateDirection(dir0, dir1, t);
		camDir_.normalize();
		computeMatrices(camPos_, camDir_);
		updateCamera(camPos_, camDir_, dt);
	}
}
