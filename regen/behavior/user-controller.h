#ifndef REGEN_USER_CONTROLLER_H_
#define REGEN_USER_CONTROLLER_H_
#include "regen/camera/camera-controller.h"

namespace regen {
	/** Mapping from a key to a motion command. */
	struct MotionCommandMapping {
		std::string key;
		MotionType command;
		bool toggle = false;
		bool reverse = false;
	};

	/**
	 * A camera controller that controls a player character with bone animations.
	 */
	class UserController : public CameraController {
	public:
		/**
		 * Load a player controller from a scene input node.
		 * @param ctx The loading context.
		 * @param node The scene input node.
		 * @param userCamera The camera to control.
		 * @return The loaded player controller.
		 */
		static ref_ptr<UserController> load(
			LoadingContext &ctx, scene::SceneInputNode &node,
			const ref_ptr<Camera> &userCamera);

		/**
		 * Create a player controller for the given camera.
		 * @param cam The camera to control.
		 */
		UserController(const ref_ptr<Camera> &cam);

		/**
		 * Set the bone animation item for this controller.
		 * @param boneAnimation The bone animation item.
		 */
		void setBoneTree(const ref_ptr<BoneAnimationItem> &boneAnimation);

		/**
		 * Activate or deactivate a motion type.
		 * This is thread-safe and can be called from e.g. GUI event handlers.
		 * @param type The motion type to activate/deactivate.
		 * @param active true to activate, false to deactivate.
		 * @param reverse true to play the motion in reverse.
		 */
		void setMotionActive(MotionType type, bool active, bool reverse=false);

		/**
		 * This is thread-safe and can be called from e.g. GUI event handlers.
		 * @param type The motion type.
		 * @return true if the given motion type is currently active.
		 */
		bool isMotionActive(MotionType type) const;

		/**
		 * @param type The motion type.
		 * @return true if the agent can perform the given motion type.
		 */
		bool canPerformMotion(MotionType type) const {
			return motionIndices_.find(type) != motionIndices_.end();
		}

		// override Animation
		void cpuUpdate(double dt) override;

	protected:
		ref_ptr<BoneAnimationItem> boneAnimation_;
		ref_ptr<BoneController> boneController_;
		// static mapping from motion type to index in motion state array
		std::unordered_map<MotionType, uint32_t> motionIndices_;
		// vector of all motion types the agent can perform
		std::vector<MotionType> motionTypes_;
		// flag indicating if a motion is active, this is an atomic array
		// such that GUI events can safely toggle the flags.
		// The size is constant, and equals the number of motion types the agent can perform.
		std::vector<std::atomic<bool>> motionState_;
		// Each frame we update the list of active motions for the bone controller
		// from the motionState_ array.
		std::vector<MotionType> activeMotions_;
		size_t numActiveMotions_ = 0;
	};
} // namespace

#endif /* REGEN_USER_CONTROLLER_H_ */
