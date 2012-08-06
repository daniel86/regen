/*
 * utility-shader.cpp
 *
 *  Created on: 06.11.2011
 *      Author: daniel
 */

#include <sstream>

#include "utility-shader.h"

PickShaderVert::PickShaderVert(
    const vector<string> &args)
: ShaderFunctions("pickShader", args)
{
  setMinVersion(140);
  addUniform( GLSLUniform( "samplerBuffer", "instanceBuffer" ));
  enableExtension("GL_EXT_gpu_shader4");

  addInput( GLSLTransfer( "vec3", "v_pos" ) );
  addInput( GLSLTransfer( "mat4", "v_instanceMat" ) );
  addUniform( GLSLUniform( "mat4", "viewProjectionMatrix" ) );
  addUniform( GLSLUniform( "mat4", "projectionMatrix" ) );
  addUniform( GLSLUniform( "mat4", "modelMat" ) );
  addUniform( GLSLUniform( "vec3", "translation" ) );
  addUniform( GLSLUniform( "bool", "useInstanceRotation" ) );
  addUniform( GLSLUniform( "bool", "useInstance" ) );

  addDependencyCode("getPosWorldSpaceWithUniforms",
      ShaderFunctions::posWorldSpaceWithUniforms);
}
string PickShaderVert::code() const
{
  stringstream s;
  s << "void pickShader() {" << endl;
  s << "    mat4 rotMat;" << endl;
  s << "    gl_Position = viewProjectionMatrix * getPosWorldSpaceWithUniforms(rotMat);" << endl;
  s << "}" << endl;
  return s.str();
}

PickShaderFrag::PickShaderFrag(const vector<string> &args)
: ShaderFunctions("pickShader", args)
{
  addUniform( GLSLUniform( "vec3", "pickColor" ) );

  addFragmentOutput( (GLSLFragmentOutput) { "vec4", "defaultColorOutput", GL_COLOR_ATTACHMENT0 } );
}
string PickShaderFrag::code() const
{
  stringstream s;
  s << "void pickShader() {" << endl;
  s << "    defaultColorOutput = vec4(pickColor.r,pickColor.g,pickColor.b,1);" << endl;
  s << "}" << endl;
  return s.str();
}

