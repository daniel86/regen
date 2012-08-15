
#include <sstream>

#include "raycast-shader.h"

#include <ogle/textures/texel-transfer.h>
#include <ogle/utility/string-util.h>

static const string intersectBox =
"bool intersectBox(vec3 origin, vec3 dir,\n"
"           vec3 minBound, vec3 maxBound,\n"
"           out float t0, out float t1)\n"
"{\n"
"    vec3 invR = 1.0 / dir;\n"
"    vec3 tbot = invR * (minBound-origin);\n"
"    vec3 ttop = invR * (maxBound-origin);\n"
"    vec3 tmin = min(ttop, tbot);\n"
"    vec3 tmax = max(ttop, tbot);\n"
"    vec2 t = max(tmin.xx, tmin.yz);\n"
"    t0 = max(t.x, t.y);\n"
"    t = min(tmax.xx, tmax.yz);\n"
"    t1 = min(t.x, t.y);\n"
"    return t0 <= t1;\n"
"}\n\n";

RayCastShader::RayCastShader(TextureState *textureState, vector<string> &args)
: ShaderFunctions("rayCast", args),
  texture_(textureState)
{
  setMinVersion(150);

  addUniform( GLSLUniform(
      "mat4", "in_viewMatrix") );
  addUniform( GLSLUniform(
      "mat4", "in_inverseViewMatrix") );
  addUniform( GLSLUniform(
      "sampler3D", FORMAT_STRING("in_" << texture_->texture()->name())) );

  int numSamples = 50;
  float stepSize = sqrt(1.0)/(float)numSamples;
  addConstant( GLSLConstant( "float", "in_stepSize", FORMAT_STRING(stepSize) ) );

  addDependencyCode( "intersectBox", intersectBox );
}

string RayCastShader::code() const
{
  stringstream s;

  s << "void " << myName_ << "(out vec3 fragmentNormal, inout vec4 fragmentColor)" << endl;
  s << "{" << endl;
  s << "    vec3 rayOrigin_ = in_inverseViewMatrix[3].xyz;" << endl;
  s << "    vec3 rayDirection = normalize(in_posWorld.xyz - in_inverseViewMatrix[3].xyz);" << endl;
  s << "    " << endl;

  s << "    float tnear, tfar;" << endl;
  s << "    intersectBox( rayOrigin_, rayDirection, " << endl;
  s << "           vec3(-1.0), vec3(+1.0), tnear, tfar);" << endl;
  s << "    if (tnear < 0.0) tnear = 0.0;" << endl;
  s << "    " << endl;

  s << "    vec3 rayStart = rayOrigin_ + rayDirection * tnear;" << endl;
  s << "    vec3 rayStop = rayOrigin_ + rayDirection * tfar;" << endl;
  s << "    rayStart.y *= -1; rayStop.y *= -1;" << endl;
  s << "    " << endl;
  s << "    // Transform from object space to texture coordinate space:" << endl;
  s << "    rayStart = 0.5 * (rayStart + 1.0);" << endl;
  s << "    rayStop = 0.5 * (rayStop + 1.0);" << endl;
  s << "    " << endl;
  s << "    vec3 ray = rayStop - rayStart;" << endl;
  s << "    float rayLength = length(ray);" << endl;
  s << "    vec3 stepVector = normalize(ray) * in_stepSize;" << endl;
  s << "    " << endl;
  s << "    vec3 pos = rayStart;" << endl;
  s << "    vec4 dst = vec4(0);" << endl;
  s << "    // break out of the loop if alpha reached 1.0" << endl;
  s << "    while(dst.a < 0.999 && rayLength > 0)" << endl;
  s << "    {" << endl;
  // TODO: RAYCAST: texture has a lot more params!
  TexelTransfer *transfer = texture_->transfer().get();
  if(transfer!=NULL) {
    s << "        vec4 density = texture(in_" << texture_->texture()->name() << ", pos);" << endl;
    s << "        vec4 src = " << transfer->name() << "(density);" << endl;
  } else {
    s << "        vec4 src = texture(in_" << texture_->texture()->name() << ", pos);" << endl;
  }
  // TODO: RAYCAST: light ?
  s << "        // opacity weighted color" << endl;
  s << "        src.rgb *= src.a;" << endl;
  s << "        // front-to-back blending" << endl;
  s << "        dst = (1.0 - dst.a) * src + dst;" << endl;
  s << "        pos += stepVector;" << endl;
  s << "        rayLength -= in_stepSize;" << endl;
  s << "    }" << endl;

#define DRAW_RAY_LENGTH 0
#define DRAW_RAY_START 0
#define DRAW_RAY_STOP 0
#if DRAW_RAY_LENGTH
  s << "    fragmentColor = vec4(vec3(length(ray)), 1.0);" << endl;
#elif DRAW_RAY_START
  s << "    fragmentColor = vec4(rayStart, 1.0);" << endl;
#elif DRAW_RAY_STOP
  s << "    fragmentColor = vec4(rayStop, 1.0);" << endl;
#else
  s << "    fragmentColor = dst;" << endl;
#endif

  s << "}" << endl;

  return s.str();
}

