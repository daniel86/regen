
#ifndef PHYSICAL_OBJECT_H_
#define PHYSICAL_OBJECT_H_

#include <regen/utility/ref-ptr.h>
#include <regen/physics/physical-props.h>

#include <btBulletDynamicsCommon.h>

namespace regen {
	/**
	 * Object in physics simulation.
	 */
	class PhysicalObject {
	public:
		/**
		 * Default constructor.
		 * @param props Properties used to create this object.
		 */
		explicit PhysicalObject(const ref_ptr<PhysicalProps> &props);

		/**
		 * @return Rigid body representation.
		 */
		auto &rigidBody() { return rigidBody_; }

		/**
		 * @return The collision shape.
		 */
		auto &shape() { return props_->shape(); }

		/**
		 * @return Motion state for transform synchronization.
		 */
		auto &motionState() { return props_->motionState(); }

		/**
		 * Set the motion state.
		 * @param motionState The motion state.
		 */
		void setMotionState(const ref_ptr<btMotionState> &motionState);

		/**
		 * @return the physical properties.
		 */
		auto &props() const { return props_; }

	protected:
		ref_ptr<btRigidBody> rigidBody_;
		ref_ptr<PhysicalProps> props_;
	};
} // namespace

#endif /* PHYSICAL_OBJECT_H_ */
