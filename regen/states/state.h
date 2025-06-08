/*
 * state.h
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#ifndef STATE_H_
#define STATE_H_

#include <set>

#include <regen/utility/event-object.h>
#include <regen/utility/ref-ptr.h>
#include <regen/gl-types/shader-input.h>
#include <regen/gl-types/input-container.h>
#include <regen/gl-types/render-state.h>
#include "regen/gl-types/ubo.h"

namespace regen {
	struct StateInput {
		ref_ptr<ShaderInput> in;
		ref_ptr<BufferBlock> block;
		ref_ptr<InputContainer> container;
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

		~State() override = default;

		/**
		 * @return flag indicating if this state is hidden.
		 */
		inline bool isHidden() const { return isHidden_; }

		/**
		 * @param v flag indicating if this state is hidden.
		 */
		void set_isHidden(bool v) { isHidden_ = v; }

		/**
		 * @return joined states.
		 */
		const std::list<ref_ptr<State> > &joined() const;

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
		 * Add a shader input state to the front of the list of joined states.
		 * @param in the shader input data.
		 * @param name optional name overwrite.
		 */
		void joinShaderInput(const ref_ptr<ShaderInput> &in, const std::string &name = "");

		/**
		 * Remove a state from the list of joined states.
		 * @param state a previously joined state.
		 */
		void disjoinStates(const ref_ptr<State> &state);

		/**
		 * Remove a shader input state from the list of joined states.
		 * @param in a previously joined state.
		 */
		void disjoinShaderInput(const ref_ptr<ShaderInput> &in);

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
		const std::map<std::string, std::string> &shaderDefines() const;

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
		const std::map<std::string, std::string> &shaderFunctions() const;

		/**
		 * @return the minimum GLSL version.
		 */
		GLuint shaderVersion() const;

		/**
		 * @param version the minimum GLSL version.
		 */
		void setShaderVersion(GLuint version);

		/**
		 * For all joined states and this state collect all
		 * uniform states and set the constant.
		 */
		void setConstantUniforms(GLboolean isConstant = GL_TRUE);

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
		std::map<std::string, std::string> shaderDefines_;
		std::vector<std::string> shaderIncludes_;
		std::map<std::string, std::string> shaderFunctions_;

		std::list<ref_ptr<State> > joined_;
		std::list<ref_ptr<EventObject> > attached_;
		ref_ptr<HasInput> inputStateBuddy_;
		bool isHidden_ = false;
		GLuint shaderVersion_;
	};
} // namespace

namespace regen {
	/**
	 * \brief A state with an input container.
	 */
	class HasInputState : public State, public HasInput {
	public:
		/**
		 * @param usage the buffer object usage.
		 */
		explicit HasInputState(
			BufferTarget target = ARRAY_BUFFER,
			BufferUsage usage = BUFFER_USAGE_DYNAMIC_DRAW) : State(), HasInput(target, usage) {}
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
		const ref_ptr<State> &globalState() const;

		// override
		void enable(RenderState *) override;

		void disable(RenderState *) override;

	protected:
		ref_ptr<State> globalState_;
	};
} // namespace

namespace regen {
	/**
	 * \brief interface for resizable objects.
	 */
	class Resizable {
	public:
		virtual ~Resizable() = default;

		/**
		 * Resize buffers / textures.
		 */
		virtual void resize() = 0;
	};
} // namespace

#endif /* STATE_H_ */
