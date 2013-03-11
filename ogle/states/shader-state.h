/*
 * shader-node.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef SHADER_NODE_H_
#define SHADER_NODE_H_

#include <ogle/states/state.h>
#include <ogle/states/texture-state.h>
#include <ogle/shading/light-state.h>
#include <ogle/gl-types/shader.h>

namespace ogle {
/**
 * \brief Binds Shader program to the RenderState.
 */
class ShaderState : public State
{
public:
  /**
   * \brief Configures a Shader object.
   *
   * Configuration is done using macros.
   * Using the GLSL preprocessors it is also possible to change
   * the type of shader input data.
   * Transform feedback configuration is also handled because
   * the info is needed in advance of linking the shader.
   */
  struct Config {
  public:
    Config() {
      version_ = 130;
    }
    Config(const Config &other) {
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
     * Macro key-value map. Macros are prepended - as '#define key value' -
     * to loaded shaders.
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
    list<const TextureState*> textures_;
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

  /**
   * @param shader the shader object.
   */
  ShaderState(const ref_ptr<Shader> &shader);
  ShaderState();

  /**
   * Load, compile and link shader with given include key.
   * @param cfg the shader config.
   * @param shaderKey the shader include key.
   * @return GL_TRUE on success.
   */
  GLboolean createShader(const Config &cfg, const string &shaderKey);

  /**
   * @return the shader object.
   */
  const ref_ptr<Shader>& shader() const;
  /**
   * @param shader the shader object.
   */
  void set_shader(ref_ptr<Shader> shader);

  // overwrite
  void enable(RenderState*);
  void disable(RenderState*);

protected:
  ref_ptr<Shader> shader_;

  void loadStage(
      const map<string, string> &shaderConfig,
      const string &effectName,
      map<GLenum,string> &code,
      GLenum stage);
};

} // namespace

#endif /* SHADER_NODE_H_ */
