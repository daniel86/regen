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
  handleGLError("before ShaderState::enable");
  state->pushShader(shader_.get());
  handleGLError("after ShaderState::pushShader");
  State::enable(state);
  handleGLError("after ShaderState::enable");
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
{;
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

  shader_ = ref_ptr<Shader>::manage(new Shader);
  map<GLenum, string> stages;
  stages[GL_FRAGMENT_SHADER] =
      ShaderManager::generateSource(fs, GL_FRAGMENT_SHADER, GL_NONE);
  stages[GL_VERTEX_SHADER] =
      ShaderManager::generateSource(vs, GL_VERTEX_SHADER, GL_FRAGMENT_SHADER);
  if(shader_->compile(stages) && shader_->link())
  {
    set<string> attributeNames, uniformNames;
    for(list<GLSLTransfer>::const_iterator
        it=vs.inputs().begin(); it!=vs.inputs().end(); ++it)
    {
      attributeNames.insert(it->name);
    }
    for(list<GLSLUniform>::const_iterator
        it=vs.uniforms().begin(); it!=vs.uniforms().end(); ++it)
    {
      uniformNames.insert(it->name);
    }
    for(list<GLSLUniform>::const_iterator
        it=fs.uniforms().begin(); it!=fs.uniforms().end(); ++it)
    {
      uniformNames.insert(it->name);
    }
    shader_->setupLocations(attributeNames, uniformNames);
  }
}
