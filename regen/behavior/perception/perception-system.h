#ifndef REGEN_PERCEPTION_SYSTEM_H_
#define REGEN_PERCEPTION_SYSTEM_H_

#include "regen/shapes/spatial-index.h"
#include "regen/shapes/bounding-shape.h"
#include "collision-monitor.h"
#include "detection-monitor.h"

namespace regen {
	/**
	 * A perception system for NPCs.
	 * It uses a spatial index to detect nearby objects and notifies registered perception monitors.
	 */
	class PerceptionSystem {
	public:
		/**
		 * Constructor.
		 * @param spatialIndex the spatial index to use for perception.
		 * @param indexedShape the shape of the NPC in the spatial index.
		 */
		PerceptionSystem(
			const ref_ptr<SpatialIndex> &spatialIndex,
			const ref_ptr<BoundingShape> &indexedShape);

		virtual ~PerceptionSystem() = default;

		/**
		 * @return the collision shape used for perception.
		 */
		ref_ptr<Frustum> collisionShape() { return collisionShape_; }

		/**
		 * Add a perception monitor to be notified of perception events.
		 * @param monitor the perception monitor.
		 */
		void addMonitor(PerceptionMonitor *monitor);

		/**
		 * Remove a perception monitor.
		 * @param monitor the perception monitor.
		 */
		void removeMonitor(PerceptionMonitor *monitor);

		/**
		 * Set the collision bit for spatial index traversal.
		 * @param bit the collision bit.
		 */
		void setCollisionBit(uint32_t bit) { collisionMask_ = (1 << bit); }

		/**
		 * Set an offset for the eye position used in perception.
		 * @param offset the eye offset.
		 */
		void setEyeOffset(const Vec3f &offset) { eyeOffset_ = offset; }

		/**
		 * Update the perception system.
		 * This should be called periodically to update the perception data and notify monitors.
		 * @param dt_s the time difference in seconds.
		 */
		void update(const Blackboard &kb, double dt_s);

		/**
		 * Handle an intersection with another shape.
		 * @param other the other shape.
		 */
		void handleIntersection(const BoundingShape &other);

	protected:
		// The spatial index used for perception.
		ref_ptr<SpatialIndex> spatialIndex_;
		// The shape of the NPC in the spatial index.
		ref_ptr<BoundingShape> indexedShape_;
		// shape used for collision avoidance, it extends in walk direction
		ref_ptr<Frustum> collisionShape_;
		uint32_t lastCollisionShapeStamp_ = 0;
		// collision mask to ignore collisions with other NPCs
		uint32_t collisionMask_ = 0;
		// Eye offset from the indexed shape origin.
		Vec3f eyeOffset_ = Vec3f::zero();
		// The perception data computed during intersection handling.
		CollisionEvent collisionEvt_;
		DetectionEvent detectionEvt_;
		// The list of registered monitors.
		std::vector<CollisionMonitor*> collisionMonitors_;
		std::vector<DetectionMonitor*> detectionMonitors_;

		void updateCollisions();

		void updateCollisionShape(bool forceUpdate);
	};
} // namespace

#endif /* REGEN_PERCEPTION_MONITOR_H_ */

