#include "transform-animation.h"

using namespace regen;

TransformAnimation::TransformAnimation(const ref_ptr<ModelTransformation> &tf, uint32_t tfIdx)
		: Animation(false, true),
		  tf_(tf),
		  tfIdx_(tfIdx) {
	auto currentTransform = tf_->modelMat()->getVertex(tfIdx_);
	it_ = frames_.end();
	dt_ = 0.0;
	setAnimationName(REGEN_STRING("animation-"<<tf->modelMat()->name()));
	// initialize transform data
	currentPos_ = currentTransform.r.position();
	currentVal_ = currentTransform.r;
	initialScale_ = currentTransform.r.scaling();
	// remove scaling before computing rotation, else we get faulty results
	auto tmp = currentTransform.r;
	tmp.scale(Vec3f(
			1.0f / initialScale_.x,
			1.0f / initialScale_.y,
			1.0f / initialScale_.z));
	currentDir_ = tmp.rotation();
	// set last frame
	lastFrame_.pos = currentPos_;
	lastFrame_.rotation = currentDir_;
	lastFrame_.dt = 0.0;
}

void TransformAnimation::addTransformKeyframe(
		const std::optional<Vec3f> &pos,
		const std::optional<Vec3f> &rotation,
		double dt) {
	TransformKeyFrame f;
	f.pos = pos;
	f.rotation = rotation;
	f.dt = dt;
	frames_.push_back(f);
	// reset iterator if this is the first frame
	if (frames_.size() == 1) {
		it_ = frames_.begin();
		dt_ = 0.0;
	}
}

void TransformAnimation::updatePose(const TransformKeyFrame &currentFrame, double t) {
	Vec3f tmp;
	if (currentFrame.pos.has_value()) {
		tmp = math::mix(lastFrame_.pos.value(), currentFrame.pos.value(), t);
		currentPos_.x = tmp.x;
		currentPos_.y = tmp.y;
		currentPos_.z = tmp.z;
	}
	if (currentFrame.rotation.has_value()) {
		tmp = math::slerp(lastFrame_.rotation.value(), currentFrame.rotation.value(), t);
		currentDir_.x = tmp.x;
		currentDir_.y = tmp.y;
		currentDir_.z = tmp.z;
	}
}

// Override
void TransformAnimation::cpuUpdate(double dt) {
	if (it_ == frames_.end()) {
		if (loopTransformAnimation_) {
			it_ = frames_.begin();
			dt_ = 0.0;
		} else {
			dt_ = 0.0;
			return;
		}
	}
	TransformKeyFrame &currentFrame = *it_;

	dt_ += dt / 1000.0;
	if (dt_ >= currentFrame.dt) {
		++it_;
		lastFrame_ = currentFrame;
		lastFrame_.pos = currentPos_;
		lastFrame_.rotation = currentDir_;
		double dt__ = dt_ - currentFrame.dt;
		dt_ = 0.0;
		cpuUpdate(dt__);
	} else {
		double t = currentFrame.dt > 0.0 ? dt_ / currentFrame.dt : 1.0;
		{
			if (mesh_.get() != nullptr && mesh_->physicalObjects().size() > 0) {
				auto &physicalObject = mesh_->physicalObjects()[0];
				physicalObject->rigidBody()->setMotionState(nullptr);
				btTransform btCurrentVal;
				btTransform btLastVal = physicalObject->rigidBody()->getWorldTransform();
				btCurrentVal.setFromOpenGLMatrix((btScalar*)&currentVal_);
				physicalObject->rigidBody()->setLinearVelocity(
						(btCurrentVal.getOrigin() - btLastVal.getOrigin()));
				physicalObject->rigidBody()->setWorldTransform(btCurrentVal);
			}

			updatePose(currentFrame, t);
			Quaternion q;
			q.setEuler(currentDir_.x, currentDir_.y, currentDir_.z);
			currentVal_ = q.calculateMatrix();
			currentVal_.scale(initialScale_);
			currentVal_.translate(currentPos_);
		}
		tf_->setModelMat(tfIdx_, currentVal_);
	}
}
