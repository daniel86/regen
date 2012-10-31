/*
 * debug-normal.cpp
 *
 *  Created on: 29.07.2012
 *      Author: daniel
 */

#include "debug-normal.h"

#include <ogle/utility/string-util.h>
#include <ogle/utility/gl-error.h>

DebugNormal::DebugNormal(
    map<string, ref_ptr<ShaderInput> > &inputs,
    GeometryShaderInput inputPrimitive,
    GLfloat normalLength)
: ShaderState()
{
  map<string,string> shaderConfig;
  switch(inputPrimitive) {
  case GS_INPUT_POINTS:
    shaderConfig["GS_INPUT_PRIMITIVE"] = "points"; break;
  case GS_INPUT_LINES_ADJACENCY:
    shaderConfig["GS_INPUT_PRIMITIVE"] = "lines_adjacency"; break;
  case GS_INPUT_LINES:
    shaderConfig["GS_INPUT_PRIMITIVE"] = "lines"; break;
  case GS_INPUT_TRIANGLES:
    shaderConfig["GS_INPUT_PRIMITIVE"] = "triangles"; break;
  case GS_INPUT_TRIANGLES_ADJACENCY:
    shaderConfig["GS_INPUT_PRIMITIVE"] = "triangles_adjacency"; break;
  }
  shaderConfig["NORMAL_LENGTH"] = FORMAT_STRING(normalLength);
  // configuration using macros
  map<GLenum,string> shaderNames;
  shaderNames[GL_FRAGMENT_SHADER] = "debug-normal.fs";
  shaderNames[GL_VERTEX_SHADER]   = "debug-normal.vs";
  shaderNames[GL_GEOMETRY_SHADER] = "debug-normal.gs";

  shader_ = Shader::create(shaderConfig, inputs, shaderNames);
  if(shader_->compile() && shader_->link()) {
    shader_->setInputs(inputs);
  } else {
    shader_ = ref_ptr<Shader>();
  }
}

string DebugNormal::name()
{
  return FORMAT_STRING("DebugNormal");
}

void DebugNormal::enable(RenderState *state)
{
  glDepthFunc(GL_LEQUAL);
  ShaderState::enable(state);
}

void DebugNormal::disable(RenderState *state)
{
  ShaderState::disable(state);
  glDepthFunc(GL_LESS);
}
