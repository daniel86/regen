#ifndef REGEN_STATE_STACKS_H_
#define REGEN_STATE_STACKS_H_

#include <GL/glew.h>

#include <regen/config.h>
#include <regen/utility/stack.h>
#include <regen/math/vector.h>

namespace regen {
	/**
	 * \brief A stack of states that can be applied to a rendering context.
	 *
	 * The stack allows to push and pop states, and applies the current state
	 * when the top of the stack changes.
	 *
	 * @tparam T the type of the state value.
	 */
	template<typename T>
	class StateStack {
	public:
		/**
		 * \brief Default constructor.
		 * @param apply the function to apply the state value.
		 */
		explicit StateStack(void (*apply)(StateStack*, const T&)) : doApply_(apply) {
			// make room for at least 4 stacked values
			stacked_.resize(4);
			// guard apply function by checking if the value is different
			guardedApply_ = guardedApply;
			// initial apply is unguarded as no value is stacked yet
			apply_ = initialApply;
		}

		static void initialApply(StateStack *s, const T &v) {
			s->stacked_[0] = v;
			s->numStacked_ = 1;
			s->doApply_(s, v);
			s->apply_ = s->guardedApply_;
		}

		static void guardedApply(StateStack *s, const T &v) {
			auto &stacked = s->stacked_[s->numStacked_ - 1];
			if (v != stacked) {
				s->doApply_(s, v);
				stacked = v;
			}
		}

		/**
		 * @param v the new state value
		 */
		void apply(const T &v) { apply_(this, v); }

		/**
		 * \brief Get the current state value.
		 * @return the current state value.
		 */
		const T &current() const { return stacked_[numStacked_-1]; }

		/**
		 * \brief Push a value onto the stack.
		 * @param v the value.
		 */
		void push(const T &v) {
			if (numStacked_+1 >= stacked_.size()) {
				// resize the stack if it is full
				stacked_.resize(stacked_.size() * 2);
			}
			stacked_[numStacked_] = v;
			if (numStacked_==0) {
				doApply_(this, v);
				apply_ = guardedApply_;
			} else if (stacked_[numStacked_-1] != v) {
				doApply_(this, v);
			}
			numStacked_++;
		}

		/**
		 * \brief Pop out last value.
		 */
		void pop() {
			if (numStacked_ == 1) {
				// leave the first value on the stack
				return;
			}
			numStacked_--;
			const T &value1 = stacked_[numStacked_];
			const T &value0 = stacked_[numStacked_-1];
			if (value1 != value0) {
				// if the last value is different from the one before it, apply the new head value.
				doApply_(this, value0);
			}
		}

	protected:
		std::vector<T> stacked_;
		uint32_t numStacked_ = 0;
		void (*doApply_)(StateStack*, const T&);
		void (*guardedApply_)(StateStack*, const T&);
		void (*apply_)(StateStack*, const T&);
	};

	/**
	 * \brief A stack of states that can be applied to a rendering context.
	 *
	 * The values are passed by reference to the apply function.
	 *
	 * @tparam T the type of the state value.
	 */
	template<typename T>
	class ByReferenceStateStack : public StateStack<T> {
	public:
		explicit ByReferenceStateStack(void (*apply)(const T&))
				: StateStack<T>([](StateStack<T> *self, const T &v) {
						((ByReferenceStateStack<T> *)self)->fn_(v);
				  }),
				  fn_(apply) {}
	protected:
		void (*fn_)(const T&);
	};

	/**
	 * \brief A stack of states that can be applied to a rendering context.
	 *
	 * The values are passed by value to the apply function.
	 *
	 * @tparam T the type of the state value.
	 */
	template<typename T>
	class ByValueStateStack : public StateStack<T> {
	public:
		explicit ByValueStateStack(void (*apply)(T))
				: StateStack<T>([](StateStack<T> *self, const T &v) {
						((ByValueStateStack<T> *)self)->fn_(v);
				  }),
				  fn_(apply) {}
	protected:
		void (*fn_)(T);
	};

