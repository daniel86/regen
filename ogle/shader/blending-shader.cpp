/*
 * blending.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include <sstream>
#include "blending-shader.h"

static const char* rgbToHsv =
"void rgb_to_hsv(vec4 rgb, out vec4 col2)\n"
"{\n"
"    float cmax, cmin, h, s, v, cdelta;\n"
"    vec3 c;\n"
"\n"
"    cmax = max(rgb[0], max(rgb[1], rgb[2]));\n"
"    cmin = min(rgb[0], min(rgb[1], rgb[2]));\n"
"    cdelta = cmax-cmin;\n"
"\n"
"    v = cmax;\n"
"    if (cmax!=0.0)\n"
"        s = cdelta/cmax;\n"
"    else {\n"
"        s = 0.0;\n"
"        h = 0.0;\n"
"    }\n"
"\n"
"    if (s == 0.0) {\n"
"        h = 0.0;\n"
"    }\n"
"    else {\n"
"        c = (vec3(cmax, cmax, cmax) - rgb.xyz)/cdelta;\n"
"\n"
"        if (rgb.x==cmax) h = c[2] - c[1];\n"
"        else if (rgb.y==cmax) h = 2.0 + c[0] -  c[2];\n"
"        else h = 4.0 + c[1] - c[0];\n"
"\n"
"        h /= 6.0;\n"
"\n"
"        if (h<0.0)\n"
"            h += 1.0;\n"
"    }\n"
"\n"
"    col2 = vec4(h, s, v, rgb.w);\n"
"}\n";

static const char* hsvToRgb =
"void hsv_to_rgb(vec4 hsv, out vec4 col2)\n"
"{\n"
"    float i, f, p, q, t, h, s, v;\n"
"    vec3 rgb;\n"
"\n"
"    h = hsv[0];\n"
"    s = hsv[1];\n"
"    v = hsv[2];\n"
"\n"
"    if(s==0.0) {\n"
"        rgb = vec3(v, v, v);\n"
"    }\n"
"    else {\n"
"        if(h==1.0)\n"
"            h = 0.0;\n"
"\n"
"        h *= 6.0;\n"
"        i = floor(h);\n"
"        f = h - i;\n"
"        rgb = vec3(f, f, f);\n"
"        p = v*(1.0-s);\n"
"        q = v*(1.0-(s*f));\n"
"        t = v*(1.0-(s*(1.0-f)));\n"
"\n"
"        if (i == 0.0) rgb = vec3(v, t, p);\n"
"        else if (i == 1.0) rgb = vec3(q, v, p);\n"
"        else if (i == 2.0) rgb = vec3(p, v, t);\n"
"        else if (i == 3.0) rgb = vec3(p, q, v);\n"
"        else if (i == 4.0) rgb = vec3(t, p, v);\n"
"        else rgb = vec3(v, p, q);"
"    }\n"
"\n"
"    col2 = vec4(rgb, hsv.w);\n"
"}\n";

////////////////

string InvertBlender::code() const
{
  stringstream s;
  s << "void invertShader(inout vec4 col)" << endl;
  s << "{" << endl;
  s << "    col.xyz = vec3(1.0, 1.0, 1.0) - col.xyz;" << endl;
  s << "}" << endl;
  return s.str();
}

BrightnessBlender::BrightnessBlender(const vector<string> &args)
: ShaderFunctions("brightnessBlend", args)
{
}
string BrightnessBlender::code() const
{
  stringstream s;
  s << "void brightnessBlend(inout vec4 col, float factor)" << endl;
  s << "{" << endl;
  s << "    col.xyz = col.xyz * factor;" << endl;
  s << "}" << endl;
  return s.str();
}

ContrastBlender::ContrastBlender(const vector<string> &args)
: ShaderFunctions("contrastBlender", args)
{
}
string ContrastBlender::code() const
{
  stringstream s;
  s << "void contrastBlender(inout vec4 col, float factor)" << endl;
  s << "{" << endl;
  s << "    float buf = factor * 0.5 - 0.5;" << endl;
  s << "    if (col.x > 0.5) {" << endl;
  s << "        col.x = clamp( col.x + buf, 0.5, 1.0);" << endl;
  s << "    } else {" << endl;
  s << "        col.x = max( 0.0, 0.5*(2.0*col.x + 1.0 - factor) );" << endl;
  s << "    }" << endl;
  s << "    if (col.y > 0.5) {" << endl;
  s << "        col.y = clamp( col.y + buf, 0.5, 1.0);" << endl;
  s << "    } else {" << endl;
  s << "        col.y = max( 0.0, 0.5*(2.0*col.y + 1.0 - factor) );" << endl;
  s << "    }" << endl;
  s << "    if (col.z > 0.5) {" << endl;
  s << "        col.z = clamp( col.x + buf, 0.5, 1.0);" << endl;
  s << "    } else {" << endl;
  s << "        col.z = max( 0.0, 0.5*(2.0*col.z + 1.0 - factor) );" << endl;
  s << "    }" << endl;
  s << "}" << endl;
  return s.str();
}

///////////////

TextureBlenderCol2::TextureBlenderCol2(
    const string &name,
    const vector<string> &args)
: ShaderFunctions(name, args)
{
}

AlphaBlender::AlphaBlender(const vector<string> &args)
: TextureBlenderCol2("alphaBlender", args)
{
}
string AlphaBlender::code() const
{
  stringstream s;
  s << "void alphaBlender(vec4 src, inout vec4 dst, float factor)" << endl;
  s << "{" << endl;
  s << "    dst = dst*(1.0 - src.a) + src*src.a;" << endl;
  s << "}" << endl;
  return s.str();
}

FrontToBackBlender::FrontToBackBlender(const vector<string> &args)
: TextureBlenderCol2("frontToBackBlender", args)
{
}
string FrontToBackBlender::code() const
{
  stringstream s;
  s << "void frontToBackBlender(vec4 src, inout vec4 dst, float factor)" << endl;
  s << "{" << endl;
  s << "    dst = (1.0 - dst.a)*src + dst;" << endl;
  s << "}" << endl;
  return s.str();
}

MixBlender::MixBlender(const vector<string> &args)
: TextureBlenderCol2("mixBlender", args)
{
}
string MixBlender::code() const
{
  stringstream s;
  s << "void mixBlender(vec4 src, inout vec4 dst, float factor)" << endl;
  s << "{" << endl;
  s << "    dst = mix(dst, src, factor);" << endl;
  s << "}" << endl;
  return s.str();
}

AddBlender::AddBlender(const vector<string> &args, bool smoothAdd, bool signedAdd)
: TextureBlenderCol2("addBlender", args),
  smoothAdd_(smoothAdd),
  signedAdd_(signedAdd)
{
}
string AddBlender::code() const
{
  stringstream s;
  s << "void addBlender(vec4 src, inout vec4 dst, float factor)" << endl;
  s << "{" << endl;
  s << "    dst += src;" << endl;
  s << "}" << endl;
  return s.str();
}

AddNormalizedBlender::AddNormalizedBlender(const vector<string> &args)
: TextureBlenderCol2("addNormalizedBlender", args)
{
}
string AddNormalizedBlender::code() const
{
  stringstream s;
  s << "void addNormalizedBlender(vec4 src, inout vec4 dst, float factor)" << endl;
  s << "{" << endl;
  s << "    vec4 outCol;" << endl;
  s << "    outCol.a = max(dst.a, src.a);" << endl;
  s << "    if(outCol.a<=0.0) { dst=vec4(0); return; }" << endl;
  s << "    outCol.rgb = 2.0*(dst.a/(dst.a + src.a))*dst.rgb + 2.0*(src.a/(dst.a + src.a))*src.rgb;" << endl;
  s << "    float m = max(outCol.r, max(outCol.g, outCol.b));" << endl;
  s << "    if(m>1.0) outCol.rgb /= m;" << endl;
  s << "    dst = mix(dst, outCol, factor);" << endl;
  s << "}" << endl;
  return s.str();
}

MulBlender::MulBlender(const vector<string> &args)
: TextureBlenderCol2("mulBlender", args)
{
}
string MulBlender::code() const
{
  stringstream s;
  s << "void mulBlender(vec4 src, inout vec4 dst, float factor)" << endl;
  s << "{" << endl;
  s << "   dst *= src;" << endl;
  s << "}" << endl;
  return s.str();
}

ScreenBlender::ScreenBlender(const vector<string> &args)
: TextureBlenderCol2("screenBlender", args)
{
}
string ScreenBlender::code() const
{
  stringstream s;
  s << "void screenBlender(vec4 src, inout vec4 dst, float factor)" << endl;
  s << "{" << endl;
  s << "    float facm = 1.0 - factor;" << endl;
  s << "    dst = vec4(1.0) - (vec4(facm) + factor*(vec4(1.0) - src))*(vec4(1.0) - dst);" << endl;
  s << "}" << endl;
  return s.str();
}

OverlayBlender::OverlayBlender(const vector<string> &args)
: TextureBlenderCol2("overlayBlende", args)
{
}
string OverlayBlender::code() const
{
  stringstream s;
  s << "void overlayBlende(vec4 src, inout vec4 dst, float factor)" << endl;
  s << "{" << endl;
  s << "    float facm = 1.0 - factor;" << endl;
  s << "" << endl;
  s << "    if(dst.r < 0.5)" << endl;
  s << "        dst.r *= facm + 2.0* factor *src.r;" << endl;
  s << "    else" << endl;
  s << "        dst.r = 1.0 - (facm + 2.0* factor *(1.0 - src.r))*(1.0 - dst.r);" << endl;
  s << "" << endl;
  s << "    if(dst.g < 0.5)" << endl;
  s << "        dst.g *= facm + 2.0* factor *src.g;" << endl;
  s << "    else" << endl;
  s << "        dst.g = 1.0 - (facm + 2.0* factor *(1.0 - src.g))*(1.0 - dst.g);" << endl;
  s << "" << endl;
  s << "    if(dst.b < 0.5)" << endl;
  s << "        dst.b *= facm + 2.0* factor *src.b;" << endl;
  s << "    else" << endl;
  s << "        dst.b = 1.0 - (facm + 2.0* factor *(1.0 - src.b))*(1.0 - dst.b);" << endl;
  s << "}" << endl;
  return s.str();
}

SubBlender::SubBlender(const vector<string> &args)
: TextureBlenderCol2("subBlender", args)
{
}
string SubBlender::code() const
{
  stringstream s;
  s << "void subBlender(vec4 src, inout vec4 dst, float factor)" << endl;
  s << "{" << endl;
  s << "    dst -= src;" << endl;
  s << "}" << endl;
  return s.str();
}

DivBlender::DivBlender(const vector<string> &args)
: TextureBlenderCol2("divBlender", args)
{
}
string DivBlender::code() const
{
  stringstream s;
  s << "void divBlender(vec4 src, inout vec4 dst, float factor)" << endl;
  s << "{" << endl;
  s << "    float facm = 1.0 - factor;" << endl;
  s << "" << endl;
  s << "    if(src.r != 0.0) dst.r = facm*dst.r + factor *dst.r/src.r;" << endl;
  s << "    if(src.g != 0.0) dst.g = facm*dst.g + factor *dst.g/src.g;" << endl;
  s << "    if(src.b != 0.0) dst.b = facm*dst.b + factor *dst.b/src.b;" << endl;
  s << "}" << endl;
  return s.str();
}

DiffBlender::DiffBlender(const vector<string> &args)
: TextureBlenderCol2("diffBlender", args)
{
}
string DiffBlender::code() const
{
  stringstream s;
  s << "void diffBlender(vec4 src, inout vec4 dst, float factor)" << endl;
  s << "{" << endl;
  s << "    dst = abs(dst - src);" << endl;
  s << "}" << endl;
  return s.str();
}

DarkBlender::DarkBlender(const vector<string> &args)
: TextureBlenderCol2("", args)
{
}
string DarkBlender::code() const
{
  stringstream s;
  s << "void darkBlender(vec4 src, inout vec4 dst, float factor)" << endl;
  s << "{" << endl;
  s << "    dst = min(dst, src*factor);" << endl;
  s << "}" << endl;
  return s.str();
}

LightBlender::LightBlender(const vector<string> &args)
: TextureBlenderCol2("lightBlender", args)
{
}
string LightBlender::code() const
{
  stringstream s;
  s << "void lightBlender(vec4 src, inout vec4 dst, float factor)" << endl;
  s << "{" << endl;
  s << "    dst = max(dst, src*factor);" << endl;
  s << "}" << endl;
  return s.str();
}

DodgeBlender::DodgeBlender(const vector<string> &args)
: TextureBlenderCol2("dodgeBlender", args)
{
}
string DodgeBlender::code() const
{
  stringstream s;
  s << "void dodgeBlender(vec4 col1, inout vec4 col2, float factor)" << endl;
  s << "{" << endl;
  s << "    if(col2.r != 0.0) {" << endl;
  s << "        float tmp = 1.0 - factor *col1.r;" << endl;
  s << "        if(tmp <= 0.0)" << endl;
  s << "            col2.r = 1.0;" << endl;
  s << "        else if((tmp = col2.r/tmp) > 1.0)" << endl;
  s << "            col2.r = 1.0;" << endl;
  s << "        else" << endl;
  s << "            col2.r = tmp;" << endl;
  s << "    }" << endl;
  s << "    if(col2.g != 0.0) {" << endl;
  s << "        float tmp = 1.0 - factor *col1.g;" << endl;
  s << "        if(tmp <= 0.0)" << endl;
  s << "            col2.g = 1.0;" << endl;
  s << "        else if((tmp = col2.g/tmp) > 1.0)" << endl;
  s << "            col2.g = 1.0;" << endl;
  s << "        else" << endl;
  s << "            col2.g = tmp;" << endl;
  s << "    }" << endl;
  s << "    if(col2.b != 0.0) {" << endl;
  s << "        float tmp = 1.0 - factor *col1.b;" << endl;
  s << "        if(tmp <= 0.0)" << endl;
  s << "            col2.b = 1.0;" << endl;
  s << "        else if((tmp = col2.b/tmp) > 1.0)" << endl;
  s << "            col2.b = 1.0;" << endl;
  s << "        else" << endl;
  s << "            col2.b = tmp;" << endl;
  s << "    }" << endl;
  s << "}" << endl;
  return s.str();
}

BurnBlender::BurnBlender(const vector<string> &args)
: TextureBlenderCol2("burnBlender", args)
{
}
string BurnBlender::code() const
{
  stringstream s;
  s << "void burnBlender(vec4 col1, inout vec4 col2, float factor)" << endl;
  s << "{" << endl;
  s << "    float tmp, facm = 1.0 - factor ;" << endl;
  s << "" << endl;
  s << "    tmp = facm + factor *col1.r;" << endl;
  s << "    if(tmp <= 0.0)" << endl;
  s << "        col2.r = 0.0;" << endl;
  s << "    else if((tmp = (1.0 - (1.0 - col2.r)/tmp)) < 0.0)" << endl;
  s << "        col2.r = 0.0;" << endl;
  s << "    else if(tmp > 1.0)" << endl;
  s << "        col2.r = 1.0;" << endl;
  s << "    else" << endl;
  s << "        col2.r = tmp;" << endl;
  s << "" << endl;
  s << "    tmp = facm + factor *col1.g;" << endl;
  s << "    if(tmp <= 0.0)" << endl;
  s << "        col2.g = 0.0;" << endl;
  s << "    else if((tmp = (1.0 - (1.0 - col2.g)/tmp)) < 0.0)" << endl;
  s << "        col2.g = 0.0;" << endl;
  s << "    else if(tmp > 1.0)" << endl;
  s << "        col2.g = 1.0;" << endl;
  s << "    else" << endl;
  s << "        col2.g = tmp;" << endl;
  s << "" << endl;
  s << "    tmp = facm + factor *col1.b;" << endl;
  s << "    if(tmp <= 0.0)" << endl;
  s << "        col2.b = 0.0;" << endl;
  s << "    else if((tmp = (1.0 - (1.0 - col2.b)/tmp)) < 0.0)" << endl;
  s << "        col2.b = 0.0;" << endl;
  s << "    else if(tmp > 1.0)" << endl;
  s << "        col2.b = 1.0;" << endl;
  s << "    else" << endl;
  s << "        col2.b = tmp;" << endl;
  s << "}" << endl;
  return s.str();
}

HueBlender::HueBlender(const vector<string> &args)
: TextureBlenderCol2("", args)
{
  addDependencyCode( "rgbToHsv",  rgbToHsv );
  addDependencyCode( "hsvToRgb",  hsvToRgb );
}
string HueBlender::code() const
{
  stringstream s;
  s << "void hueBlender(vec4 col1, inout vec4 col2, float factor)" << endl;
  s << "{" << endl;
  s << "    float facm = 1.0 - factor ;" << endl;
  s << "" << endl;
  s << "    vec4 buf = col2;" << endl;
  s << "" << endl;
  s << "    vec4 hsv, hsv2, tmp;" << endl;
  s << "    rgb_to_hsv(col1, hsv2);" << endl;
  s << "" << endl;
  s << "    if(hsv2.y != 0.0) {" << endl;
  s << "        rgb_to_hsv(col2, hsv);" << endl;
  s << "        hsv.x = hsv2.x;" << endl;
  s << "        hsv_to_rgb(hsv, tmp);" << endl;
  s << "" << endl;
  s << "        col2 = mix(buf, tmp, factor );" << endl;
  s << "        col2.a = buf.a;" << endl;
  s << "    }" << endl;
  s << "}" << endl;
  return s.str();
}

SatBlender::SatBlender(const vector<string> &args)
: TextureBlenderCol2("satBlender", args)
{
  addDependencyCode( "rgbToHsv",  rgbToHsv );
  addDependencyCode( "hsvToRgb",  hsvToRgb );
}
string SatBlender::code() const
{
  stringstream s;
  s << "void satBlender( vec4 col1, inout vec4 col2, float factor)" << endl;
  s << "{" << endl;
  s << "    float facm = 1.0 - factor ;" << endl;
  s << "" << endl;
  s << "    vec4 hsv = vec4(0.0);" << endl;
  s << "    vec4 hsv2 = vec4(0.0);" << endl;
  s << "    rgb_to_hsv(col2, hsv);" << endl;
  s << "" << endl;
  s << "    if(hsv.y != 0.0) {" << endl;
  s << "        rgb_to_hsv(col1, hsv2);" << endl;
  s << "        hsv.y = facm*hsv.y + factor *hsv2.y;" << endl;
  s << "        hsv_to_rgb(hsv, col2);" << endl;
  s << "    }" << endl;
  s << "}" << endl;
  return s.str();
}

ValBlender::ValBlender(const vector<string> &args)
: TextureBlenderCol2("valBlender", args)
{
  addDependencyCode( "rgbToHsv",  rgbToHsv );
  addDependencyCode( "hsvToRgb",  hsvToRgb );
}
string ValBlender::code() const
{
  stringstream s;
  s << "void valBlender(vec4 col1, inout vec4 col2, float factor)" << endl;
  s << "{" << endl;
  s << "    float facm = 1.0 - factor ;" << endl;
  s << "" << endl;
  s << "    vec4 hsv, hsv2;" << endl;
  s << "    rgb_to_hsv(col2, hsv);" << endl;
  s << "    rgb_to_hsv(col1, hsv2);" << endl;
  s << "" << endl;
  s << "    hsv.z = facm*hsv.z + factor *hsv2.z;" << endl;
  s << "    hsv_to_rgb(hsv, col2);" << endl;
  s << "}" << endl;
  return s.str();
}

ColBlender::ColBlender(const vector<string> &args)
: TextureBlenderCol2("colBlender", args)
{
  addDependencyCode( "rgbToHsv",  rgbToHsv );
  addDependencyCode( "hsvToRgb",  hsvToRgb );
}
string ColBlender::code() const
{
  stringstream s;
  s << "void colBlender(vec4 col1, inout vec4 col2, float factor)" << endl;
  s << "{" << endl;
  s << "    float facm = 1.0 - factor ;" << endl;
  s << "" << endl;
  s << "    vec4 hsv, hsv2, tmp;" << endl;
  s << "    rgb_to_hsv(col1, hsv2);" << endl;
  s << "" << endl;
  s << "    if(hsv2.y != 0.0) {" << endl;
  s << "        rgb_to_hsv(col2, hsv);" << endl;
  s << "        hsv.x = hsv2.x;" << endl;
  s << "        hsv.y = hsv2.y;" << endl;
  s << "        hsv_to_rgb(hsv, tmp);" << endl;
  s << "" << endl;
  s << "        col2.rgb = mix(col2.rgb, tmp.rgb, factor );" << endl;
  s << "    }" << endl;
  s << "}" << endl;
  return s.str();
}

string SoftBlender::code() const
{
  stringstream s;
  s << "void softBlender(vec4 col1, inout vec4 col2, float factor)" << endl;
  s << "{" << endl;
  s << "    float facm = 1.0 - factor ;" << endl;
  s << "" << endl;
  s << "    vec4 one = vec4(1.0);" << endl;
  s << "    vec4 scr = one - (one - col1)*(one - col2);" << endl;
  s << "    col2 = facm*col2 + factor *((one - col2)*col1*col2 + col2*scr);" << endl;
  s << "}" << endl;
  return s.str();
}

string LinearBlender::code() const
{
  stringstream s;
  s << "void linearBlender(vec4 col1, inout vec4 col2, float factor)" << endl;
  s << "{" << endl;
  s << "    if(col1.r > 0.5)" << endl;
  s << "        col2.r= col2.r + factor *(2.0*(col1.r - 0.5));" << endl;
  s << "    else" << endl;
  s << "        col2.r= col2.r + factor *(2.0*(col1.r) - 1.0);" << endl;
  s << "" << endl;
  s << "    if(col1.g > 0.5)" << endl;
  s << "        col2.g= col2.g + factor *(2.0*(col1.g - 0.5));" << endl;
  s << "    else" << endl;
  s << "        col2.g= col2.g + factor *(2.0*(col1.g) - 1.0);" << endl;
  s << "" << endl;
  s << "    if(col1.b > 0.5)" << endl;
  s << "        col2.b= col2.b + factor *(2.0*(col1.b - 0.5));" << endl;
  s << "    else" << endl;
  s << "        col2.b= col2.b + factor *(2.0*(col1.b) - 1.0);" << endl;
  s << "}" << endl;
  return s.str();
}



TextureBlenderCol2* newBlender(TextureBlendMode blendMode,
		const vector<string> &args)
{
  switch(blendMode)
  {
  case BLEND_MODE_SRC: {
    return NULL;
  } case BLEND_MODE_ALPHA: {
    return new AlphaBlender(args);
  } case BLEND_MODE_MIX: {
    return new MixBlender(args);
  } case BLEND_MODE_MULTIPLY: {
    return new MulBlender(args);
  } case BLEND_MODE_FRONT_TO_BACK: {
    return new FrontToBackBlender(args);
  } case BLEND_MODE_ADD: {
    bool smoothAdd = false;
    bool signedAdd = false;
    return new AddBlender(args, smoothAdd, signedAdd);
  } case BLEND_MODE_ADD_NORMALIZED: {
    return new AddNormalizedBlender(args);
  } case BLEND_MODE_SMOOTH_ADD: {
      bool smoothAdd = true;
      bool signedAdd = false;
      return new AddBlender(args, smoothAdd, signedAdd);
  } case BLEND_MODE_SIGNED_ADD: {
      bool smoothAdd = false;
      bool signedAdd = true;
      return new AddBlender(args, smoothAdd, signedAdd);
  } case BLEND_MODE_SUBSTRACT: {
      return new SubBlender(args);
  } case BLEND_MODE_DIVIDE: {
      return new DivBlender(args);
  } case BLEND_MODE_DIFFERENCE: {
      return new DiffBlender(args);
  } case BLEND_MODE_LIGHTEN: {
      return new LightBlender(args);
  } case BLEND_MODE_DARKEN: {
      return new DarkBlender(args);
  } case BLEND_MODE_SCREEN: {
      return new ScreenBlender(args);
  } case BLEND_MODE_OVERLAY: {
      return new OverlayBlender(args);
  } case BLEND_MODE_HUE: {
      return new HueBlender(args);
  } case BLEND_MODE_SATURATION: {
      return new SatBlender(args);
  } case BLEND_MODE_VALUE: {
      return new ValBlender(args);
  } case BLEND_MODE_COLOR: {
      return new ColBlender(args);
  } case BLEND_MODE_DODGE: {
      return new DodgeBlender(args);
  } case BLEND_MODE_BURN: {
      return new BurnBlender(args);
  } case BLEND_MODE_SOFT: {
      return new SoftBlender(args);
  } case BLEND_MODE_LINEAR: {
      return new LinearBlender(args);
  }}
}


