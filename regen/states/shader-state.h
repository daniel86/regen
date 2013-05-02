/*
 * shader-node.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef SHADER_NODE_H_
#define SHADER_NODE_H_

#include <regen/states/state.h>
#include <regen/states/texture-state.h>
#include <regen/shading/light-state.h>
#include <regen/gl-types/shader.h>

namespace regen {
/**
 * \brief Binds Shader program to the RenderState.
 */
class ShaderState : public State
{
public:
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
