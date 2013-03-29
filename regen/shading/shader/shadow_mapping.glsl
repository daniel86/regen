
-- moments.defines
#ifdef IS_2D_SHADOW
  #define IS_2D_TEXTURE
#endif
#ifdef IS_CUBE_SHADOW
  #define IS_CUBE_TEXTURE
#endif
#ifdef IS_ARRAY_SHADOW
  #define IS_ARRAY_TEXTURE
#endif
#ifdef NUM_SHADOW_MAP_SLICES
  #define NUM_TEXTURE_LAYERS NUM_SHADOW_MAP_SLICES
#endif

--------------------------------------
--------------------------------------
---- Compute moments from previously sampled depth values.
---- Deferred moment computation allows to use abriary FS
---- for shadow casters.
--------------------------------------
--------------------------------------
-- moments.vs
#include shadow_mapping.moments.defines
#include sampling.vs

-- moments.gs
#include shadow_mapping.moments.defines
#include sampling.gs

-- moments.fs
#include shadow_mapping.moments.defines
#include sampling.fsHeader

out vec4 out_color;

#ifndef IS_ARRAY_SHADOW
uniform float in_shadowFar;
uniform float in_shadowNear;
#endif

#ifndef IS_ARRAY_SHADOW
float linearizeDepth(float expDepth, float n, float f) {
    float z_n = 2.0*expDepth - 1.0;
    return (2.0*n)/(f+n - z_n*(f-n));
}
#endif

void main()
{
    float depth = texture(in_inputTexture, in_texco).x;
#ifdef IS_ARRAY_SHADOW
    // no need to linearize for ortho projection

#else
    // Perspective projection saves depth none linear.
    // Linearize it for shadow comparison.
    depth = clamp( linearizeDepth(
        depth, in_shadowNear, in_shadowFar), 0.0, 1.0 );
#endif

    // Rate of depth change in texture space.
    // This will actually compare local depth with the depth calculated for
    // neighbor texels.
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    out_color = vec4(depth, depth*depth + 0.25*(dx*dx+dy*dy), 1.0, 1.0);
}

--------------------------------------
--------------------------------------
------ Shadow map filtering functions
--------------------------------------
--------------------------------------

-- filtering.vsm
#ifndef __SM_FILTER_VSM_included__
#define2 __SM_FILTER_VSM_included__
#include utility.linstep

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
#include utility.computeCubeOffset

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
float shadowGaussian(samplerCubeShadow tex, vec4 coord)
{
    vec3 dx, dy;
    float texelSize = 1.0/512.0; // TODO texture texel size uniform
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
#include shadow_mapping.filtering.gaussian
#include shadow_mapping.filtering.vsm

--------------------------------------
--------------------------------------
------ Shadow map sampling functions
--------------------------------------
--------------------------------------
-- sampling.dir
#include shadow_mapping.filtering.all

vec4 dirShadowCoord(int layer, vec3 posWorld, mat4 shadowMatrix)
{
    // transform this fragment's position from world space to scaled light clip space
    // such that the xy coordinates are in [0;1]
    vec4 shadowCoord = shadowMatrix*vec4(posWorld,1.0);
    shadowCoord.w = shadowCoord.z;
    // tell glsl in which layer to do the look up
    shadowCoord.z = float(layer);
    return shadowCoord;
}

float dirShadowSingle(sampler2DArrayShadow tex, vec4 coord)
{
    return float(shadow2DArray(tex, coord));
}
float dirShadowGaussian(sampler2DArrayShadow tex, vec4 coord)
{
    return shadowGaussian(tex, coord);
}
float dirShadowVSM(sampler2DArray tex, vec4 coord)
{
    return shadowVSM(tex, coord);
}

-- sampling.point
#include shadow_mapping.filtering.all
#include utility.linstep

float pointShadowSingle(samplerCubeShadow tex, vec3 lightVec, float n, float f)
{
    return shadowCube(tex, vec4(-lightVec, 1.0)).x;
}
float pointShadowGaussian(samplerCubeShadow tex, vec3 lightVec, float n, float f)
{
    return shadowGaussian(tex, vec4(-lightVec, 1.0));
}
float pointShadowVSM(samplerCube tex, vec3 lightVec, float n, float f)
{
    float depth = linstep(n, f, length(lightVec));
    return shadowVSM(tex, vec4(-lightVec, 1.0), depth);
}

-- sampling.spot
#include shadow_mapping.filtering.all
#include utility.linstep

float spotShadowSingle(sampler2DShadow tex, vec4 texco, vec3 lightVec, float n, float f)
{
    return textureProj(tex,texco);
}
float spotShadowGaussian(sampler2DShadow tex, vec4 texco, vec3 lightVec, float n, float f)
{
    return shadowGaussian(tex,texco);
}
float spotShadowVSM(sampler2D tex, vec4 texco, vec3 lightVec, float n, float f)
{
    float depth = linstep(n, f, length(lightVec));
    return shadowVSM(tex, texco, depth);
}

-- sampling.all
#include shadow_mapping.sampling.dir
#include shadow_mapping.sampling.point
#include shadow_mapping.sampling.spot