	/**
	 * \brief A stack of states that can be applied to a rendering context.
	 *
	 * The values are passed together with a key to the apply function.
	 * This is useful for OpenGL state changes that require a key to identify the state.
	 *
	 * @tparam T the type of the state value.
	 */
	template<typename T>
	class KeyedStateStack : public StateStack<T> {
	public:
		KeyedStateStack(GLenum key, void (*apply)(GLenum, T))
				: StateStack<T>([](StateStack<T> *self, const T &v) {
						auto *s = (KeyedStateStack<T> *)self;
						s->fn_(s->key_, v);
				  }),
				  fn_(apply),
				  key_(key) {}

		GLenum key() const { return key_; }
	protected:
		void (*fn_)(GLenum, T);
		GLenum key_;
	};

	///////////
	///////////

	template<typename T>
	void regen_lockedValue(const T &v) {}

	template<typename StackType, typename ValueType>
	static void applyFilledStamped(StackType *s, const ValueType &v) {
		if (v != s->head_->v) { s->apply_(v); }
	}

	template<typename StackType, typename ValueType>
	static void applyFilledStampedi(StackType *s, GLuint i, const ValueType &v) {
		if (v != s->headi_[i]->v) { s->applyi_(i, v); }
	}

	template<typename StackType, typename ValueType>
	static void applyInitStamped(StackType *s, const ValueType &v) {
		s->apply_(v);
		s->doApply_ = &applyFilledStamped;
	}

	template<typename StackType, typename ValueType>
	static void applyInitStampedi(StackType *s, GLuint i, const ValueType &v) {
		s->applyi_(i, v);
		s->doApplyi_[i] = &applyFilledStampedi;
	}

	template<typename T>
	void regen_lockedIndexed(GLuint i, const T &v) {}

	/**
	 * \brief State stack with indexed apply function.
	 *
	 * This means there is an apply function that applies to all indices
	 * and there is an apply that can apply to an ondividual index.
	 */
	template<typename ValueType>
	class IndexedStateStack {
	public:
		/**
		 * Function to apply the value to all indices.
		 */
		typedef void (*ApplyValue)(const ValueType &v);

		/**
		 * Function to apply the value to a single index.
		 */
		typedef void (*ApplyValueIndexed)(GLuint i, const ValueType &v);

		/**
		 * @param numIndices number of indices
		 * @param apply apply a stack value to all indices.
		 * @param applyi apply a stack value to a single index.
		 */
		IndexedStateStack(GLuint numIndices, ApplyValue apply, ApplyValueIndexed applyi)
				: numIndices_(numIndices),
				  head_(new Node(zeroValue_)),
				  apply_(apply),
				  applyi_(applyi),
				  lockCounter_(0),
				  lastStampi_(-1),
				  isEmpty_(GL_TRUE) {
			head_->stamp = -1;
			counter_.x = 0;
			counter_.y = 0;
			doApply_ = &applyInitStamped;

			headi_ = new Node *[numIndices];
			doApplyi_ = new DoApplyValueIndexed[numIndices];
			isEmptyi_ = new GLboolean[numIndices];
			for (GLuint i = 0; i < numIndices_; ++i) {
				doApplyi_[i] = &applyInitStampedi;
				headi_[i] = new Node(zeroValue_);
				isEmptyi_[i] = GL_TRUE;
			}
		}

		~IndexedStateStack() {
			if (head_) {
				deleteNodes(head_);
			}
			if (isEmptyi_) {
				delete[]isEmptyi_;
				isEmptyi_ = nullptr;
			}
			if (headi_) {
				for (GLuint i = 0; i < numIndices_; ++i) { deleteNodes(headi_[i]); }
				delete[]headi_;
				headi_ = nullptr;
			}
			if (doApplyi_) {
				delete[]doApplyi_;
				doApplyi_ = nullptr;
			}
		}

		/**
		 * @return the current state value or the value created by default constructor.
		 */
		const ValueType &globalValue() { return head_->v; }

