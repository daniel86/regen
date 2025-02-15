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
#include <regen/animations/input-animation.h>
#include <regen/gl-types/render-state.h>

#define REGEN_INPUT_STATE_CATEGORY "input"

/**
 * Sums up the time differences between invocations.
 */
namespace regen {
	class TimerInput : public ShaderInput1f, public Animation {
	public:
		/**
		 * @param timeScale scale for dt values.
		 * @param name optional timer name.
		 */
		explicit TimerInput(GLfloat timeScale, const std::string &name = "time")
				: ShaderInput1f(name),
				  Animation(false, true),
				  timeScale_(timeScale) {
			setUniformData(0.0f);
		}

		// Override
		void animate(GLdouble dt) override {
			auto mapped = mapClientVertex<float>(ShaderData::READ | ShaderData::WRITE, 0);
			mapped.w = mapped.r + static_cast<float>(dt) * timeScale_;
		}

	private:
		GLfloat timeScale_;
	};
} // namespace

#include <regen/gl-types/shader-input.h>
#include <stack>

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates ShaderInput's.
		 */
		class InputStateProvider : public StateProcessor {
		public:
			template<class T>
			static void setInput(SceneInputNode &input, ShaderInput *shaderInput, unsigned int count) {
				auto v_values = shaderInput->mapClientData<T>(ShaderData::WRITE);
				for (unsigned int i = 0; i < count; ++i) {
					v_values.w[i] = input.getValue<T>("value", T(1));
				}
				for (auto &child : input.getChildren()) {
					if (child->getCategory() == "set") {
						std::list<GLuint> indices = child->getIndexSequence(count);
						ValueGenerator<T> generator(child.get(), indices.size(),
													child->getValue<T>("value", T(1)));
						for (auto it = indices.begin(); it != indices.end(); ++it) {
							v_values.w[*it] = generator.next();
						}
					} else {
						REGEN_WARN("No processor registered for '" << child->getDescription() << "'.");
					}
				}
			}

			static void setInput(SceneInputNode &input, ShaderInput *in, unsigned int count) {
				if (input.getChildren().empty()) return;
				switch (in->dataType()) {
					case GL_FLOAT:
						switch (in->valsPerElement()) {
						case 1: setInput<float>(input, in, count); break;
						case 2: setInput<Vec2f>(input, in, count); break;
						case 3: setInput<Vec3f>(input, in, count); break;
						case 4: setInput<Vec4f>(input, in, count); break;
						}
						break;
					case GL_INT:
						switch (in->valsPerElement()) {
						case 1: setInput<int>(input, in, count); break;
						case 2: setInput<Vec2i>(input, in, count); break;
						case 3: setInput<Vec3i>(input, in, count); break;
						case 4: setInput<Vec4i>(input, in, count); break;
						}
						break;
					case GL_UNSIGNED_INT:
						switch (in->valsPerElement()) {
						case 1: setInput<unsigned int>(input, in, count); break;
						case 2: setInput<Vec2ui>(input, in, count); break;
						case 3: setInput<Vec3ui>(input, in, count); break;
						case 4: setInput<Vec4ui>(input, in, count); break;
						}
						break;
					default:
						REGEN_WARN("No processor registered for '" << in->name() << "'.");
				}
			}

			static int getNumInstances(const ref_ptr<Mesh> &mesh) {
				int num = mesh->inputContainer()->numInstances();
				std::stack<ref_ptr<State>> stack;
				stack.emplace(mesh);
				while (!stack.empty()) {
					auto state = stack.top();
					stack.pop();
					for (auto &joined : state->joined()) {
						stack.push(joined);
					}
					auto *hasInput = dynamic_cast<HasInput *>(state.get());
					if (hasInput != nullptr) {
						num = std::max(num, hasInput->inputContainer()->numInstances());
					}
				}
				return num;
			}

			/**
			 * Processes SceneInput and creates ShaderInput.
			 * @return The ShaderInput created or a null reference on failure.
			 */
			static ref_ptr<ShaderInput> createShaderInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<State> &state) {
				ref_ptr<ShaderInput> in;

				if (input.hasAttribute("state")) {
					// take uniform from state
					ref_ptr<State> s = parser->getState(input.getValue("state"));
					if (s.get() == nullptr) {
						s = parser->getResources()->getTransform(parser, input.getValue("state"));
					}
					if (s.get() == nullptr) {
						parser->getResources()->loadResources(parser, input.getValue("state"));
						s = parser->getState(input.getValue("state"));
						if (s.get() == nullptr) {
							REGEN_WARN("No State found for for '" << input.getDescription() << "'.");
							return {};
						}
					}
					auto in_opt = s->findShaderInput(input.getValue("component"));
					if (!in_opt.has_value() || !in_opt.value().in.get()) {
						REGEN_WARN("No ShaderInput found for for '" << input.getDescription() << "'.");
						return {};
					}
					in = in_opt.value().in;
					if (in->isVertexAttribute()) {
						in->setVertexData(in->numVertices(), nullptr);
						setInput(input, in.get(), in->numVertices());
					} else {
						in->setInstanceData(in->numInstances(), 1, nullptr);
						setInput(input, in.get(), in->numInstances());
					}
				}
				else if (input.hasAttribute("mesh")) {
					auto meshVec = parser->getResources()->getMesh(parser, input.getValue("mesh"));
					if (meshVec.get() == nullptr || meshVec->empty()) {
						REGEN_WARN("No Mesh found for '" << input.getDescription() << "'.");
						return {};
					}
					auto meshIndex = input.getValue<GLuint>("mesh-index", 0);
					ref_ptr<Mesh> mesh = meshVec->at(0);
					if (meshVec->size() > meshIndex) {
						mesh = meshVec->at(meshIndex);
					}
					auto in_opt = mesh->findShaderInput(input.getValue("component"));
					if (!in_opt.has_value() || !in_opt.value().in.get()) {
						REGEN_WARN("No ShaderInput found for for '" << input.getDescription() << "'.");
						return {};
					}
					in = in_opt.value().in;
					auto numInstances = getNumInstances(mesh);
					in->setInstanceData(numInstances, 1, nullptr);
					setInput(input, in.get(), numInstances);
				}
				else if (input.hasAttribute("ubo")) {
					auto ubo = parser->getResources()->getUBO(parser, input.getValue("ubo"));
					if (ubo.get() == nullptr) {
						REGEN_WARN("No UBO found for '" << input.getDescription() << "'.");
						return {};
					}
					in = ref_ptr<UniformBlock>::alloc(input.getValue("name"), ubo);
				}
				else {
					auto type = input.getValue<std::string>("type", "");
					if (type == "time") {
						auto scale = input.getValue<GLfloat>("scale", 1.0f);
						auto timer = ref_ptr<TimerInput>::alloc(scale);
						in = timer;
						timer->startAnimation();
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

				if (in->name() != input.getValue("name")) {
					// TODO: there is a problem with renaming of inputs, as state configurer
					//   uses the name. We can avoid problems in shader generation by adding some macros here manually.
					//   but this case of inserting inputs with different names should be handled better IMO.
					s->shaderDefine(REGEN_STRING("HAS_"<<input.getValue("name")), "TRUE");
				}

				if (input.getValue<bool>("join", true)) {
					if (x == nullptr) {
						ref_ptr<HasInputState> inputState = ref_ptr<HasInputState>::alloc();
						inputState->setInput(in, input.getValue("name"));
						state->joinStates(inputState);
					} else {
						x->setInput(in, input.getValue("name"));
					}
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
					auto values = v->mapClientDataRaw(ShaderData::WRITE);
					auto typedValues = (T*)values.w;
					for (GLuint i = 0; i < count; i += 1) typedValues[i] = defaultValue;
					values.unmap();
					setInput(input, v.get(), count);
				}

				// Load
				for (const auto& n : input.getChildren("animation")) {
					ref_ptr<InputAnimation<U, T> > inputAnimation = ref_ptr<InputAnimation<U, T> >::alloc(v);
					for (const auto &m : n->getChildren("key-frame")) {
						inputAnimation->push_back(
								m->getValue<T>("value", defaultValue),
								m->getValue<GLdouble>("dt", 1.0)
						);
					}
					state->attach(inputAnimation);
					inputAnimation->startAnimation();
				}

				return v;
			}
		};
	}
}

#endif /* REGEN_SCENE_INPUT_H_ */
