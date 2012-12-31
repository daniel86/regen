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
  map<string,string> functions_;
  map<string,string> defines_;

  map<string, ref_ptr<ShaderInput> > inputs_;

  list<const TextureState*> textures_;
  list<const Light*> lights_;

  list< ref_ptr<VertexAttribute> > transformFeedbackAttributes_;
  GLenum transformFeedbackMode_;
};

/**
 * Provides shader program for child states.
 */
class ShaderState : public State
{
public:
  ShaderState(ref_ptr<Shader> shader);
  ShaderState();

  GLboolean createShader(const ShaderConfig &cfg, const string &effectName);
  GLboolean createSimple(map<string, string> &shaderConfig, map<GLenum, string> &shaderNames);

  virtual void enable(RenderState*);
  virtual void disable(RenderState*);

  ref_ptr<Shader>& shader();
  void set_shader(ref_ptr<Shader> shader);
protected:
  ref_ptr<Shader> shader_;
};

#endif /* SHADER_NODE_H_ */
