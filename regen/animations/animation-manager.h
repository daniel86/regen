/*
 * animation-manager.h
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#ifndef GL_ANIMATION_MANAGER_H_
#define GL_ANIMATION_MANAGER_H_

#include <list>
#include <set>

#include <regen/utility/threading.h>
#include <regen/animations/animation.h>
#include "regen/shapes/spatial-index.h"

namespace regen {
	/**
	 * \brief Manages multiple glAnimations in a separate thread.
	 */
	class AnimationManager : public Thread {
	public:
		/**
		 * @return animation manager reference.
		 */
		static AnimationManager &get();

		/**
		 * Adds an animation.
		 * @param animation a Animation instance.
		 */
		void addAnimation(Animation *animation);

		/**
		 * Removes previously added animation.
		 * @param animation a Animation instance.
		 */
		void removeAnimation(Animation *animation);

		/**
		 * Invoke glAnimate() on added glAnimations.
		 * @param rs the render state.
		 * @param dt time difference to last call in milliseconds.
		 */
		void updateGraphics(RenderState *rs, GLdouble dt);

		/**
		 * Wait until next step was calculated in animation thread.
		 */
		void waitForStep();

		/**
		 * Wake up the animation thread if it is waiting for
		 * the next frame to finish.
		 */
		void nextFrame();

		/**
		 * Close animation thread.
		 */
		void close(bool blocking = false);

		/**
		 * Pause glAnimations.
		 * Can be resumed by call to resume().
		 */
		void pause(bool blocking = false);

		/**
		 * Clear all animations.
		 */
		void clear();

		/**
		 * Resumes previously paused glAnimations.
		 */
		void resume();

		/**
		 * Reset the time of the animation manager.
		 */
		void resetTime();

		/**
		 * @return the set of glAnimations.
		 */
		auto& graphicsAnimations() { return gpuAnimations_; }

		/**
		 * @return the set of animations.
		 */
		auto& synchronizedAnimations() { return synchronizedAnimations_; }

		/**
		 * @return the set of unsynchronized animations.
		 */
		auto& unsynchronizedAnimations() { return unsynchronizedAnimations_; }

		/**
		 * Set the root state.
		 * @param rootState the root state.
		 */
		void setRootState(const ref_ptr<State> &rootState);

		/**
		 * Set the spatial indices.
		 * @param indices the spatial indices.
		 */
		void setSpatialIndices(const std::map<std::string, ref_ptr<SpatialIndex>> &indices);

	private:

		boost::posix_time::ptime time_;
		boost::posix_time::ptime lastTime_;
		std::vector<Animation *> synchronizedAnimations_;
		std::vector<Animation *> unsynchronizedAnimations_;
		std::vector<boost::thread> unsynchronizedThreads_;
		std::set<Animation *> gpuAnimations_;
		ref_ptr<State> rootState_;
		std::map<std::string, ref_ptr<SpatialIndex>> spatialIndices_;

		boost::thread::id animationThreadID_;
		boost::thread::id glThreadID_;
		boost::thread::id removeThreadID_;
		boost::thread::id addThreadID_;
		bool animInProgress_;
		bool glInProgress_;
		bool removeInProgress_;
		bool addInProgress_;
		bool animChangedDuringLoop_;
		bool glChangedDuringLoop_;
		bool closeFlag_;
		bool pauseFlag_;

		boost::mutex stepMut_;
		boost::mutex frameMut_;
		boost::mutex unsynchronizedMut_;
		boost::condition_variable stepCond_;
		boost::condition_variable frameCond_;
		std::atomic<bool> hasNextFrame_ = false;
		std::atomic<bool> hasNextStep_ = false;

		AnimationManager();

		~AnimationManager() override;

		void run();

		void runUnsynchronized(Animation *animation) const;

		void nextStep();

		void waitForFrame();
	};
} // namespace

#endif /* GL_ANIMATION_MANAGER_H_ */
