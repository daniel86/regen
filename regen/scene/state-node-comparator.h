#ifndef STATE_NODE_COMPARATOR_H_
#define STATE_NODE_COMPARATOR_H_

#include <regen/objects/model-transformation.h>
#include <regen/camera/camera.h>
#include <regen/scene/state-node.h>

namespace regen {
	/**
	 * \brief Compares nodes by distance to camera.
	 */
	class NodeEyeDepthComparator {
	public:
		/**
		 * @param cam the perspective camera.
		 * @param frontToBack sort front to back or back to front
		 */
		NodeEyeDepthComparator(const ref_ptr<Camera> &cam, bool frontToBack);

		/**
		 * @param worldPosition the world position.
		 * @return world position camera distance.
		 */
		float getEyeDepth(const Vec3f &worldPosition) const;

		/**
		 * @param n a node.
		 * @return a model view matrix.
		 */
		ModelTransformation *findModelTransformation(StateNode *n) const;

		/**
		 * @param n a node.
		 * @return a model view matrix.
		 */
		ModelTransformation *getModelTransformation(StateNode *n) const;

		/**
		 * Do the comparison.
		 */
		bool operator()(ref_ptr<StateNode> &n0, ref_ptr<StateNode> &n1) const;

	protected:
		mutable std::map<StateNode*, ModelTransformation*> modelTransformations_;
		ref_ptr<Camera> cam_;
		int mode_;
	};
} // namespace

#endif /* STATE_NODE_COMPARATOR_H_ */
