#ifndef REGEN_STATE_H_
#define REGEN_STATE_H_

#include <set>

#include <regen/utility/event-object.h>
#include <regen/utility/ref-ptr.h>
#include "regen/shader/shader-input.h"
#include <regen/gl/render-state.h>
#include "regen/memory/ubo.h"

namespace regen {
	struct StateInput {
		ref_ptr<ShaderInput> in;
		ref_ptr<BufferBlock> block;
	};

	/**
	 * \brief Base class for states.
	 *
	 * Joined states are enabled one after each other
	 * and then disabled in reverse order.
	 */
	class State : public EventObject, public Resource {
	public:
		State();

		/**
		 * Copy constructor.
		 * @param other another state.
		 */
		explicit State(const ref_ptr<State> &other);

		~State() override;

		/**
		 * @return flag indicating if this state is hidden.
		 */
		bool isHidden() const noexcept { return isHidden_.load(std::memory_order_relaxed); }

		/**
		 * @param v flag indicating if this state is hidden.
		 */
		void set_isHidden(bool v) noexcept { isHidden_.store(v, std::memory_order_relaxed); }

		/**
		 * @return joined states.
		 */
		ref_ptr<std::vector<ref_ptr<State>>> joined() const { return joined_; }

		/**
		 * Add a state to the end of the list of joined states.
		 * @param state a state.
		 */
		void joinStates(const ref_ptr<State> &state);

		/**
		 * Add a state to the front of the list of joined states.
		 * @param state a state.
		 */
		void joinStatesFront(const ref_ptr<State> &state);

		/**
		 * Remove a state from the list of joined states.
		 * @param state a previously joined state.
		 */
		bool disjoinStates(const ref_ptr<State> &state);

		/**
		 * @return Previously added shader inputs.
		 */
		const std::vector<NamedShaderInput> &inputs() const { return inputs_; }

		/**
		 * @param name the shader input name.
		 * @return true if an input data with given name was added before.
		 */
		bool hasInput(const std::string &name) const;

		/**
		 * @param name the shader input name.
		 * @return input data with specified name.
		 */
		ref_ptr<ShaderInput> getInput(const std::string &name) const;

		/**
		 * @param in the shader input data.
		 * @param name the shader input name.
		 * @return iterator of data container
		 */
		void setInput(const ref_ptr<ShaderInput> &in,
				const std::string &name = "",
				const std::string &memberSuffix = "");

		/**
		 * Remove previously added shader input.
		 * @param in a previously joined state.
		 */
		void removeInput(const ref_ptr<ShaderInput> &in);

		/**
		 * Remove previously added shader input.
		 * @param name the shader input name.
		 */
		void removeInput(const std::string &name);

		/**
		 * Fins ShaderInput attached to this State and joined states.
		 * @param out The output list.
		 */
		void collectShaderInput(ShaderInputList &out);

		/**
		 * Find named ShaderInput joined into state. First match returns.
		 * @param name ShaderInput name.
		 * @return The ShaderInput if any or a null reference if not found.
		 */
		std::optional<StateInput> findShaderInput(const std::string &name);

		/**
		 * @return Specifies the number of vertices to be rendered.
		 */
		int32_t numVertices() const;

		/**
		 * @param v Specifies the number of vertices to be rendered.
		 */
		void set_numVertices(int32_t v);

		/**
		 * @return Number of instances of added input data.
		 */
		int32_t numInstances() const;

		/**
		 * @param v Specifies the number of instances to be rendered.
		 */
		void set_numInstances(int32_t v);

		/**
		 * Defines a GLSL macro.
		 * @param name the macro key.
		 * @param value the macro value.
		 */
		void shaderDefine(const std::string &name, const std::string &value);

		/**
		 * Undefine a GLSL macro, i.e. removing it from the shader.
		 * @param name the macro key.
		 */
		void shaderUndefine(const std::string &name);

		/**
		 * @return GLSL macros.
		 */
		const std::map<std::string, std::string> &shaderDefines() const { return shaderDefines_; }

		/**
		 * Adds a GLSL include to generated shaders.
		 * @param name the include name.
		 */
		void shaderInclude(const std::string &name);

		/**
		 * @return GLSL includes.
		 */
		const std::vector<std::string> &shaderIncludes() const { return shaderIncludes_; }

		/**
		 * Adds a GLSL function to generated shaders.
		 * @param name the function name.
		 * @param value the GLSL code.
		 */
		void shaderFunction(const std::string &name, const std::string &value);

		/**
		 * @return GLSL functions.
		 */
		const std::map<std::string, std::string> &shaderFunctions() const { return shaderFunctions_; }

		/**
		 * @return the minimum GLSL version.
		 */
		uint32_t shaderVersion() const { return shaderVersion_; }

		/**
		 * @param version the minimum GLSL version.
		 */
		void setShaderVersion(uint32_t version) { shaderVersion_ = std::max(shaderVersion_, version); }

		/**
		 * For all joined states and this state collect all
		 * uniform states and set the constant.
		 */
		void setConstantUniforms(bool isConstant = true);

		/**
		 * Activate state in given RenderState.
		 * @param rs the render state.
		 */
		virtual void enable(RenderState *rs);

		/**
		 * Deactivate state in given RenderState.
		 * @param rs the render state.
		 */
		virtual void disable(RenderState *rs);

		/**
		 * Keep a reference on event object.
		 */
		void attach(const ref_ptr<EventObject> &obj);

	protected:
		ref_ptr<std::vector<ref_ptr<State>>> joined_;
		std::vector<ref_ptr<EventObject> > attached_;
		std::atomic<bool> isHidden_{false};

		// shader inputs
		// note: the inputs are not share between copies.
		// e.g. a basic mesh sets up inputs such as vertex attributes, and copies of the mesh
		// may add additional specialized inputs for their respective shaders.
		std::vector<NamedShaderInput> inputs_;
		std::set<std::string> inputMap_;

		std::map<std::string, std::string> shaderDefines_;
		std::vector<std::string> shaderIncludes_;
		std::map<std::string, std::string> shaderFunctions_;
		uint32_t shaderVersion_ = 330;

		template<typename F> void updateVector(F &&fn) {
			// Make a copy, modify, then atomically swap
			auto newVec = ref_ptr<std::vector<ref_ptr<State>>>::alloc(*joined_.get());
			fn(*newVec.get());
			joined_ = newVec;
		}

	private:
		struct StateShared; // forward declaration
		ref_ptr<StateShared> shared_;
	};
} // namespace

namespace regen {
	/**
	 * \brief Joined states of the sequence are enabled and disabled
	 * one after each other.
	 *
	 * Contrary to the State base class where each joined State is enabled
	 * and afterwards each State is disabled.
	 * The sequence also supports a single global state. The global state
	 * is enabled first and disabled last.
	 */
	class StateSequence : public State {
	public:
		StateSequence();

		static ref_ptr<StateSequence> load(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * @param globalState the global state.
		 */
		void set_globalState(const ref_ptr<State> &globalState);

		/**
		 * @return the global state.
		 */
		const ref_ptr<State> &globalState() const  { return globalState_; }

		// override
		void enable(RenderState *) override;

		void disable(RenderState *) override;

	protected:
		ref_ptr<State> globalState_;
	};
} // namespace

#endif /* REGEN_STATE_H_ */
