/*
 * debugging-shader.cpp
 *
 *  Created on: 06.11.2011
 *      Author: daniel
 */


#include <sstream>

#include "debugging-shader.h"

VisualizeDepthShaderVert::VisualizeDepthShaderVert(const vector<string> &args)
: ShaderFunctions("visualizeDepthShader", args)
{
  addInput( GLSLTransfer( "vec3", "v_pos" ));
  addOutput( GLSLTransfer( "vec2", "f_uv0" ) );
}
string VisualizeDepthShaderVert::code() const
{
  stringstream s;
  s << "void visualizeDepthShader() {" << endl;
  s << "    uvVarying0 = (vec4(0.5)*vec4(pos,1.0) + vec4(0.5)).xy;" << endl;
  s << "    gl_Position = vec4(pos,1.0);" << endl;
  s << "}" << endl;
  return s.str();
}

VisualizeDepthShaderFrag::VisualizeDepthShaderFrag(const vector<string> &args)
: ShaderFunctions("visualizeDepthShader", args)
{
  addInput( GLSLTransfer( "vec2", "f_uv0" ) );
  addUniform( GLSLUniform( "float", "layer" ) );
  addUniform( GLSLUniform( "sampler2DArray", "tex" ) );
  addFragmentOutput( (GLSLFragmentOutput) {
    "vec4", "defaultColorOutput", GL_COLOR_ATTACHMENT0 } );
  setMinVersion(130);
  enableExtension("GL_EXT_texture_array");
}
string VisualizeDepthShaderFrag::code() const
{
  stringstream s;
  s << "void visualizeDepthShader() {" << endl;
  s << "    vec4 tex_coord = vec4(uvVarying0.x, uvVarying0.y, layer, 1.0);" << endl;
  s << "    defaultColorOutput = texture2DArray(tex, tex_coord.xyz);" << endl;
  s << "}" << endl;
  return s.str();
}

