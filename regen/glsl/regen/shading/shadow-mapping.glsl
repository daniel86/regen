
-- filtering.vsm
#ifndef __SM_FILTER_VSM_included__
#define2 __SM_FILTER_VSM_included__
#include regen.math.linstep

float chebyshevUpperBound(float dist, vec2 moments)
{
    float variance = max(moments.y - moments.x*moments.x, 0.01);
    float d = dist - moments.x;
    float p = float(dist < moments.x);
    float p_max = linstep(0.2, 1.0, variance/(variance + d*d));
    return clamp(max(p, p_max), 0.0, 1.0);
}

float shadowVSM(sampler2D tex, vec4 shadowCoord, float linearDepth)
{
    vec2 moments = texture(tex, shadowCoord.xy/shadowCoord.w).xy;
    return chebyshevUpperBound(linearDepth, moments);
}
float shadowVSM(samplerCube tex, vec4 shadowCoord, float linearDepth)
{
    vec2 moments = texture(tex, shadowCoord.xyz/shadowCoord.w).xy;
    return chebyshevUpperBound(linearDepth, moments);
}
float shadowVSM(sampler2DArray tex, vec4 shadowCoord)
{
    vec2 moments = texture(tex, shadowCoord.xyz).xy;
    // Ortho matrix projects linear depth
    float depth = shadowCoord.w;
    return chebyshevUpperBound(depth, moments);
}
#endif

-- filtering.gaussian
#ifndef __SM_FILTER_GAUSS_included__
#define2 __SM_FILTER_GAUSS_included__
#include regen.math.computeCubeOffset

// Gaussian 3x3 filter
float shadowGaussian(sampler2DShadow tex, vec4 shadowCoord)
{
    float ret = textureProj(tex, shadowCoord) * 0.25;
    ret += textureProjOffset(tex, shadowCoord, ivec2( -1, -1)) * 0.0625;
    ret += textureProjOffset(tex, shadowCoord, ivec2( -1, 0)) * 0.125;
    ret += textureProjOffset(tex, shadowCoord, ivec2( -1, 1)) * 0.0625;
    ret += textureProjOffset(tex, shadowCoord, ivec2( 0, -1)) * 0.125;
    ret += textureProjOffset(tex, shadowCoord, ivec2( 0, 1)) * 0.125;
    ret += textureProjOffset(tex, shadowCoord, ivec2( 1, -1)) * 0.0625;
    ret += textureProjOffset(tex, shadowCoord, ivec2( 1, 0)) * 0.125;
    ret += textureProjOffset(tex, shadowCoord, ivec2( 1, 1)) * 0.0625;
    return ret;
}
float shadowGaussian(sampler2DArrayShadow tex, vec4 shadowCoord)
{
    float ret = shadow2DArray(tex, shadowCoord).x * 0.25;
    ret += shadow2DArrayOffset(tex, shadowCoord, ivec2( -1, -1)).x * 0.0625;
    ret += shadow2DArrayOffset(tex, shadowCoord, ivec2( -1, 0)).x * 0.125;
    ret += shadow2DArrayOffset(tex, shadowCoord, ivec2( -1, 1)).x * 0.0625;
    ret += shadow2DArrayOffset(tex, shadowCoord, ivec2( 0, -1)).x * 0.125;
    ret += shadow2DArrayOffset(tex, shadowCoord, ivec2( 0, 1)).x * 0.125;
    ret += shadow2DArrayOffset(tex, shadowCoord, ivec2( 1, -1)).x * 0.0625;
    ret += shadow2DArrayOffset(tex, shadowCoord, ivec2( 1, 0)).x * 0.125;
    ret += shadow2DArrayOffset(tex, shadowCoord, ivec2( 1, 1)).x * 0.0625;
    return ret;
}
float shadowGaussian(samplerCubeShadow tex, vec4 coord, float texelSize)
{
    vec3 dx, dy;
    computeCubeOffset(coord.xyz, texelSize, dx, dy);
    
    float ret = shadowCube(tex, coord).x * 0.25;
#define CUBE_MAP_OFFSET(off) float(shadowCube(tex,vec4(coord.xyz+off.x*dx+off.y*dy,coord.w)))
    ret += CUBE_MAP_OFFSET(vec2(-1,-1)) * 0.0625;
    ret += CUBE_MAP_OFFSET(vec2(-1, 0)) * 0.125;
    ret += CUBE_MAP_OFFSET(vec2(-1, 1)) * 0.0625;
    ret += CUBE_MAP_OFFSET(vec2( 0,-1)) * 0.125;
    ret += CUBE_MAP_OFFSET(vec2( 0, 1)) * 0.125;
    ret += CUBE_MAP_OFFSET(vec2( 1,-1)) * 0.0625;
    ret += CUBE_MAP_OFFSET(vec2( 1, 0)) * 0.125;
    ret += CUBE_MAP_OFFSET(vec2( 1, 1)) * 0.0625;
#undef CUBE_MAP_OFFSET
    return ret;
}
#endif

-- filtering.all
#include regen.shading.shadow-mapping.filtering.gaussian
#include regen.shading.shadow-mapping.filtering.vsm

--------------------------------------
--------------------------------------
------ Shadow map sampling functions
--------------------------------------
--------------------------------------
-- sampling.dir
#include regen.shading.shadow-mapping.filtering.all

vec4 dirShadowCoord(int layer, vec3 posWorld, mat4 lightMatrix)
{
    // transform this fragment's position from world space to scaled light clip space
    // such that the xy coordinates are in [0;1]
    vec4 shadowCoord = lightMatrix*vec4(posWorld,1.0);
    shadowCoord.w = shadowCoord.z;
    // tell glsl in which layer to do the look up
    shadowCoord.z = float(layer);
    return shadowCoord;
}

#define dirShadowSingle(tex,x)   float(shadow2DArray(tex, x))
#define dirShadowGaussian(tex,x) shadowGaussian(tex, x)
#define dirShadowVSM(tex,x)      shadowVSM(tex, coord)

-- sampling.point
#include regen.shading.shadow-mapping.filtering.all
#include regen.math.linstep

#define pointShadowSingle(tex,l,d,n,f,s)   shadowCube(tex, vec4(-l,d)).x
#define pointShadowGaussian(tex,l,d,n,f,s) shadowGaussian(tex, vec4(-l,d), s)
#define pointShadowVSM(tex,l,d,n,f,s)      shadowVSM(tex,vec4(-l,d), linstep(n,f,length(l)))

-- sampling.spot
#include regen.shading.shadow-mapping.filtering.all
#include regen.math.linstep

#define spotShadowSingle(tex,x,l,n,f)   textureProj(tex,x);
#define spotShadowGaussian(tex,x,l,n,f) shadowGaussian(tex,x)
#define spotShadowVSM(tex,x,l,n,f)      shadowVSM(tex,x,linstep(n,f,length(l)))

-- sampling.all
#include regen.shading.shadow-mapping.sampling.dir
#include regen.shading.shadow-mapping.sampling.point
#include regen.shading.shadow-mapping.sampling.spot

