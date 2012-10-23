/*
 * debug-normal.cpp
 *
 *  Created on: 29.07.2012
 *      Author: daniel
 */

#include "debug-normal.h"

#include <ogle/shader/shader-manager.h>
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
    shaderConfig["IS_POINT"] = "1"; break;
  case GS_INPUT_LINES_ADJACENCY:
  case GS_INPUT_LINES:
    shaderConfig["IS_LINE"] = "1"; break;
  case GS_INPUT_TRIANGLES:
  case GS_INPUT_TRIANGLES_ADJACENCY:
    shaderConfig["IS_TRIANGLE"] = "1"; break;
  }
  shaderConfig["NORMAL_LENGTH"] = FORMAT_STRING(normalLength);
  // configuration using macros
  string shaderHeader = ShaderManager::getShaderHeader(shaderConfig);
  map<GLenum,string> shaderNames;
  shaderNames[GL_FRAGMENT_SHADER] = "debug.debugNormal.fs";
  shaderNames[GL_VERTEX_SHADER]   = "debug.debugNormal.vs";
  shaderNames[GL_GEOMETRY_SHADER] = "debug.debugNormal.gs";

  string signature = ShaderManager::getShaderSignature(
      shaderNames, shaderConfig);
  shader_ = ShaderManager::createShaderWithSignarure(
      signature, shaderHeader, shaderNames);
  if(shader_.get()!=NULL) {
    shader_->setupInputs(inputs);
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
