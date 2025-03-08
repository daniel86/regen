#ifndef REGEN_STATE_NODE_H_
#define REGEN_STATE_NODE_H_

#include <regen/gl-types/render-state.h>
#include <regen/states/state.h>
#include <regen/camera/camera.h>

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
		void set_name(const std::string &name) { name_ = name; }

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
		GLboolean isHidden() const { return isHidden_; }

		/**
		 * @param isHidden is the node hidden.
		 */
		void set_isHidden(GLboolean isHidden) { isHidden_ = isHidden; }

		/**
		 * @return true if a parent is set.
		 */
		GLboolean hasParent() const;

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

		ref_ptr<Camera> getParentCamera();

		/**
		 * Find a node with a given name.
		 */
		StateNode *findNodeWithName(const std::string &name);

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
				for (auto &joined: node->state_->joined()) {
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
				for (auto &joined: node->state_->joined()) {
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
		GLboolean isHidden_;
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
		void render(GLdouble dt);

		/**
		 * Do something after render call.
		 * @param dt time difference to last traversal.
		 */
		static void postRender(GLdouble dt);
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
		explicit LoopNode(GLuint numIterations);

		/**
		 * @param state Associated state.
		 * @param numIterations The number of iterations.
		 */
		LoopNode(const ref_ptr<State> &state, GLuint numIterations);

		/**
		 * @return The number of iterations.
		 */
		GLuint numIterations() const;

		/**
		 * @param numIterations The number of iterations.
		 */
		void set_numIterations(GLuint numIterations);

		// Override
		void traverse(RenderState *rs) override;

	protected:
		GLuint numIterations_;
	};
} // namespace

#endif /* REGEN_STATE_NODE_H_ */
