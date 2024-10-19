/*
 * animation.h
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#ifndef GL_ANIMATION_H_
#define GL_ANIMATION_H_

#include <GL/glew.h>
#include <boost/thread/mutex.hpp>

#include <regen/utility/event-object.h>
#include <regen/gl-types/render-state.h>
#include "regen/gl-types/shader.h"

namespace regen {
	/**
	 * \brief Abstract base class for animations.
	 */
	class Animation : public EventObject {
	public:
		/**
		 * The animation state switched to active.
		 */
		static GLuint ANIMATION_STARTED;
		/**
		 * The animation state switched to inactive.
		 */
		static GLuint ANIMATION_STOPPED;

		/**
		 * Create an animation.
		 * Note that the animation removes itself from the AnimationManager
		 * in the destructor.
		 * @param useGLAnimation execute with render context.
		 * @param useAnimation execute without render context in separate thread.
		 * @param autoStart is true the animation adds itself to the AnimationManager.
		 */
		Animation(GLboolean useGLAnimation, GLboolean useAnimation, GLboolean autoStart = GL_TRUE);

		virtual ~Animation();

		/**
		 * @return true if this animation is active.
		 */
		GLboolean isRunning() const;

		/**
		 * Activate this animation.
		 */
		virtual void startAnimation();

		/**
		 * Deactivate this animation.
		 */
		virtual void stopAnimation();

		/**
		 * Mutex lock for data access.
		 * @return false if not successful.
		 */
		GLboolean try_lock();

		/**
		 * Mutex lock for data access.
		 * @return false if not successful.
		 */
		GLboolean try_lock_gl();

		/**
		 * Mutex lock for data access.
		 * Blocks until lock can be acquired.
		 */
		void lock();

		/**
		 * Mutex lock for data access.
		 * Blocks until lock can be acquired.
		 */
		void lock_gl();

		/**
		 * Mutex lock for data access.
		 * Unlocks a previously acquired lock.
		 */
		void unlock();

		/**
		 * Mutex lock for data access.
		 * Unlocks a previously acquired lock.
		 */
		void unlock_gl();

		/**
		 * Waits for a while.
		 * @param milliseconds number of ms to wait
		 */
		void wait(GLuint milliseconds);

		/**
		 * @return true if the animation implements glAnimate().
		 */
		GLboolean useGLAnimation() const;

		/**
		 * @return true if the animation implements animate().
		 */
		GLboolean useAnimation() const;

		/**
		 * Make the next animation step.
		 * This should be called each frame.
		 * @param dt time difference to last call in milliseconds.
		 */
		virtual void animate(GLdouble dt) {}

		/**
		 * Upload animation data to GL.
		 * This should be called each frame in a thread
		 * with a GL context.
		 * @param rs the render state.
		 * @param dt time difference to last call in milliseconds.
		 */
		virtual void glAnimate(RenderState *rs, GLdouble dt) {}

		/**
		 * @return the shader if any.
		 */
		const auto& shader() { return shader_; }

		/**
		 * Set the shader.
		 * @param shader the shader.
		 */
		void setShader(const ref_ptr<Shader> &shader) { shader_ = shader; }

	protected:
		boost::mutex mutex_;
		boost::mutex mutex_gl_;
		GLboolean useGLAnimation_;
		GLboolean useAnimation_;
		GLboolean isRunning_;
		ref_ptr<Shader> shader_;

		Animation(const Animation &);

		void operator=(const Animation &);
	};
} // namespace

#endif /* GL_ANIMATION_H_ */
