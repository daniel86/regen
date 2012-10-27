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
#include <ogle/gl-types/shader.h>

/**
 * Provides shader program for child states.
 */
class ShaderState : public State
{
public:
  ShaderState(ref_ptr<Shader> shader);
  ShaderState();

  GLboolean createShader(
      const ShaderConfig &cfg,
      const string &effectName);
  GLboolean createShader(
      const ShaderConfig &cfg,
      const string &effectName,
      map<GLenum,string> &body);

  virtual void enable(RenderState*);
  virtual void disable(RenderState*);

  ref_ptr<Shader> shader();
  void set_shader(ref_ptr<Shader> shader);

  virtual string name();

  string shadePropertiesCode(const ShaderConfig &cfg);
  string shadeCode(const ShaderConfig &cfg);

  string modifyTransformationCode(const ShaderConfig &cfg);
  string modifyLightCode(const ShaderConfig &cfg);
  string modifyColorCode(const ShaderConfig &cfg);
  string modifyAlphaCode(const ShaderConfig &cfg);
  string modifyNormalCode(const ShaderConfig &cfg);

  string texelCode(const TextureState *texState);
  string blendCode(const TextureState *texState, const string &src, const string &dst);
protected:
  ref_ptr<Shader> shader_;
};

#endif /* SHADER_NODE_H_ */
