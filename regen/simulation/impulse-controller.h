#ifndef REGEN_IMPULSE_CONTROLLER_H
#define REGEN_IMPULSE_CONTROLLER_H

#include <regen/camera/camera-controller.h>

namespace regen {
	/**
	 * \brief Camera controller that applies an impulse to a physical object.
	 */
	class ImpulseController : public CameraController {
	public:
		static ref_ptr<ImpulseController> load(LoadingContext &ctx,
			scene::SceneInputNode &node,
			const ref_ptr<Camera> &userCamera);

		/**
		 * @param cam The camera.
		 * @param physicalObject The physical object.
		 */
		ImpulseController(const ref_ptr<Camera> &cam, const ref_ptr<PhysicalObject> &physicalObject);

		/**
		 * @param factor the physics speed factor.
		 */
		void setPhysicsSpeedFactor(float factor) { physicsSpeedFactor_ = factor; }

		/**
		 * @return the physics speed factor.
		 */
		auto physicsSpeedFactor() { return physicsSpeedFactor_; }

		// override Animation
		void cpuUpdate(double dt) override;

	protected:
		ref_ptr<PhysicalObject> physicalObject_;
		float physicsSpeedFactor_;
	};
}

#endif //REGEN_IMPULSE_CONTROLLER_H
