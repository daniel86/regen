#include "transform-animation.h"
#include "regen/math/quaternion.h"

using namespace regen;

TransformAnimation::TransformAnimation(const ref_ptr<ShaderInputMat4> &in)
		: Animation(GL_TRUE, GL_TRUE),
		  in_(in) {
	auto &currentTransform = in_->getVertex(0);
	currentPos_ = currentTransform.position();
	// TODO: get euler angles
	currentDir_ = Vec3f(0.0f);
	it_ = frames_.end();
	lastFrame_.pos = currentPos_;
	lastFrame_.rotation = currentDir_;
	lastFrame_.dt = 0.0;
	dt_ = 0.0;
}

void TransformAnimation::push_back(const std::optional<Vec3f> &pos,
								   const std::optional<Vec3f> &rotation,
								   GLdouble dt) {
	TransformKeyFrame f;
	f.pos = pos;
	f.rotation = rotation;
	f.dt = dt;
	frames_.push_back(f);
	if (frames_.size() == 1) {
		it_ = frames_.begin();
	}
}

// Override
void TransformAnimation::animate(GLdouble dt) {
	if (it_ == frames_.end()) {
		it_ = frames_.begin();
		dt_ = 0.0;
	}
	TransformKeyFrame &currentFrame = *it_;

	dt_ += dt / 1000.0;
	if (dt_ >= currentFrame.dt) {
		++it_;
		lastFrame_ = currentFrame;
		lastFrame_.pos = currentPos_;
		lastFrame_.rotation = currentDir_;
		GLdouble dt__ = dt_ - currentFrame.dt;
		dt_ = 0.0;
		animate(dt__);
	} else {
		GLdouble t = currentFrame.dt > 0.0 ? dt_ / currentFrame.dt : 1.0;
		lock();
		{
			if (mesh_.get() != nullptr && mesh_->physicalObjects().size() > 0) {
				// TODO: not sure why, but only if setting here the velocity
				// so far makes the mesh move exactly with the bullet shape.
				auto &physicalObject = mesh_->physicalObjects()[0];
				physicalObject->rigidBody()->setMotionState(nullptr);
				btTransform btCurrentVal;
				btTransform btLastVal = physicalObject->rigidBody()->getWorldTransform();
				btCurrentVal.setFromOpenGLMatrix((btScalar*)&currentVal_);
				physicalObject->rigidBody()->setLinearVelocity(
						(btCurrentVal.getOrigin() - btLastVal.getOrigin()));
				physicalObject->rigidBody()->setWorldTransform(btCurrentVal);
			}

			if (currentFrame.pos.has_value()) {
				currentPos_ = math::mix(lastFrame_.pos.value(), currentFrame.pos.value(), t);
			}
			if (currentFrame.rotation.has_value()) {
				currentDir_ = math::slerp(lastFrame_.rotation.value(), currentFrame.rotation.value(), t);
			}
			Quaternion q(0.0, 0.0, 0.0, 1.0);
			q.setEuler(currentDir_.x, currentDir_.y, currentDir_.z);
			currentVal_ = q.calculateMatrix();
			currentVal_.translate(currentPos_);
		}
		unlock();
	}
}

void TransformAnimation::glAnimate(RenderState *rs, GLdouble dt) {
	lock();
	{
		in_->setVertex(0, currentVal_);
	}
	unlock();
}
