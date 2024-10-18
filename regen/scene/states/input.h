/*
 * input.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_INPUT_H_
#define REGEN_SCENE_INPUT_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>
#include <regen/scene/value-generator.h>
#include <regen/scene/resource-manager.h>

#define REGEN_INPUT_STATE_CATEGORY "input"

/**
 * Sums up the time differences between invocations.
 */
class TimerInput : public ShaderInput1f, Animation {
public:
	/**
	 * @param timeScale scale for dt values.
	 * @param name optional timer name.
	 */
	TimerInput(GLfloat timeScale, const string &name = "time")
			: ShaderInput1f(name),
			  Animation(GL_TRUE, GL_FALSE, GL_TRUE),
			  timeScale_(timeScale) {
		setUniformData(0.0f);
	}

	// Override
	void glAnimate(RenderState *rs, GLdouble dt) override {
		setVertex(0, getVertex(0) + dt * timeScale_);
	}

private:
	GLfloat timeScale_;
};

template<class T>
struct KeyFrame {
	T val;
	GLdouble dt;
};

template<class U, class T>
class KeyFrameAnimation : public Animation {
public:
	explicit KeyFrameAnimation(const ref_ptr<U> &in)
			: Animation(GL_TRUE, GL_TRUE),
			  in_(in) {
		val_ = in_->getVertex(0);
		it_ = frames_.end();
		lastFrame_.val = val_;
		lastFrame_.dt = 0.0;
		dt_ = 0.0;
	}

	void push_back(const T &value, GLdouble dt) {
		KeyFrame<T> f;
		f.val = value;
		f.dt = dt;
		frames_.push_back(f);
		if (frames_.size() == 1) {
			it_ = frames_.begin();
		}
	}

	// Override
	void animate(GLdouble dt) override {
		if (it_ == frames_.end()) {
			it_ = frames_.begin();
			dt_ = 0.0;
		}
		KeyFrame<T> &currentFrame = *it_;

		dt_ += dt / 1000.0;
		if (dt_ >= currentFrame.dt) {
			++it_;
			lastFrame_ = currentFrame;
			GLdouble dt__ = dt_ - currentFrame.dt;
			dt_ = 0.0;
			animate(dt__);
		} else {
			GLdouble t = currentFrame.dt > 0.0 ? dt_ / currentFrame.dt : 1.0;
			lock();
			{
				val_ = math::mix(lastFrame_.val, currentFrame.val, t);
			}
			unlock();
		}
	}

	void glAnimate(RenderState *rs, GLdouble dt) override {
		lock();
		{
			in_->setVertex(0, val_);
		}
		unlock();
	}

protected:
	ref_ptr<U> in_;
	std::list<KeyFrame<T> > frames_;
	typename std::list<KeyFrame<T> >::iterator it_;
	KeyFrame<T> lastFrame_;
	GLdouble dt_;
	T val_;
};

