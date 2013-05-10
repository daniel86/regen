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
#include <regen/gl-types/texture.h>
#include <regen/gl-types/gl-util.h>
#include <regen/gl-types/shader-input.h>
#include <regen/gl-types/shader-input-container.h>
#include <regen/gl-types/render-state.h>

namespace regen {
  /**
   * \brief Configures a Shader object.
   *
   * Configuration is done using macros.
   * Using the GLSL preprocessors it is also possible to change
   * the type of shader input data.
   * Transform feedback configuration is also handled because
   * the info is needed in advance of linking the shader.
   */
  struct StateConfig {
  public:
    StateConfig() {
      version_ = 130;
#ifdef GL_VERSION_4_0
      defines_["GL_VERSION_4_0"] = "TRUE";
#endif
#ifdef GLEW_ARB_tessellation_shader
      defines_["GL_ARB_tessellation_shader"] = "TRUE";
#endif
    }
    /**
     * Copy constructor.
     */
    StateConfig(const StateConfig &other) {
      functions_ = other.functions_;
      defines_ = other.defines_;
      inputs_ = other.inputs_;
      textures_ = other.textures_;
      feedbackAttributes_ = other.feedbackAttributes_;
      feedbackMode_ = other.feedbackMode_;
      feedbackStage_ = other.feedbackStage_;
      version_ = other.version_;
    }

    /**
     * @param version the GLSL version.
     */
    void setVersion(GLuint version)
    { if(version>version_) version_=version; }
    /**
     * @return the GLSL version.
     */
    GLuint version() const
    { return version_; }

    /**
     * Macro key-value map. Macros are prepended to loaded shaders.
     */
    map<string,string> defines_;
    /**
     * User defined GLSL functions for the shader.
     */
    map<string,string> functions_;
    /**
     * Specified shader input data.
     */
    map<string, ref_ptr<ShaderInput> > inputs_;
    /**
     * Specified shader textures.
     */
    map<string, ref_ptr<Texture> > textures_;
    /**
     * List of attribute names to capture using transform feedback.
     */
    list<string> feedbackAttributes_;
    /**
     * Interleaved or separate ?
     */
    GLenum feedbackMode_;
    /**
     * Capture output of this shader stage.
     */
    GLenum feedbackStage_;

  protected:
    GLuint version_;
  };
} // namespace

namespace regen {
  /**
   * \brief Base class for states.
   *
   * Joined states are enabled one after each other
   * and then disabled in reverse order.
   */
  class State : public EventObject
  {
  public:
    State();
    virtual ~State() {}

    /**
     * @return flag indicating if this state is hidden.
     */
    GLboolean isHidden() const;
    /**
     * @param v flag indicating if this state is hidden.
     */
    void set_isHidden(GLboolean v);

    /**
     * @return joined states.
     */
    const list< ref_ptr<State> >& joined() const;

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
    void joinShaderInput(const ref_ptr<ShaderInput> &in, const string &name="");
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
     * Defines a GLSL macro.
     * @param name the macro key.
     * @param value the macro value.
     */
    void shaderDefine(const string &name, const string &value);
    /**
     * @return GLSL macros.
     */
    const map<string,string>& shaderDefines() const;

    /**
     * Adds a GLSL function to generated shaders.
     * @param name the function name.
     * @param value the GLSL code.
     */
    void shaderFunction(const string &name, const string &value);
    /**
     * @return GLSL functions.
     */
    const map<string,string>& shaderFunctions() const;

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
    void setConstantUniforms(GLboolean isConstant=GL_TRUE);

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

  protected:
    map<string,string> shaderDefines_;
    map<string,string> shaderFunctions_;

    list< ref_ptr<State> > joined_;
    ref_ptr<HasInput> inputStateBuddy_;
    GLboolean isHidden_;
    GLuint shaderVersion_;
  };
} // namespace

namespace regen {
  /**
   * \brief A state with an input container.
   */
  class HasInputState : public State, public HasInput
  {
  public:
    /**
     * @param usage the buffer object usage.
     */
    HasInputState(VBO::Usage usage=VBO::USAGE_DYNAMIC) : State(), HasInput(usage) {}
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
  class StateSequence : public State
  {
  public:
    StateSequence();

    /**
     * @param globalState the global state.
     */
    void set_globalState(const ref_ptr<State> &globalState);
    /**
     * @return the global state.
     */
    const ref_ptr<State>& globalState() const;

    // override
    virtual void enable(RenderState*);
    virtual void disable(RenderState*);

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
    virtual ~Resizable() {}
    /**
     * Resize buffers / textures.
     */
    virtual void resize()=0;
  };
} // namespace

#endif /* STATE_H_ */