		/**
		 * @return the current state value or the value created by default constructor.
		 */
		const auto& value(GLuint index) const {
			return headi_[index]->v;
		}

		/**
		 * Push a value onto the stack.
		 * Applies to all indices.
		 * @param v the value.
		 */
		void push(const ValueType &v) {
			if (counter_.y > 0) {
				// an indexed value was pushed before
				// if the indexed value was pushed before the last global push. only
				// apply when the value of last global push is not equal
				if (head_->stamp > lastStampi_) { doApply_(this, v); }
					// else an indexed value was pushed. always call apply even if some
					// indices may already contain the right value
				else { apply_(v); }
			} else {
				// no indexed push was done before
				doApply_(this, v);
			}

			if (isEmpty_) {
				// initial push to the stack or first push after everything
				// was popped out.
				head_->v = v;
				isEmpty_ = GL_FALSE;
			} else if (head_->next) {
				// use node created earlier
				head_ = head_->next;
				head_->v = v;
			} else {
				// first time someone pushed so deep
				head_->next = new Node(v, head_);
				head_ = head_->next;
			}
			head_->stamp = counter_.x;

			// count number of pushes for value stamps
			counter_.x += 1;
		}

		/**
		 * Push a value onto the stack with given index.
		 * @param index the index.
		 * @param v the value.
		 */
		void push(GLuint index, const ValueType &v) {
			Node *headi = headi_[index];

			if (counter_.x > counter_.y) {
				GLint lastStampi = (isEmptyi_[index] ? -1 : headi->stamp);
				// an global value was pushed before
				// if the global value was pushed before the last indexed push. only
				// apply when the value of last indexed push is not equal
				if (lastStampi > head_->stamp) { doApplyi_[index](this, index, v); }
					// else an global value was pushed. call apply if values not equal
				else if (v != head_->v) { applyi_(index, v); }
			} else {
				// no global push was done before
				doApplyi_[index](this, index, v);
			}

			if (isEmptyi_[index]) {
				// initial push to the stack or first push after everything
				// was popped out.
				headi->v = v;
				isEmptyi_[index] = GL_FALSE;
			} else if (headi->next) {
				// use node created earlier
				headi = headi->next;
				headi->v = v;
				headi_[index] = headi;
			} else {
				// first time someone pushed so deep
				headi->next = new Node(v, headi);
				headi = headi->next;
				headi_[index] = headi;
			}
			headi->stamp = counter_.y;
			lastStampi_ = counter_.y;

			// count number of pushes for value stamps
			counter_.x += 1;
			counter_.y += 1;
		}

		/**
		 * Apply a value to all indices.
		 */
		void apply(const ValueType &v) {
			if (head_->v != v) {
				apply_(v);
				head_->v = v;
			}
		}

		/**
		 * Apply a value to given index.
		 * @param index the value index.
		 * @param v the value.
		 */
		void apply(GLuint index, const ValueType &v) {
			if (headi_[index]->v != v) {
				applyi_(index, v);
				headi_[index]->v = v;
			}
		}

		/**
		 * Pop out last value that was applied to all indices.
		 */
		void pop() {
			// apply previously pushed global value
			if (head_->prev) {
				if (head_->v != head_->prev->v) {
					apply_(head_->prev->v);
				}
				head_ = head_->prev;
			} else {
				isEmpty_ = GL_TRUE;
			}

			// if there are indexed values pushed we may
			// have to re-enable them here...
			if (counter_.y > 0) {
				// Indexed states only applied with stamp>lastEqStamp
				// Loop over all indexed stacks and compare stamps of top element.
				for (GLuint i = 0; i < numIndices_; ++i) {
					if (isEmptyi_[i]) { continue; }
					Node *headi = headi_[i];

					if (headi->stamp > head_->stamp && head_->v != headi->v) { applyi_(i, headi->v); }
				}
			}

			// count number of pushes for value stamps
			counter_.x -= 1;
		}

