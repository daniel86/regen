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

GLboolean ShaderState::createShader(ShaderConfig &cfg, const string &effectName)
{
  const map<string, ref_ptr<ShaderInput> > specifiedInput = cfg.inputs();
  const map<string, string> &shaderConfig = cfg.defines();
  const map<string, string> &shaderFunctions = cfg.functions();
  map<GLenum,string> code;

  code[GL_VERTEX_SHADER] = "#include " + effectName + "." +
      GLSLInputOutputProcessor::getPrefix(GL_VERTEX_SHADER);
  code[GL_FRAGMENT_SHADER] = "#include " + effectName + "." +
      GLSLInputOutputProcessor::getPrefix(GL_FRAGMENT_SHADER);
  //code[GL_GEOMETRY_SHADER] = "#include " + effectName + "." +
  //    GLSLInputOutputProcessor::getPrefix(GL_GEOMETRY_SHADER);
  // create tess shader
  if(shaderConfig.count("HAS_TESSELATION")>0) {
    code[GL_TESS_EVALUATION_SHADER] = "#include " + effectName + "." +
        GLSLInputOutputProcessor::getPrefix(GL_TESS_EVALUATION_SHADER);
    if(cfg.tessCfg().isAdaptive) {
      code[GL_TESS_CONTROL_SHADER] = "#include " + effectName + "." +
          GLSLInputOutputProcessor::getPrefix(GL_TESS_CONTROL_SHADER);
    }
  }

  ref_ptr<Shader> shader = Shader::create(shaderConfig,shaderFunctions,specifiedInput,code);

  // setup shader outputs
  const list<ShaderOutput> &outputs = cfg.outputs();
  shader->setOutputs(outputs);

  // setup transform feedback attributes
  const list< ref_ptr<VertexAttribute> > &tranformFeedbackAtts = cfg.transformFeedbackAttributes();
  list<string> transformFeedback;
  for(list< ref_ptr<VertexAttribute> >::const_iterator
      it=tranformFeedbackAtts.begin(); it!=tranformFeedbackAtts.end(); ++it)
  {
    transformFeedback.push_back((*it)->name());
  }
  shader->setTransformFeedback(transformFeedback, GL_SEPARATE_ATTRIBS);

  if(!shader->compile()) { return GL_FALSE; }

  if(!shader->link()) { return GL_FALSE; }

  shader->setInputs(specifiedInput);

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

ref_ptr<Shader> ShaderState::shader()
{
  return shader_;
}
void ShaderState::set_shader(ref_ptr<Shader> shader)
{
  shader_ = shader;
}
