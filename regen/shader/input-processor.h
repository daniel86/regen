#ifndef REGEN_SCENE_INPUT_H_
#define REGEN_SCENE_INPUT_H_

#include <regen/scene/scene-loader.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/scene-processors.h>
#include <regen/scene/value-generator.h>
#include <regen/scene/resource-manager.h>
#include <regen/animation/input-animation.h>

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
		explicit TimerInput(float timeScale, const std::string &name = "time")
				: ShaderInput1f(name),
				  Animation(false, true),
				  timeScale_(timeScale) {
			setUniformData(0.0f);
		}

		// Override
		void cpuUpdate(double dt) override {
			auto mapped = mapClientVertex<float>(BUFFER_GPU_READ | BUFFER_GPU_WRITE, 0);
			mapped.w = mapped.r + static_cast<float>(dt) * timeScale_;
		}

	private:
		float timeScale_;
	};
} // namespace

#include "regen/shader/shader-input.h"
#include <stack>
#include "regen/memory/ssbo.h"

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates ShaderInput's.
		 */
		class ShaderInputProcessor : public StateProcessor {
		public:
			template<typename T, typename BaseType>
			static void setInput(SceneInputNode &input, ShaderInput *shaderInput, unsigned int count) {
				auto v_values =  shaderInput->mapClientData<T>(BUFFER_GPU_WRITE);
				auto default_value = input.getValue<T>("value", Vec::create<T>(0));
				for (unsigned int i = 0; i < count; ++i) {
					v_values.w[i] = default_value;
				}
				for (auto &child: input.getChildren()) {
					if (child->getCategory() == "set") {
						std::list<scene::IndexRange> indices = child->getIndexSequence(count);
						auto blendMode = child->getValue<BlendMode>("blend-mode", BLEND_MODE_SRC);

						uint32_t numInstances = 0;
						for (auto &range : indices) {
							numInstances += (range.to - range.from) / range.step + 1;
						}
						ValueGenerator<T> generator(child.get(), numInstances,
													child->getValue<T>("value", Vec::create<T>(1)));
						for (auto &range : indices) {
							for (unsigned int j = range.from; j <= range.to; j = j + range.step) {
								switch (blendMode) {
									case BLEND_MODE_ADD:
										v_values.w[j] = v_values.r[j] + generator.next();
										break;
									case BLEND_MODE_MULTIPLY:
										v_values.w[j] = v_values.r[j] * generator.next();
										break;
									default:
										v_values.w[j] = generator.next();
										break;
								}
							}
						}
					} else if (child->getCategory() == "animation") {
						// handled in processTyped
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
								setInput<float,float>(input, in, count);
								break;
							case 2:
								setInput<Vec2f,float>(input, in, count);
								break;
							case 3:
								setInput<Vec3f,float>(input, in, count);
								break;
							case 4:
								setInput<Vec4f,float>(input, in, count);
								break;
						}
						break;
					case GL_INT:
						switch (in->valsPerElement()) {
							case 1:
								setInput<int,int>(input, in, count);
								break;
							case 2:
								setInput<Vec2i,int>(input, in, count);
								break;
							case 3:
								setInput<Vec3i,int>(input, in, count);
								break;
							case 4:
								setInput<Vec4i,int>(input, in, count);
								break;
						}
						break;
					case GL_UNSIGNED_INT:
						switch (in->valsPerElement()) {
							case 1:
								setInput<uint32_t,uint32_t>(input, in, count);
								break;
							case 2:
								setInput<Vec2ui,uint32_t>(input, in, count);
								break;
							case 3:
								setInput<Vec3ui,uint32_t>(input, in, count);
								break;
							case 4:
								setInput<Vec4ui,uint32_t>(input, in, count);
								break;
						}
						break;
					default:
						REGEN_WARN("No processor registered for '" << in->name() << "'.");
				}
			}

			static int getNumInstances(const ref_ptr<Mesh> &mesh) {
				int num = mesh->numInstances();
				std::stack<ref_ptr<State>> stack;
				stack.emplace(mesh);
				while (!stack.empty()) {
					auto state = stack.top();
					stack.pop();
					for (auto &joined: *state->joined().get()) {
						stack.push(joined);
					}
					num = std::max(num, state->numInstances());
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
					} else if (in->isStagedBuffer()) {
						setInput(input, in.get(), in->numInstances());
					} else {
						in->setInstanceData(in->numInstances(), 1, nullptr);
						setInput(input, in.get(), in->numInstances());
					}
				} else if (input.hasAttribute("mesh")) {
					auto compositeMesh = scene->getResource<CompositeMesh>(input.getValue("mesh"));
					if (compositeMesh.get() == nullptr || compositeMesh->meshes().empty()) {
						REGEN_WARN("No Mesh found for '" << input.getDescription() << "'.");
						return {};
					}
					auto meshIndex = input.getValue<uint32_t>("mesh-index", 0);
					ref_ptr<Mesh> mesh = compositeMesh->meshes().front();
					if (compositeMesh->meshes().size() > meshIndex) {
						mesh = compositeMesh->meshes().at(meshIndex);
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
					if (block.get() == nullptr || !block->isUBO()) {
						REGEN_WARN("No UBO found for '" << input.getDescription() << "'.");
						return {};
					}
					in = ref_ptr<UBO>::dynamicCast(block);
				}
				else if (input.hasAttribute("ssbo")) {
					auto block = scene->getResource<BufferBlock>(input.getValue("ubo"));
					if (block.get() == nullptr || !block->isSSBO()) {
						REGEN_WARN("No SSBO found for '" << input.getDescription() << "'.");
						return {};
					}
					in = ref_ptr<SSBO>::dynamicCast(block);
				}
				else {
					auto type = input.getValue<std::string>("type", "");
					if (type == "time") {
						auto scale = input.getValue<float>("scale", 1.0f);
						auto timer = ref_ptr<TimerInput>::alloc(scale);
						in = timer;
						timer->startAnimation();
					} else if (type == "int") {
						in = createShaderInputTyped<ShaderInput1i, int, int>(input, state, 0);
					} else if (type == "ivec2") {
						in = createShaderInputTyped<ShaderInput2i, Vec2i, int>(input, state, Vec2i::zero());
					} else if (type == "ivec3") {
						in = createShaderInputTyped<ShaderInput3i, Vec3i, int>(input, state, Vec3i::zero());
					} else if (type == "ivec4") {
						in = createShaderInputTyped<ShaderInput4i, Vec4i, int>(input, state, Vec4i::zero());
					} else if (type == "uint") {
						in = createShaderInputTyped<ShaderInput1ui, uint32_t, uint32_t>(input, state, 0u);
					} else if (type == "uvec2") {
						in = createShaderInputTyped<ShaderInput2ui, Vec2ui, uint32_t>(input, state, Vec2ui::zero());
					} else if (type == "uvec3") {
						in = createShaderInputTyped<ShaderInput3ui, Vec3ui, uint32_t>(input, state, Vec3ui::zero());
					} else if (type == "uvec4") {
						in = createShaderInputTyped<ShaderInput4ui, Vec4ui, uint32_t>(input, state, Vec4ui::zero());
					} else if (type == "float") {
						in = createShaderInputTyped<ShaderInput1f, float, float>(input, state, 0.0f);
					} else if (type == "vec2") {
						in = createShaderInputTyped<ShaderInput2f, Vec2f, float>(input, state, Vec2f::zero());
					} else if (type == "vec3") {
						in = createShaderInputTyped<ShaderInput3f, Vec3f, float>(input, state, Vec3f::zero());
					} else if (type == "vec4") {
						in = createShaderInputTyped<ShaderInput4f, Vec4f, float>(input, state, Vec4f::zero());
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

				const uint32_t dataType = in->baseType();
				const uint32_t numComponents = in->valsPerElement();
				if (dataType == GL_INT) {
					if (numComponents == 1) processTyped<ShaderInput1i,int,int>(input, state, in);
					else if (numComponents == 2) processTyped<ShaderInput2i,Vec2i,int>(input, state, in);
					else if (numComponents == 3) processTyped<ShaderInput3i,Vec3i,int>(input, state, in);
					else if (numComponents == 4) processTyped<ShaderInput4i,Vec4i,int>(input, state, in);
				} else if (dataType == GL_UNSIGNED_INT) {
					if (numComponents == 1) processTyped<ShaderInput1ui,uint32_t,uint32_t>(input, state, in);
					else if (numComponents == 2) processTyped<ShaderInput2ui,Vec2ui,unsigned int>(input, state, in);
					else if (numComponents == 3) processTyped<ShaderInput3ui,Vec3ui,unsigned int>(input, state, in);
					else if (numComponents == 4) processTyped<ShaderInput4ui,Vec4ui,unsigned int>(input, state, in);
				} else if (dataType == GL_FLOAT) {
					if (numComponents == 1) processTyped<ShaderInput1f,float,float>(input, state, in);
					else if (numComponents == 2) processTyped<ShaderInput2f,Vec2f,float>(input, state, in);
					else if (numComponents == 3) processTyped<ShaderInput3f,Vec3f,float>(input, state, in);
					else if (numComponents == 4) processTyped<ShaderInput4f,Vec4f,float>(input, state, in);
					else if (numComponents == 9) processTyped<ShaderInputMat3,Mat3f,float>(input, state, in);
					else if (numComponents == 16) processTyped<ShaderInputMat4,Mat4f,float>(input, state, in);
				}

				if (in->name() != input.getValue("name")) {
					state->shaderDefine(REGEN_STRING("HAS_" << input.getValue("name")), "TRUE");
				}

				if (input.getValue<bool>("join", true)) {
					state->setInput(in,
						input.getValue("name"),
						input.getValue<std::string>("member-suffix", ""));
				}
			}

			template<class U, class T, typename ValueType>
			static void processTyped(
					SceneInputNode &input,
					const ref_ptr<State> &state,
					const ref_ptr<ShaderInput> &untyped) {
				ref_ptr<U> v = ref_ptr<U>::dynamicCast(untyped);
				// Load animations.
				for (const auto &n: input.getChildren("animation")) {
					ref_ptr<InputAnimation<U, T> > inputAnimation = ref_ptr<InputAnimation<U, T> >::alloc(v);
					for (const auto &m: n->getChildren("key-frame")) {
						inputAnimation->push_back(
								m->getValue<T>("value", T()),
								m->getValue<double>("dt", 1.0)
						);
					}
					state->attach(inputAnimation);
					inputAnimation->startAnimation();
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
				if (input.hasAttribute("layout")) {
					v->setMemoryLayout(input.getValue<BufferMemoryLayout>("layout", BUFFER_MEMORY_STD430));
				}

				auto numInstances = input.getValue<uint32_t>("num-instances", 1u);
				auto numVertices = input.getValue<uint32_t>("num-vertices", 1u);
				bool isInstanced = input.getValue<bool>("is-instanced", false);
				bool isAttribute = input.getValue<bool>("is-attribute", false);
				uint32_t count = 1;
				// read the gpu-usage flag
				if (input.getValue<std::string>("gpu-usage", "READ") == "WRITE") {
					v->setServerAccessMode(BUFFER_GPU_WRITE);
				} else {
					v->setServerAccessMode(BUFFER_GPU_READ);
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
					auto values = v->template mapClientData<T>(BUFFER_GPU_WRITE);
					auto typedValues = values.w;
					for (uint32_t i = 0; i < count; i += 1) typedValues[i] = defaultValue;
					values.unmap();
					setInput(input, v.get(), count);
				}

				// Set the schema.
				bool hasMinMax = input.hasAttribute("min") || input.hasAttribute("max");
				auto semantics = input.getValue<InputSchema::Semantics>(
						"schema", InputSchema::Semantics::UNKNOWN);
				if (hasMinMax) {
					auto min = input.getValue<T>("min", Vec::create<T>(0));
					auto max = input.getValue<T>("max", Vec::create<T>(1));
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

				return v;
			}
		};
	}
}

#endif /* REGEN_SCENE_INPUT_H_ */
