/*
 * input.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_INPUT_H_
#define REGEN_SCENE_INPUT_H_

#include <regen/scene/scene-loader.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/scene-processors.h>
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
#include "regen/gl-types/ssbo.h"

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates ShaderInput's.
		 */
		class ShaderInputProcessor : public StateProcessor {
		public:
			template<class T>
			static void setInput(SceneInputNode &input, ShaderInput *shaderInput, unsigned int count) {
				auto v_values = shaderInput->mapClientData<T>(ShaderData::WRITE | ShaderData::READ);
				auto default_value = input.getValue<T>("value", T(0));
				for (unsigned int i = 0; i < count; ++i) {
					v_values.w[i] = default_value;
				}
				for (auto &child: input.getChildren()) {
					if (child->getCategory() == "set") {
						std::list<GLuint> indices = child->getIndexSequence(count);
						auto blendMode = child->getValue<BlendMode>("blend-mode", BLEND_MODE_SRC);
						ValueGenerator<T> generator(child.get(), indices.size(),
													child->getValue<T>("value", T(1)));
						for (unsigned int & indice : indices) {
							switch (blendMode) {
								case BLEND_MODE_ADD:
									v_values.w[indice] = v_values.r[indice] + v_values.w[indice] + generator.next();
									break;
								case BLEND_MODE_MULTIPLY:
									v_values.w[indice] = v_values.r[indice] * v_values.w[indice] * generator.next();
									break;
								default:
									v_values.w[indice] = generator.next();
									break;
							}
						}
					} else {
						REGEN_WARN("No processor registered for '" << child->getDescription() << "'.");
					}
				}
			}

			static void setInput(SceneInputNode &input, ShaderInput *in, unsigned int count) {
				if (input.getChildren().empty()) return;
				switch (in->baseType()) {
					case GL_FLOAT:
						switch (in->valsPerElement()) {
							case 1:
								setInput<float>(input, in, count);
								break;
							case 2:
								setInput<Vec2f>(input, in, count);
								break;
							case 3:
								setInput<Vec3f>(input, in, count);
								break;
							case 4:
								setInput<Vec4f>(input, in, count);
								break;
						}
						break;
					case GL_INT:
						switch (in->valsPerElement()) {
							case 1:
								setInput<int>(input, in, count);
								break;
							case 2:
								setInput<Vec2i>(input, in, count);
								break;
							case 3:
								setInput<Vec3i>(input, in, count);
								break;
							case 4:
								setInput<Vec4i>(input, in, count);
								break;
						}
						break;
					case GL_UNSIGNED_INT:
						switch (in->valsPerElement()) {
							case 1:
								setInput<unsigned int>(input, in, count);
								break;
							case 2:
								setInput<Vec2ui>(input, in, count);
								break;
							case 3:
								setInput<Vec3ui>(input, in, count);
								break;
							case 4:
								setInput<Vec4ui>(input, in, count);
								break;
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
					for (auto &joined: state->joined()) {
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
					scene::SceneLoader *scene,
					SceneInputNode &input,
					const ref_ptr<State> &state) {
				ref_ptr<ShaderInput> in;

				if (input.hasAttribute("state")) {
					// take uniform from state
					ref_ptr<State> s = scene->getState(input.getValue("state"));
					if (s.get() == nullptr) {
						s = scene->getResource<ModelTransformation>(input.getValue("state"));
					}
					if (s.get() == nullptr) {
						scene->loadResources(input.getValue("state"));
						s = scene->getState(input.getValue("state"));
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
				} else if (input.hasAttribute("mesh")) {
					auto meshVec = scene->getResource<MeshVector>(input.getValue("mesh"));
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
					auto block = scene->getResource<BufferBlock>(input.getValue("ubo"));
					if (block.get() == nullptr || !block->isUniformBlock()) {
						REGEN_WARN("No UBO found for '" << input.getDescription() << "'.");
						return {};
					}
					in = ref_ptr<UBO>::dynamicCast(block);
				}
				else if (input.hasAttribute("ssbo")) {
					auto block = scene->getResource<BufferBlock>(input.getValue("ubo"));
					if (block.get() == nullptr || !block->isShaderStorageBlock()) {
						REGEN_WARN("No SSBO found for '" << input.getDescription() << "'.");
						return {};
					}
					in = ref_ptr<SSBO>::dynamicCast(block);
				}
				else {
					auto type = input.getValue<std::string>("type", "");
					if (type == "time") {
						auto scale = input.getValue<GLfloat>("scale", 1.0f);
						auto timer = ref_ptr<TimerInput>::alloc(scale);
						in = timer;
						timer->startAnimation();
					} else if (type == "int") {
						in = createShaderInputTyped<ShaderInput1i, GLint, int>(input, state, GLint(0));
					} else if (type == "ivec2") {
						in = createShaderInputTyped<ShaderInput2i, Vec2i, int>(input, state, Vec2i(0));
					} else if (type == "ivec3") {
						in = createShaderInputTyped<ShaderInput3i, Vec3i, int>(input, state, Vec3i(0));
					} else if (type == "ivec4") {
						in = createShaderInputTyped<ShaderInput4i, Vec4i, int>(input, state, Vec4i(0));
					} else if (type == "uint") {
						in = createShaderInputTyped<ShaderInput1ui, GLuint, unsigned int>(input, state, GLuint(0));
					} else if (type == "uvec2") {
						in = createShaderInputTyped<ShaderInput2ui, Vec2ui, unsigned int>(input, state, Vec2ui(0));
					} else if (type == "uvec3") {
						in = createShaderInputTyped<ShaderInput3ui, Vec3ui, unsigned int>(input, state, Vec3ui(0));
					} else if (type == "uvec4") {
						in = createShaderInputTyped<ShaderInput4ui, Vec4ui, unsigned int>(input, state, Vec4ui(0));
					} else if (type == "float") {
						in = createShaderInputTyped<ShaderInput1f, GLfloat, float>(input, state, GLfloat(0));
					} else if (type == "vec2") {
						in = createShaderInputTyped<ShaderInput2f, Vec2f, float>(input, state, Vec2f(0));
					} else if (type == "vec3") {
						in = createShaderInputTyped<ShaderInput3f, Vec3f, float>(input, state, Vec3f(0));
					} else if (type == "vec4") {
						in = createShaderInputTyped<ShaderInput4f, Vec4f, float>(input, state, Vec4f(0));
					} else if (type == "mat3") {
						in = createShaderInputTyped<ShaderInputMat3, Mat3f, float>(input, state, Mat3f::identity());
					} else if (type == "mat4") {
						in = createShaderInputTyped<ShaderInputMat4, Mat4f, float>(input, state, Mat4f::identity());
					} else {
						REGEN_WARN("Unknown input type '" << type << "'.");
					}
				}

				return in;
			}

			ShaderInputProcessor()
					: StateProcessor(REGEN_INPUT_STATE_CATEGORY) {}

			// Override
			void processInput(
					scene::SceneLoader *scene,
					SceneInputNode &input,
					const ref_ptr<StateNode> &parent,
					const ref_ptr<State> &state) override {
				ref_ptr<ShaderInput> in = createShaderInput(scene, input, state);
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
					s->shaderDefine(REGEN_STRING("HAS_" << input.getValue("name")), "TRUE");
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

			template<class U, class T, typename ValueType>
			static ref_ptr<U> createShaderInputTyped(
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
				// read the gpu-usage flag
				if (input.getValue<std::string>("gpu-usage", "READ") == "WRITE") {
					v->set_gpuUsage(ShaderData::WRITE);
				} else {
					v->set_gpuUsage(ShaderData::READ);
				}

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
					auto typedValues = (T *) values.w;
					for (GLuint i = 0; i < count; i += 1) typedValues[i] = defaultValue;
					values.unmap();
					setInput(input, v.get(), count);
				}

				// Set the schema.
				bool hasMinMax = input.hasAttribute("min") || input.hasAttribute("max");
				auto semantics = input.getValue<InputSchema::Semantics>(
						"schema", InputSchema::Semantics::UNKNOWN);
				if (hasMinMax) {
					auto min = input.getValue<T>("min", T(0));
					auto max = input.getValue<T>("max", T(1));
					auto schema = InputSchema::alloc(semantics);
					for (int i = 0; i < v->valsPerElement(); i += 1) {
						schema->setLimits(i,
							static_cast<float>(((ValueType*)&min)[i]),
							static_cast<float>(((ValueType*)&max)[i]));
					}
					v->setSchema(schema);
				} else if (input.hasAttribute("schema")) {
					v->setSchema(InputSchema::getDefault(semantics));
				}

				// Load animations.
				for (const auto &n: input.getChildren("animation")) {
					ref_ptr<InputAnimation<U, T> > inputAnimation = ref_ptr<InputAnimation<U, T> >::alloc(v);
					for (const auto &m: n->getChildren("key-frame")) {
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
