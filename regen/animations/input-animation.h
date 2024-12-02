#ifndef REGEN_INPUT_ANIMATION_H
#define REGEN_INPUT_ANIMATION_H

#include <regen/animations/animation.h>

namespace regen {
	/**
	 * Input key frame.
	 * @tparam T the type of the value, e.g. Vec3f.
	 */
	template<class T>
	struct InputKeyFrame {
		T val;
		GLdouble dt;
	};

	/**
	 * Input animation used to animate shader inputs.
	 * @tparam U the type of the input, e.g. ShaderInputVec3.
	 * @tparam T the type of the value, e.g. Vec3f.
	 */
	template<class U, class T>
	class InputAnimation : public Animation {
	public:
		explicit InputAnimation(const ref_ptr <U> &in)
				: Animation(GL_TRUE, GL_TRUE),
				  in_(in) {
			val_ = in_->getVertex(0);
			it_ = frames_.end();
			lastFrame_.val = val_;
			lastFrame_.dt = 0.0;
			dt_ = 0.0;
			setAnimationName(REGEN_STRING("animation-" << in->name()));
		}

		void push_back(const T &value, GLdouble dt) {
			InputKeyFrame<T> f;
			f.val = value;
			f.dt = dt;
			frames_.push_back(f);
			if (frames_.size() == 1) {
				it_ = frames_.begin();
			}
		}

		// Override
		void animate(GLdouble dt) override {
			if (it_ == frames_.end()) {
				it_ = frames_.begin();
				dt_ = 0.0;
			}
			InputKeyFrame<T> &currentFrame = *it_;

			dt_ += dt / 1000.0;
			if (dt_ >= currentFrame.dt) {
				++it_;
				lastFrame_ = currentFrame;
				GLdouble dt__ = dt_ - currentFrame.dt;
				dt_ = 0.0;
				animate(dt__);
			} else {
				GLdouble t = currentFrame.dt > 0.0 ? dt_ / currentFrame.dt : 1.0;
				lock();
				{
					val_ = math::mix(lastFrame_.val, currentFrame.val, t);
				}
				unlock();
			}
		}

		void glAnimate(RenderState *rs, GLdouble dt) override {
			lock();
			{
				in_->setVertex(0, val_);
			}
			unlock();
		}

	protected:
		ref_ptr <U> in_;
		std::list <InputKeyFrame<T>> frames_;
		typename std::list<InputKeyFrame<T> >::iterator it_;
		InputKeyFrame<T> lastFrame_;
		GLdouble dt_;
		T val_;
	};
}

#endif //REGEN_INPUT_ANIMATION_H
