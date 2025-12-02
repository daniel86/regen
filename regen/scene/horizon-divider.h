#ifndef REGEN_HORIZON_DIVIDER_NODE_H_
#define REGEN_HORIZON_DIVIDER_NODE_H_

#include <regen/scene/state-node.h>
#include <regen/camera/camera.h>

namespace regen {
	/**
	 * \brief A node that divides scene graph traversal
	 * based on camera position relative to a horizon level.
	 */
	class HorizonDividerNode : public StateNode {
	public:
		/**
		 * Load HorizonDividerNode from scene input.
		 * @param ctx Loading context.
		 * @param input Scene input node.
		 * @return The loaded HorizonDividerNode.
		 */
		static ref_ptr<HorizonDividerNode> load(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * @param camera The camera to use for horizon determination.
		 */
		explicit HorizonDividerNode(const ref_ptr<Camera> &camera);

		~HorizonDividerNode() override = default;

		/**
		 * @param node The node to traverse when below horizon.
		 */
		void setBelowHorizonNode(const ref_ptr<StateNode> &node);

		/**
		 * @param node The node to traverse when above horizon.
		 */
		void setAboveHorizonNode(const ref_ptr<StateNode> &node);

		/**
		 * @param height The surface height (horizon level).
		 */
		void setSurfaceHeight(float height) { surfaceHeight_ = height; }

		/**
		 * @return The surface height (horizon level).
		 */
		float surfaceHeight() const { return surfaceHeight_; }

		/**
		 * @return true if camera is below horizon.
		 */
		bool isBelowHorizon() const;

		// override
		void traverse(RenderState *state) override;

	protected:
		ref_ptr<Camera> camera_;
		float surfaceHeight_ = 0.0f;

		ref_ptr<StateNode> belowHorizonNode_;
		ref_ptr<StateNode> aboveHorizonNode_;
	};
} // namespace

#endif /* GEOM_PICKING_STATE_H_ */
