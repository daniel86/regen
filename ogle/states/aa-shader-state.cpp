/*
 * aa-shader-state.cpp
 *
 *  Created on: 28.10.2012
 *      Author: daniel
 */

#include "aa-shader-state.h"

AAShaderState::AAShaderState()
: ShaderState()
{
  map<string, string> shaderConfig_;
  map<GLenum, string> shaderNames_;
  shaderNames_[GL_VERTEX_SHADER] = "fxaa.vs";
  shaderNames_[GL_FRAGMENT_SHADER] = "fxaa.fs";
  shader_ = Shader::create(shaderConfig_, shaderNames_);
}
