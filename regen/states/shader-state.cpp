/*
 * shader-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>

#include <regen/utility/string-util.h>
#include <regen/shading/light-state.h>
#include <regen/states/material-state.h>
#include <regen/states/texture-state.h>
#include <regen/gl-types/gl-util.h>
#include <regen/gl-types/glsl-io-processor.h>
#include <regen/gl-types/glsl-directive-processor.h>
#include <regen/gl-types/gl-enum.h>

#include "shader-state.h"
using namespace regen;

ShaderState::ShaderState(const ref_ptr<Shader> &shader)
: State(), shader_(shader) {}

ShaderState::ShaderState()
: State() {}

void ShaderState::loadStage(
    const map<string, string> &shaderConfig,
    const string &effectName,
    map<GLenum,string> &code,
    GLenum stage)
{
  string stageName = glenum::glslStageName(stage);
  string effectKey = REGEN_STRING(effectName << "." << glenum::glslStagePrefix(stage));
  string ignoreKey = REGEN_STRING("IGNORE_" << stageName);

  map<string, string>::const_iterator it = shaderConfig.find(ignoreKey);
  if(it!=shaderConfig.end() && it->second=="TRUE") { return; }

  code[stage] = GLSLDirectiveProcessor::include(effectKey);
  // failed to include ?
  if(code[stage].empty()) { code.erase(stage); }
}

GLboolean ShaderState::createShader(const StateConfig &cfg, const string &shaderKey)
{
  const map<string, ref_ptr<ShaderInput> > specifiedInput = cfg.inputs_;
  const map<string, ref_ptr<Texture> > &textures = cfg.textures_;
  const map<string, string> &shaderConfig = cfg.defines_;
  const map<string, string> &shaderFunctions = cfg.functions_;
  map<GLenum,string> code;

  for(GLint i=0; i<glenum::glslStageCount(); ++i) {
    loadStage(shaderConfig, shaderKey, code, glenum::glslStages()[i]);
  }

  ref_ptr<Shader> shader = Shader::create(
      cfg.version(),
      shaderConfig,
      shaderFunctions,
      specifiedInput,
      code);
  // setup transform feedback attributes
  shader->setTransformFeedback(cfg.feedbackAttributes_, cfg.feedbackMode_, cfg.feedbackStage_);

  if(!shader->compile()) {
    REGEN_ERROR("Shader '" << shaderKey << "' failed to compiled.");
    return GL_FALSE;
  }
  if(!shader->link()) {
    REGEN_ERROR("Shader '" << shaderKey << "' failed to link.");
  }

  shader->setInputs(specifiedInput);
  for(map<string, ref_ptr<Texture> >::const_iterator
      it=textures.begin(); it!=textures.end(); ++it)
  {
    shader->setTexture(it->second, it->first);
  }

  shader_ = shader;

  REGEN_INFO("Shader '" << shaderKey << "' compiled.");

  return GL_TRUE;
}

const ref_ptr<Shader>& ShaderState::shader() const
{
  return shader_;
}
void ShaderState::set_shader(ref_ptr<Shader> shader)
{
  shader_ = shader;
}

void ShaderState::enable(RenderState *rs)
{
  rs->shader().push(shader_->id());
  if(!rs->shader().isLocked()) {
    shader_->enable(rs);
  }
  State::enable(rs);
}
void ShaderState::disable(RenderState *rs)
{
  State::disable(rs);
  rs->shader().pop();
}
