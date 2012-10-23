/*
 * shader-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include "shader-state.h"
#include <ogle/shader/shader-manager.h>
#include <ogle/utility/string-util.h>
#include <ogle/utility/gl-error.h>
#include <ogle/states/render-state.h>

ShaderState::ShaderState(ref_ptr<Shader> shader)
: State(),
  shader_(shader)
{
}

ShaderState::ShaderState()
: State()
{
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

///////////

OrthoShaderState::OrthoShaderState()
: ShaderState()
{
}

OrthoShaderState::OrthoShaderState(
    const list<ShaderFunctions> &fragmentFuncs)
: ShaderState()
{
  updateShader(fragmentFuncs);
}

void OrthoShaderState::updateShader(
    const list<ShaderFunctions> &fragmentFuncs)
{
  ShaderFunctions fs, vs;

  vs.addInput(GLSLTransfer(
      "vec3", "in_pos" ) );
  vs.addExport(GLSLExport(
      "gl_Position", "in_projectionMatrix * vec4(in_pos,1.0)"));

  fs.addUniform(GLSLUniform(
      "samplerRect", "in_sceneTexture" ));
  fs.addMainVar(GLSLVariable(
      "vec4", "fragmentColor_", "") );
  fs.addMainVar(GLSLVariable(
      "vec2", "sceneTextureTexco_", "gl_FragCoord.xy") );
  fs.addExport(GLSLExport(
      "defaultColorOutput", "fragmentColor_" ));
  fs.addFragmentOutput(GLSLFragmentOutput(
      "vec4", "defaultColorOutput", GL_COLOR_ATTACHMENT0 ) );
  fs.enableExtension("GL_ARB_texture_rectangle");

  for(list<ShaderFunctions>::const_iterator
      it=fragmentFuncs.begin(); it!=fragmentFuncs.end(); ++it)
  {
    const ShaderFunctions &f = *it;
    fs.operator+=( f );
  }

  map<GLenum, string> stages;
  stages[GL_FRAGMENT_SHADER] =
      ShaderManager::generateSource(fs, GL_FRAGMENT_SHADER, GL_NONE);
  stages[GL_VERTEX_SHADER] =
      ShaderManager::generateSource(vs, GL_VERTEX_SHADER, GL_FRAGMENT_SHADER);

  shader_ = ref_ptr<Shader>::manage(new Shader(stages));
  if(shader_->compile() && shader_->link())
  {
    shader_->setupInputLocations();
  }
}
