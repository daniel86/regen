/*
 * texture-transfer.cpp
 *
 *  Created on: 12.03.2012
 *      Author: daniel
 */

#include "texel-transfer.h"

#include <ogle/gl-types/texture.h>
#include <ogle/gl-types/shader.h>
#include <ogle/shader/shader-function.h>
#include <ogle/states/texture-state.h>
#include <ogle/utility/string-util.h>

TexelTransfer::TexelTransfer()
: State()
{
  stringstream s;
  s << "transferFunc" << this;
  transferFuncName_ = s.str();
  s.str("");
}

ScalarToAlphaTransfer::ScalarToAlphaTransfer()
: TexelTransfer()
{
  texelFactor_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f(FORMAT_STRING("texelFactor" << this)) );
  texelFactor_->setUniformData( 1.0f );
  joinShaderInput(ref_ptr<ShaderInput>::cast(texelFactor_));

  fillColorPositive_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f(FORMAT_STRING("fillColorPositive" << this)) );
  fillColorPositive_->setUniformData( Vec3f(1.0f, 1.0f, 1.0f) );
  joinShaderInput(ref_ptr<ShaderInput>::cast(fillColorPositive_));

  fillColorNegative_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f(FORMAT_STRING("fillColorNegative" << this)) );
  fillColorNegative_->setUniformData( Vec3f(0.0f, 0.0f, 1.0f) );
  joinShaderInput(ref_ptr<ShaderInput>::cast(fillColorNegative_));
}
void ScalarToAlphaTransfer::addShaderInputs(ShaderFunctions *shader)
{
  shader->addUniform( GLSLUniform( "float", texelFactor_->name()) );
  shader->addUniform( GLSLUniform( "vec3", fillColorPositive_->name()) );
  shader->addUniform( GLSLUniform( "vec3", fillColorNegative_->name()) );
}
string ScalarToAlphaTransfer::transfer() {
  stringstream s;
  s << "vec4 " << transferFuncName_ << "(vec4 v) {" << endl;
  s << "    vec4 outCol;" << endl;
  s << "    float x = in_" << texelFactor_->name() << "*v.r;" << endl;
#if 1
  s << "    outCol.a = min(1.0, x>0 ? x : -x);" << endl;
  s << "    outCol.rgb = ( x>0 ? in_"
      << fillColorPositive_->name() << " : in_"
      << fillColorNegative_->name() << " );" << endl;
#else
  s << "    outCol = vec4(x);" << endl;
#endif
  s << "    return outCol;" << endl;
  s << "}" << endl;
  return s.str();
}

RGBColorfullTransfer::RGBColorfullTransfer()
: TexelTransfer()
{
  texelFactor_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f(FORMAT_STRING("texelFactor" << this)) );
  texelFactor_->setUniformData( 1.0f );
  joinShaderInput(ref_ptr<ShaderInput>::cast(texelFactor_));
}
void RGBColorfullTransfer::addShaderInputs(ShaderFunctions *shader)
{
  shader->addUniform( GLSLUniform("float", texelFactor_->name()) );
}
string RGBColorfullTransfer::transfer() {
  stringstream s;
  s << "vec4 " << transferFuncName_ << "(vec4 v) {" << endl;
  s << "    vec4 outCol;" << endl;
  s << "    outCol.rgb = in_" << texelFactor_->name() << "*v.rgb;" << endl;
  s << "    outCol.a = min(1.0, length(outCol.rgb));" << endl;
  s << "    outCol.b = 0.0;" << endl;
  s << "    if(outCol.r < 0.0) outCol.b += -0.5*outCol.r;" << endl;
  s << "    if(outCol.g < 0.0) outCol.b += -0.5*outCol.g;" << endl;
  s << "    return outCol;" << endl;
  s << "}" << endl;
  return s.str();
}

LevelSetTransfer::LevelSetTransfer()
: TexelTransfer()
{
  texelFactor_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f(FORMAT_STRING("texelFactor" << this)) );
  texelFactor_->setUniformData( 1.0f );
  joinShaderInput(ref_ptr<ShaderInput>::cast(texelFactor_));
}
void LevelSetTransfer::addShaderInputs(ShaderFunctions *shader)
{
  shader->addUniform( GLSLUniform("float", texelFactor_->name()) );
}
string LevelSetTransfer::transfer() {
  stringstream s;
  s << "vec4 " << transferFuncName_ << "(vec4 v) {" << endl;
  s << "    float levelSet = in_" << texelFactor_->name() << "*v.r;" << endl;
  s << "    if( levelSet < 0 ) {" << endl;
  s << "        return vec4(0.0f, 0.0f, 1.0f, 1.0f);" << endl;
  s << "    } else {" << endl;
  s << "        return vec4(0.0f, 0.0f, 0.0f, 0.0f);" << endl;
  s << "    }" << endl;
  s << "}" << endl;
  return s.str();
}

#undef SET_NAME
