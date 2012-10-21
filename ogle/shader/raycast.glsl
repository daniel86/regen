
#include <sstream>

#include "raycast-shader.h"

#include <ogle/textures/texel-transfer.h>
#include <ogle/utility/string-util.h>


RayCastShader::RayCastShader(TextureState *textureState, vector<string> &args)
: ShaderFunctions("rayCast", args),
  texture_(textureState)
{
  setMinVersion(150);

  addUniform( GLSLUniform(
      "sampler3D", FORMAT_STRING("in_" << textureState->texture()->name())) );
  addConstant( GLSLConstant(
      "float", "in_rayStep", FORMAT_STRING(1.0/50.0) ) );

  addDependencyCode( "intersectBox", intersectBox );
}

-- rayCast

uniform sampler3D in_volume;
const float in_rayStep=(1.0/50.0);

bool intersectBox(vec3 origin, vec3 dir,
           vec3 minBound, vec3 maxBound,
           out float t0, out float t1)
{
    vec3 invR = 1.0 / dir;
    vec3 tbot = invR * (minBound-origin);
    vec3 ttop = invR * (maxBound-origin);
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);
    vec2 t = max(tmin.xx, tmin.yz);
    t0 = max(t.x, t.y);
    t = min(tmax.xx, tmax.yz);
    t1 = min(t.x, t.y);
    return t0 < t1;
}

void rayCast(out vec3 fragmentNormal, inout vec4 fragmentColor)
{
    vec3 rayOrigin_ = in_inverseViewMatrix[3].xyz;
    vec3 rayDirection = normalize(in_posWorld.xyz - in_inverseViewMatrix[3].xyz);

    float tnear, tfar;
    if(!intersectBox( rayOrigin_, rayDirection, 
           vec3(-1.0), vec3(+1.0), tnear, tfar))
    {
        fragmentColor=vec4(0);
        return;
    }
    if (tnear < 0.0) tnear = 0.0;
    

    vec3 rayStart = rayOrigin_ + rayDirection * tnear;
    vec3 rayStop = rayOrigin_ + rayDirection * tfar;
    rayStart.y *= -1; rayStop.y *= -1;
    
    // Transform from object space to texture coordinate space:
    rayStart = 0.5 * (rayStart + 1.0);
    rayStop = 0.5 * (rayStop + 1.0);
    
    vec3 ray = rayStop - rayStart;
    vec3 stepVector = normalize(ray) * in_rayStep;
    
    vec3 pos = rayStart;
    vec4 dst = vec4(0);
    for(float rayLength=length(ray); rayLength>0.0; rayLength-=in_rayStep)
    {
  TexelTransfer *transfer = texture_->transfer().get();
  if(transfer!=NULL) {
          vec4 src = " << transfer->name() << "(texture(in_" << texture_->texture()->name() << ", pos));
  } else {
          vec4 src = texture(in_" << texture_->texture()->name() << ", pos);
  }
        // opacity weighted color
        src.rgb *= src.a;
        // front-to-back blending
        dst = (1.0 - dst.a) * src + dst;
        pos += stepVector;
        // break out of the loop if alpha reached 1.0
        if(dst.a > 0.999) break;
    }

#define DRAW_RAY_LENGTH 0
#define DRAW_RAY_START 0
#define DRAW_RAY_STOP 0
#if DRAW_RAY_LENGTH
    fragmentColor = vec4(vec3(length(ray)), 1.0);
#elif DRAW_RAY_START
    fragmentColor = vec4(rayStart, 1.0);
#elif DRAW_RAY_STOP
    fragmentColor = vec4(rayStop, 1.0);
#else
    fragmentColor = dst;
#endif

}

