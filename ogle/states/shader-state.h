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
#include <ogle/states/light-state.h>
#include <ogle/gl-types/shader.h>

struct ShaderConfig
{
  ShaderConfig() { }
  ShaderConfig(const ShaderConfig &other) {
    functions_ = other.functions_;
    defines_ = other.defines_;
    inputs_ = other.inputs_;
    textures_ = other.textures_;
    feedbackAttributes_ = other.feedbackAttributes_;
    feedbackMode_ = other.feedbackMode_;
    feedbackStage_ = other.feedbackStage_;
  }

  map<string,string> functions_;
  map<string,string> defines_;

  map<string, ref_ptr<ShaderInput> > inputs_;

  list<const TextureState*> textures_;

  list<string> feedbackAttributes_;
  GLenum feedbackMode_;
  GLenum feedbackStage_;
};

/**
 * Provides shader program for child states.
 */
class ShaderState : public State
{
public:
  ShaderState(const ref_ptr<Shader> &shader);
  ShaderState();

  GLboolean createShader(const ShaderConfig &cfg, const string &effectName);
  GLboolean createSimple(map<string, string> &shaderConfig, map<GLenum, string> &shaderNames);

  virtual void enable(RenderState*);
  virtual void disable(RenderState*);

  const ref_ptr<Shader>& shader() const;
  void set_shader(ref_ptr<Shader> shader);
protected:
  ref_ptr<Shader> shader_;

  void loadStage(
      const map<string, string> &shaderConfig,
      const string &effectName,
      map<GLenum,string> &code,
      GLenum stage);
};

#endif /* SHADER_NODE_H_ */