#include <regen/gl-types/shader-input.h>

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates ShaderInput's.
		 */
		class InputStateProvider : public StateProcessor {
		public:
			/**
			 * Processes SceneInput and creates ShaderInput.
			 * @return The ShaderInput created or a null reference on failure.
			 */
			static ref_ptr<ShaderInput> createShaderInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<State> &state) {
				if (input.hasAttribute("state")) {
					// take uniform from state
					ref_ptr<State> s = parser->getState(input.getValue("state"));
					if (s.get() == nullptr) {
						parser->getResources()->loadResources(parser, input.getValue("state"));
						s = parser->getState(input.getValue("state"));
						if (state.get() == nullptr) {
							REGEN_WARN("No State found for for '" << input.getDescription() << "'.");
							return {};
						}
					}
					ref_ptr<ShaderInput> ret = s->findShaderInput(input.getValue("component"));
					if (ret.get() == nullptr) {
						REGEN_WARN("No ShaderInput found for for '" << input.getDescription() << "'.");
					}
					return ret;
				}

				ref_ptr<ShaderInput> in;
				const string type = input.getValue<string>("type", "");
				if (type == "time") {
					auto scale = input.getValue<GLfloat>("scale", 1.0f);
					in = ref_ptr<TimerInput>::alloc(scale);
				} else if (type == "int") {
					in = createShaderInputTyped<ShaderInput1i, GLint>(parser, input, state, GLint(0));
				} else if (type == "ivec2") {
					in = createShaderInputTyped<ShaderInput2i, Vec2i>(parser, input, state, Vec2i(0));
				} else if (type == "ivec3") {
					in = createShaderInputTyped<ShaderInput3i, Vec3i>(parser, input, state, Vec3i(0));
				} else if (type == "ivec4") {
					in = createShaderInputTyped<ShaderInput4i, Vec4i>(parser, input, state, Vec4i(0));
				} else if (type == "uint") {
					in = createShaderInputTyped<ShaderInput1ui, GLuint>(parser, input, state, GLuint(0));
				} else if (type == "uvec2") {
					in = createShaderInputTyped<ShaderInput2ui, Vec2ui>(parser, input, state, Vec2ui(0));
				} else if (type == "uvec3") {
					in = createShaderInputTyped<ShaderInput3ui, Vec3ui>(parser, input, state, Vec3ui(0));
				} else if (type == "uvec4") {
					in = createShaderInputTyped<ShaderInput4ui, Vec4ui>(parser, input, state, Vec4ui(0));
				} else if (type == "float") {
					in = createShaderInputTyped<ShaderInput1f, GLfloat>(parser, input, state, GLfloat(0));
				} else if (type == "vec2") {
					in = createShaderInputTyped<ShaderInput2f, Vec2f>(parser, input, state, Vec2f(0));
				} else if (type == "vec3") {
					in = createShaderInputTyped<ShaderInput3f, Vec3f>(parser, input, state, Vec3f(0));
				} else if (type == "vec4") {
					in = createShaderInputTyped<ShaderInput4f, Vec4f>(parser, input, state, Vec4f(0));
				} else if (type == "mat3") {
					in = createShaderInputTyped<ShaderInputMat3, Mat3f>(parser, input, state, Mat3f::identity());
				} else if (type == "mat4") {
					in = createShaderInputTyped<ShaderInputMat4, Mat4f>(parser, input, state, Mat4f::identity());
				} else {
					REGEN_WARN("Unknown input type '" << type << "'.");
				}
				return in;
			}

			InputStateProvider()
					: StateProcessor(REGEN_INPUT_STATE_CATEGORY) {}

			// Override
			void processInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<State> &state) override {
				ref_ptr<ShaderInput> in = createShaderInput(parser, input, state);
				if (in.get() == nullptr) {
					REGEN_WARN("Failed to create input for " << input.getDescription() << ".");
					return;
				}

				ref_ptr<State> s = state;
				while (!s->joined().empty()) {
					s = *s->joined().rbegin();
				}
				auto *x = dynamic_cast<HasInput *>(s.get());

				if (x == nullptr) {
					ref_ptr<HasInputState> inputState = ref_ptr<HasInputState>::alloc();
					inputState->setInput(in, input.getValue("name"));
					state->joinStates(inputState);
				} else {
					x->setInput(in, input.getValue("name"));
				}


			}

			template<class U, class T>
			static ref_ptr<U> createShaderInputTyped(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<State> &state,
					const T &defaultValue) {
				if (!input.hasAttribute("name")) {
					REGEN_WARN("No name specified for " << input.getDescription() << ".");
					return ref_ptr<U>();
				}
				ref_ptr<U> v = ref_ptr<U>::alloc(input.getValue("name"));
				v->set_isConstant(input.getValue<bool>("is-constant", false));

				auto numInstances = input.getValue<GLuint>("num-instances", 1u);
				auto numVertices = input.getValue<GLuint>("num-vertices", 1u);
				bool isInstanced = input.getValue<bool>("is-instanced", false);
				bool isAttribute = input.getValue<bool>("is-attribute", false);
				GLuint count = 1;

				if (isInstanced) {
					v->setInstanceData(numInstances, 1, nullptr);
					count = numInstances;
				} else if (isAttribute) {
					v->setVertexData(numVertices, nullptr);
					count = numVertices;
				} else {
					v->setUniformData(input.getValue<T>("value", defaultValue));
				}

				// Handle Attribute values.
				if (isInstanced || isAttribute) {
					T *values = (T *) v->clientDataPtr();
					for (GLuint i = 0; i < count; i += 1) values[i] = defaultValue;

					const list<ref_ptr<SceneInputNode> > &childs = input.getChildren();
					for (auto it = childs.begin(); it != childs.end(); ++it) {
						ref_ptr<SceneInputNode> child = *it;
						list<GLuint> indices = child->getIndexSequence(count);

						if (child->getCategory() == "set") {
							ValueGenerator<T> generator(child.get(), indices.size(),
														child->getValue<T>("value", T(0)));
							for (auto it = indices.begin(); it != indices.end(); ++it) {
								values[*it] += generator.next();
							}
						} else {
							REGEN_WARN("No processor registered for '" << child->getDescription() << "'.");
						}
					}
				}

				// Load animations
				const list<ref_ptr<SceneInputNode> > &c0 = input.getChildren("key-frame-animation");
				for (auto it = c0.begin(); it != c0.end(); ++it) {
					ref_ptr<SceneInputNode> n = *it;
					ref_ptr<KeyFrameAnimation<U, T> > inputAnimation = ref_ptr<KeyFrameAnimation<U, T> >::alloc(v);

					const list<ref_ptr<SceneInputNode> > &c1 = n->getChildren("key-frame");
					for (auto jt = c1.begin(); jt != c1.end(); ++jt) {
						ref_ptr<SceneInputNode> m = *jt;
						inputAnimation->push_back(
								m->getValue<T>("value", defaultValue),
								m->getValue<GLdouble>("dt", 1.0)
						);
					}

					state->attach(inputAnimation);
				}

				return v;
			}
		};
	}
}

#endif /* REGEN_SCENE_INPUT_H_ */
