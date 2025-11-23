#ifndef REGEN_STATE_NODE_H_
#define REGEN_STATE_NODE_H_

#include <queue>
#include <regen/gl-types/render-state.h>
#include <regen/states/state.h>

namespace regen {
	/**
	 * \brief A node that holds a State.
	 */
	class StateNode {
	public:
		StateNode();

		/**
		 * @param state the state object.
		 */
		explicit StateNode(const ref_ptr<State> &state);

		virtual ~StateNode() = default;

		/**
		 * @return Node name. Has no semantics.
		 */
		const std::string &name() const { return name_; }

		/**
		 * @param name Node name. Has no semantics.
		 */
		void set_name(std::string_view name) { name_ = name; }

		/**
		 * Removes all children.
		 */
		void clear();

		/**
		 * @return the state object.
		 */
		const ref_ptr<State> &state() const { return state_; }

		/**
		 * @return is the node hidden.
		 */
		bool isHidden() const { return isHidden_; }

		/**
		 * @param isHidden is the node hidden.
		 */
		void set_isHidden(bool isHidden) { isHidden_ = isHidden; }

		/**
		 * @return true if a parent is set.
		 */
		bool hasParent() const;

		/**
		 * @return the parent node.
		 */
		StateNode *parent() const { return parent_; }

		/**
		 * @param parent the parent node.
		 */
		void set_parent(StateNode *parent) { parent_ = parent; }

		/**
		 * Add a child node to the end of the child list.
		 */
		void addChild(const ref_ptr<StateNode> &child);

		/**
		 * Add a child node to the start of the child list.
		 */
		void addFirstChild(const ref_ptr<StateNode> &child);

		/**
		 * Removes a child node.
		 */
		void removeChild(StateNode *child);

		/**
		 * @return list of all child nodes.
		 */
		auto &childs() { return childs_; }

		/**
		 * @return list of all child nodes.
		 */
		auto &childs() const { return childs_; }

		ref_ptr<State> getParentCamera();

		ref_ptr<State> getParentFrameBuffer();

		/**
		 * Find a node with a given name.
		 */
		StateNode *findNodeWithName(std::string_view name);

		template<typename StateType>
		StateType *findStateWithType() {
			auto queue = std::queue<StateNode *>();
			queue.push(this);

			while (!queue.empty()) {
				auto node = queue.front();
				queue.pop();

				auto *thisState = dynamic_cast<StateType *>(node->state_.get());
				if (thisState) {
					return thisState;
				}
				auto j = node->state_->joined();
				for (auto &joined: *j.get()) {
					auto *joinedState = dynamic_cast<StateType *>(joined.get());
					if (joinedState) {
						return joinedState;
					}
				}

				for (auto &child: node->childs_) {
					queue.push(child.get());
				}
			}

			return nullptr;
		}

		template<typename StateType>
		void foreachWithType(std::function<bool(StateType &)> const &func) {
			auto queue = std::queue<StateNode *>();
			queue.push(this);

			while (!queue.empty()) {
				auto node = queue.front();
				queue.pop();

				auto *thisState = dynamic_cast<StateType *>(node->state_.get());
				if (thisState) {
					if (func(*thisState)) {
						return;
					}
				}
				auto j = node->state_->joined();
				for (auto &joined: *j.get()) {
					auto *joinedState = dynamic_cast<StateType *>(joined.get());
					if (joinedState) {
						if (func(*joinedState)) {
							return;
						}
					}
				}

				for (auto &child: node->childs_) {
					queue.push(child.get());
				}
			}
		}

		/**
		 * Scene graph traversal.
		 */
		virtual void traverse(RenderState *rs);

	protected:
		ref_ptr<State> state_;
		StateNode *parent_;
		std::list<ref_ptr<StateNode> > childs_;
		bool isHidden_;
		std::string name_;
	};

	/**
	 * A named object.
	 */
	struct NamedObject {
		int id;
		ref_ptr<StateNode> node;
	};

} // namespace

namespace regen {
	/**
	 * \brief Provides some global uniforms and keeps
	 * a reference on the render state.
	 */
	class RootNode : public StateNode {
	public:
		RootNode();

		/**
		 * Initialize node. Should be called when GL context setup.
		 */
		void init();

		/**
		 * Tree traversal.
		 * @param dt time difference to last traversal.
		 */
		void render(float dt);

		/**
		 * Do something after render call.
		 * @param dt time difference to last traversal.
		 */
		static void postRender(double dt);
	};
} // namespace

namespace regen {
	/**
	 * \brief Adds the possibility to traverse child tree
	 * n times.
	 */
	class LoopNode : public StateNode {
	public:
		/**
		 * @param numIterations The number of iterations.
		 */
		explicit LoopNode(uint32_t numIterations);

		/**
		 * @param state Associated state.
		 * @param numIterations The number of iterations.
		 */
		LoopNode(const ref_ptr<State> &state, uint32_t numIterations);

		/**
		 * @return The number of iterations.
		 */
		uint32_t numIterations() const;

		/**
		 * @param numIterations The number of iterations.
		 */
		void set_numIterations(uint32_t numIterations);

		// Override
		void traverse(RenderState *rs) override;

	protected:
		uint32_t numIterations_;
	};
} // namespace

#endif /* REGEN_STATE_NODE_H_ */
