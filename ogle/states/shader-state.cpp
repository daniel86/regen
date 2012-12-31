/*
 * shader-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>

#include "shader-state.h"
#include <ogle/utility/string-util.h>
#include <ogle/utility/gl-error.h>
#include <ogle/states/render-state.h>
#include <ogle/states/light-state.h>
#include <ogle/states/material-state.h>
#include <ogle/states/texture-state.h>
#include <ogle/gl-types/glsl-io-processor.h>

ShaderState::ShaderState(ref_ptr<Shader> shader)
: State(),
  shader_(shader)
{
}

ShaderState::ShaderState()
: State()
{
}

GLboolean ShaderState::createShader(const ShaderConfig &cfg, const string &effectName)
{
  const map<string, ref_ptr<ShaderInput> > specifiedInput = cfg.inputs_;
  const list<const TextureState*> &textures = cfg.textures_;
  const map<string, string> &shaderConfig = cfg.defines_;
  const map<string, string> &shaderFunctions = cfg.functions_;
  map<GLenum,string> code;

  code[GL_VERTEX_SHADER] = "#include " + effectName + "." +
      GLSLInputOutputProcessor::getPrefix(GL_VERTEX_SHADER);
  code[GL_FRAGMENT_SHADER] = "#include " + effectName + "." +
      GLSLInputOutputProcessor::getPrefix(GL_FRAGMENT_SHADER);
  if(shaderConfig.count("HAS_GEOMETRY_SHADER")>0) {
    code[GL_GEOMETRY_SHADER] = "#include " + effectName + "." +
        GLSLInputOutputProcessor::getPrefix(GL_GEOMETRY_SHADER);
  }
  // create tess shader
  if(shaderConfig.count("HAS_TESSELATION")>0) {
    code[GL_TESS_EVALUATION_SHADER] = "#include " + effectName + "." +
        GLSLInputOutputProcessor::getPrefix(GL_TESS_EVALUATION_SHADER);
    map<string,string>::const_iterator it = cfg.defines_.find("TESS_IS_ADAPTIVE");
    if(it!=cfg.defines_.end() && it->second=="TRUE") {
      code[GL_TESS_CONTROL_SHADER] = "#include " + effectName + "." +
          GLSLInputOutputProcessor::getPrefix(GL_TESS_CONTROL_SHADER);
    }
  }

  ref_ptr<Shader> shader = Shader::create(shaderConfig,shaderFunctions,specifiedInput,code);
  // setup transform feedback attributes
  shader->setTransformFeedback(cfg.transformFeedbackAttributes_, cfg.transformFeedbackMode_);

  if(!shader->compile()) { return GL_FALSE; }

  if(!shader->link()) { return GL_FALSE; }

  shader->setInputs(specifiedInput);
  for(list<const TextureState*>::const_iterator
      it=textures.begin(); it!=textures.end(); ++it)
  {
    const TextureState *s = *it;
    if(!s->name().empty()) {
      shader->setTexture(s->channelPtr(), s->name());
    }
  }

  shader_ = shader;

  return GL_TRUE;
}

GLboolean ShaderState::createSimple(
    map<string, string> &shaderConfig,
    map<GLenum, string> &shaderNames)
{
  map<string, string> shaderFunctions;
  shader_ = Shader::create(shaderConfig,shaderFunctions,shaderNames);
  return shader_.get() != NULL;
}

void ShaderState::enable(RenderState *state)
{
  state->pushShader(shader_.get());
  State::enable(state);
}

void ShaderState::disable(RenderState *state)
{
  State::disable(state);
  state->popShader();
}

ref_ptr<Shader>& ShaderState::shader()
{
  return shader_;
}
void ShaderState::set_shader(ref_ptr<Shader> shader)
{
  shader_ = shader;
}
