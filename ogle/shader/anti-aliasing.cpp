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

FXAA::FXAA(const Config &cfg)
: ShaderFunctions("fxaa")
{
  addUniform( GLSLUniform( "sampler2D", "in_sceneTexture" ));
  addUniform( GLSLUniform( "vec2", "in_viewport" ));
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
  return
"vec4 fxaa() {\n"
"    // lookup North, South, East, and West neighbors\n"
"    vec3 rgbNW = textureOffset(in_sceneTexture, in_texco, ivec2(-1,-1)).xyz;\n"
"    vec3 rgbNE = textureOffset(in_sceneTexture, in_texco, ivec2( 1,-1)).xyz;\n"
"    vec3 rgbSW = textureOffset(in_sceneTexture, in_texco, ivec2(-1, 1)).xyz;\n"
"    vec3 rgbSE = textureOffset(in_sceneTexture, in_texco, ivec2( 1, 1)).xyz;\n"
"    vec3 rgbM  = texture(in_sceneTexture, in_texco).xyz;\n"
"    // convert rgb into a scalar estimate of luminance for shader logic.\n"
"    float lumaNW = fxaaLuma(rgbNW);\n"
"    float lumaNE = fxaaLuma(rgbNE);\n"
"    float lumaSW = fxaaLuma(rgbSW);\n"
"    float lumaSE = fxaaLuma(rgbSE);\n"
"    float lumaM  = fxaaLuma(rgbM);\n"
"    // find luminance range\n"
"    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));\n"
"    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));\n"
"    float lumaRange = lumaMax - lumaMin;\n"
"    // if difference in local maximum and minimum luma is lower than a threshold\n"
"    // proportional to the maximum local luma, then early exits (no visible aliasing)\n"
"    if(lumaRange < max(in_fxaaEdgeThresholdMin, lumaMax*in_fxaaEdgeThreshold))\n"
"    {\n"
"        return vec4(rgbM,1.0);\n"
"    }\n"
"    vec2 dir;\n"
"    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));\n"
"    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));\n"
"\n"
"    float dirReduce = max((lumaNW+lumaNE+lumaSW+lumaSE)*(0.25*in_fxaaReduceMul), in_fxaaReduceMin);\n"
"    float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);\n"
"    dir = min(vec2(in_fxaaSpanMax), max(vec2(-in_fxaaSpanMax), dir*rcpDirMin))/in_viewport.xy;\n"
"\n"
"    vec3 rgbA = 0.5 * (\n"
"        texture(in_sceneTexture, in_texco + dir * (1.0/3.0 - 0.5)).xyz +\n"
"        texture(in_sceneTexture, in_texco + dir * (2.0/3.0 - 0.5)).xyz);\n"
"    vec3 rgbB = rgbA * 0.5 + 0.25 * (\n"
"        texture(in_sceneTexture, in_texco - dir * 0.5).xyz +\n"
"        texture(in_sceneTexture, in_texco + dir * 0.5).xyz);\n"
"\n"
"    float lumaB = dot(rgbB, vec3(0.299, 0.587, 0.114));\n"
"    if((lumaB < lumaMin) || (lumaB > lumaMax)) {\n"
"        return vec4(rgbA,1.0);\n"
"    } else {\n"
"        return vec4(rgbB,1.0);\n"
"    }\n"
"}\n";
}
