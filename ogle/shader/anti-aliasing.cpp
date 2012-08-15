/*
 * anti-aliasing.cpp
 *
 *  Created on: 29.07.2012
 *      Author: daniel
 */

#include "anti-aliasing.h"

#include <ogle/utility/string-util.h>

static const string fxaaLuma =
#if 1
"float fxaaLuma(vec3 rgb) { return rgb.y * (0.587/0.299) + rgb.x; }";
#else
"float fxaaLuma(vec3 rgb) { return dot(rgb, vec3(0.299, 0.587, 0.114)); }";
#endif

FXAA::FXAA(
    const vector<string> &args,
    const Texture &tex,
    const FXAAConfig &cfg)
: TextureShader("fxaa", args, tex)
{
  // TODO FXAA: there are a lot of implementations around....
  addConstant( GLSLConstant( "float", "in_fxaaSpanMax",
      FORMAT_STRING(cfg.spanMax) ) );
  addConstant( GLSLConstant( "float", "in_fxaaReduceMin",
      FORMAT_STRING(cfg.reduceMin) ) );
  addConstant( GLSLConstant( "float", "in_fxaaReduceMul",
      FORMAT_STRING(cfg.reduceMul) ) );
  addConstant( GLSLConstant( "float", "in_fxaaEdgeThreshold",
      FORMAT_STRING(cfg.edgeThreshold) ) );
  addConstant( GLSLConstant( "float", "in_fxaaEdgeThresholdMin",
      FORMAT_STRING(cfg.edgeThresholdMin) ) );

  enableExtension("GL_EXT_gpu_shader4");
  addDependencyCode("fxaaLuma", fxaaLuma);
}
string FXAA::code() const
{
  stringstream s;
  s << "void fxaa(vec2 texco, "<<samplerType_<<" tex, inout vec4 col)" << endl;
  s << "{" << endl;
  s << "    // lookup North, South, East, and West neighbors" << endl;
  s << "    vec3 rgbNW = textureOffset(tex, texco, ivec2(-1,-1)).xyz;" << endl;
  s << "    vec3 rgbNE = textureOffset(tex, texco, ivec2( 1,-1)).xyz;" << endl;
  s << "    vec3 rgbSW = textureOffset(tex, texco, ivec2(-1, 1)).xyz;" << endl;
  s << "    vec3 rgbSE = textureOffset(tex, texco, ivec2( 1, 1)).xyz;" << endl;
  s << "    vec3 rgbM  = texture(tex, texco).xyz;" << endl;
  s << endl;
  s << "    // convert rgb into a scalar estimate of luminance for shader logic." << endl;
  s << "    float lumaNW = fxaaLuma(rgbNW);" << endl;
  s << "    float lumaNE = fxaaLuma(rgbNE);" << endl;
  s << "    float lumaSW = fxaaLuma(rgbSW);" << endl;
  s << "    float lumaSE = fxaaLuma(rgbSE);" << endl;
  s << "    float lumaM  = fxaaLuma(rgbM);" << endl;
  s << endl;
  s << "    // find luminance range" << endl;
  s << "    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));" << endl;
  s << "    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));" << endl;
  s << "    float lumaRange = lumaMax - lumaMin;" << endl;
  s << "    // if difference in local maximum and minimum luma is lower than a threshold" << endl;
  s << "    // proportional to the maximum local luma, then early exits (no visible aliasing)" << endl;
  s << "    if(lumaRange < max(in_fxaaEdgeThresholdMin, lumaMax*in_fxaaEdgeThreshold))" << endl;
  s << "    {" << endl;
  s << "        col = vec4(rgbM,1.0);" << endl;
  s << "        return;" << endl;
  s << "    }" << endl;
  s << endl;

  s << "    vec2 dir;" << endl;
  s << "    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));" << endl;
  s << "    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));" << endl;
  s << endl;
  s << "    float dirReduce = max((lumaNW+lumaNE+lumaSW+lumaSE)*(0.25*in_fxaaReduceMul), in_fxaaReduceMin);" << endl;
  s << "    float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);" << endl;
  s << "    dir = min(vec2(in_fxaaSpanMax), max(vec2(-in_fxaaSpanMax), dir*rcpDirMin));" << endl;
  s << endl;
  s << "    vec3 rgbA = 0.5 * (" << endl;
  s << "        texture(tex, texco + dir * (1.0/3.0 - 0.5)).xyz +" << endl;
  s << "        texture(tex, texco + dir * (2.0/3.0 - 0.5)).xyz);" << endl;
  s << "    vec3 rgbB = rgbA * 0.5 + 0.25 * (" << endl;
  s << "        texture(tex, texco - dir * 0.5).xyz +" << endl;
  s << "        texture(tex, texco + dir * 0.5).xyz);" << endl;
  s << endl;
  s << "    float lumaB = dot(rgbB, vec3(0.299, 0.587, 0.114));" << endl;
  s << "    if((lumaB < lumaMin) || (lumaB > lumaMax))" << endl;
  s << "        col = vec4(rgbA,1.0);" << endl;
  s << "    else" << endl;
  s << "        col = vec4(rgbB,1.0);" << endl;
  s << "}" << endl;
  return s.str();
}