		/**
		 * Pop out last value at given index.
		 * @param index the value index.
		 */
		void pop(GLuint index) {
			Node *headi = headi_[index];
			GLboolean valueChanged;

			// apply previously pushed indexed value
			if (headi->prev) {
				valueChanged = (headi->v != headi->prev->v);
				headi = headi->prev;
				headi_[index] = headi;
				lastStampi_ = headi->stamp;
			} else {
				valueChanged = GL_FALSE;
				isEmptyi_[index] = GL_TRUE;
				lastStampi_ = -1;
			}

			GLint lastStampi = (isEmptyi_[index] ? -1 : headi->stamp);
			// reset to equation with latest stamp
			if (head_->stamp > lastStampi) {
				if (headi->v != head_->v) applyi_(index, head_->v);
			} else if (valueChanged) { applyi_(index, headi->v); }

			// count number of pushes for value stamps
			counter_.x -= 1;
			counter_.y -= 1;
		}

		/**
		 * Lock this stack. Until locked push/pop is ignored.
		 */
		void lock() {
			++lockCounter_;
			apply_ = regen_lockedValue;
			applyi_ = regen_lockedIndexed;
		}

		/**
		 * Unlock previously locked stack.
		 */
		void unlock() {
			--lockCounter_;
			if (lockCounter_ < 1) {
				apply_ = lockedApply_;
				applyi_ = lockedApplyi_;
			}
		}

		/**
		 * @return true if the stack is locked.
		 */
		GLboolean isLocked() const {
			return lockCounter_ > 0;
		}

	protected:
		struct Node {
			Node()
					: prev(NULL), next(NULL), stamp(-1) {}

			explicit Node(const ValueType &_v)
					: prev(NULL), next(NULL), v(_v), stamp(-1) {}

			Node(const ValueType &_v, Node *_prev)
					: prev(_prev), next(NULL), v(_v), stamp(-1) {
				_prev->next = this;
			}

			Node *prev;
			Node *next;
			ValueType v;
			GLint stamp;
		};

		void deleteNodes(Node *n_) {
			Stack<Node *> s;
			Node *n = n_;
			while (n->prev) n = n->prev;
			for (; n != NULL; n = n->next) { s.push(n); }
			while (!s.isEmpty()) {
				Node *x = s.top();
				delete x;
				s.pop();
			}
		}

		// Number of indices.
		GLuint numIndices_;
		// A stack containing stamped values applied to all indices.
		Node *head_;
		// A stack array containing stamped values applied to individual indices.
		Node **headi_;
		// Counts number of pushes to any stack and number of pushes to indexed stacks only.
		Vec2ui counter_;

		ValueType zeroValue_;

		// Function to apply the value to all indices.
		ApplyValue apply_;
		// Function to apply the value to a single index.
		ApplyValueIndexed applyi_;
		// Points to actual apply function when the stack is locked.
		ApplyValue lockedApply_;
		// Points to actual apply function when the stack is locked.
		ApplyValueIndexed lockedApplyi_;

		// Counts number of locks.
		GLint lockCounter_;

		GLint lastStampi_;

		GLboolean isEmpty_;
		GLboolean *isEmptyi_;

		typedef void (*DoApplyValue)(IndexedStateStack *, const ValueType &);

		typedef void (*DoApplyValueIndexed)(IndexedStateStack *, GLuint, const ValueType &);

		// use function pointer to avoid some if statements
		DoApplyValue doApply_;
		DoApplyValueIndexed *doApplyi_;

		friend void applyInitStamped<IndexedStateStack, ValueType>
				(IndexedStateStack *, const ValueType &);

		friend void applyFilledStamped<IndexedStateStack, ValueType>
				(IndexedStateStack *, const ValueType &);

		friend void applyInitStampedi<IndexedStateStack, ValueType>
				(IndexedStateStack *, GLuint, const ValueType &);

		friend void applyFilledStampedi<IndexedStateStack, ValueType>
				(IndexedStateStack *, GLuint, const ValueType &);
	};
} // namespace

#endif /* REGEN_STATE_STACKS_H_ */
