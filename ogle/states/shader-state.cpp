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

GLboolean ShaderState::createShader(
    ShaderConfig &cfg,
    const string &effectName)
{
  map<GLenum,string> code;
  return createShader(cfg, effectName, code);
}

GLboolean ShaderState::createShader(
    ShaderConfig &cfg,
    const string &effectName,
    map<GLenum,string> &code)
{
  const map<string, ref_ptr<ShaderInput> > specifiedInput = cfg.inputs();
  const map<string, string> &shaderConfig = cfg.defines();

  GLboolean hasTesselation = GL_FALSE;
  if(shaderConfig.count("HAS_TESSELATION")>0) {
    hasTesselation = (shaderConfig.find("HAS_TESSELATION")->second == "TRUE");
  }

  set<GLenum> usedStages;
  usedStages.insert(GL_VERTEX_SHADER);
  //usedStages.push_back(GL_GEOMETRY_SHADER);
  usedStages.insert(GL_FRAGMENT_SHADER);
  if(hasTesselation) {
    usedStages.insert(GL_TESS_EVALUATION_SHADER);
    if(cfg.tessCfg().isAdaptive) {
      usedStages.insert(GL_TESS_CONTROL_SHADER);
    }
  }

  // load effect headers
  for(set<GLenum>::iterator it=usedStages.begin(); it!=usedStages.end(); ++it)
  {
    code[*it] = FORMAT_STRING(
        "#include " << effectName << "." <<
        GLSLInputOutputProcessor::getPrefix(*it) << ".header" << endl << endl << code[*it]);
  }

  // load main functions
  for(set<GLenum>::iterator it=usedStages.begin(); it!=usedStages.end(); ++it)
  {
    code[*it] = FORMAT_STRING(
        code[*it] << endl <<
        "#include " << effectName << "." << GLSLInputOutputProcessor::getPrefix(*it) << ".main" << endl);
  }

  ref_ptr<Shader> shader = Shader::create(shaderConfig, specifiedInput, code);

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
  shader_ = Shader::create(shaderConfig, shaderNames);
  return shader_.get() != NULL;
}

string ShaderState::name()
{
  return FORMAT_STRING("ShaderState(" << shader_->id() << ")");
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
