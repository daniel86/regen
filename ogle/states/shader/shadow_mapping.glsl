
-- linearizeDepth
float linearizeDepth(float expDepth, float n, float f)
{
    float z_n = 2.0*expDepth - 1.0;
    return (2.0*n)/(f+n - z_n*(f-n));
}

-- moments.vs
in vec3 in_pos;
#ifdef IS_2D_SHADOW
out vec2 out_texco;
#endif

void main() {
#ifdef IS_2D_SHADOW
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
#endif
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- moments.gs
#extension GL_EXT_geometry_shader4 : enable
#extension GL_ARB_gpu_shader5 : enable

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;
#ifdef IS_CUBE_SHADOW
layout(invocations = 6) in;
#else
layout(invocations = NUM_SHADOW_MAP_SLICES) in;
#endif

#ifdef IS_2D_SHADOW
out vec2 out_texco;
#else
out vec3 out_texco;
#endif

#ifdef IS_CUBE_SHADOW
#include utility.computeCubeMapDirection
#endif

#ifdef IS_CUBE_SHADOW
void emitVertex(vec4 P, int layer)
{
    gl_Position = P;
    out_texco = computeCubeMapDirection(vec2(P.x, -P.y), layer);
    EmitVertex();
}
#endif
#ifdef IS_ARRAY_SHADOW
void emitVertex(vec4 P, int layer)
{
    gl_Position = P;
    out_texco = vec3(0.5*(gl_Position.xy+vec2(1.0)), layer);
    EmitVertex();
}
#endif

void main(void) {
    int layer = gl_InvocationID;
    // select framebuffer layer
    gl_Layer = layer;
    // TODO: allow to skip layers
    emitVertex(gl_PositionIn[0], layer);
    emitVertex(gl_PositionIn[1], layer);
    emitVertex(gl_PositionIn[2], layer);
    EndPrimitive();
}

-- moments.fs
out vec4 output;
#ifdef IS_2D_SHADOW
in vec2 in_texco;
#else
in vec3 in_texco;
#endif

#ifdef IS_ARRAY_SHADOW
uniform sampler2DArray in_shadowTexture;
#elif IS_CUBE_SHADOW
uniform samplerCube in_shadowTexture;
#else // IS_2D_SHADOW
uniform sampler2D in_shadowTexture;
#endif

uniform float in_shadowFar;
uniform float in_shadowNear;

#include shadow_mapping.linearizeDepth

void main()
{
    float depth = texture(in_shadowTexture, in_texco).x;
#ifdef IS_ARRAY_SHADOW
    // no need to linearize for orthografic projection ?

#else
    // The depth is in exp space and must be linearized for shadow comparison.
    depth = clamp( linearizeDepth(
        depth, in_shadowNear, in_shadowFar), 0.0, 1.0 );
#endif

    // Rate of depth change in texture space.
    // This will actually compare local depth with the depth calculated for
    // neighbor texels.
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    output = vec4(depth, depth*depth + 0.25*(dx*dx+dy*dy), 1.0, 1.0);
}


-----------------------------
------ Shadow sampling ------
-----------------------------

-- filtering.vsm
#ifndef __SM_FILTER_VSM_included__
#define2 __SM_FILTER_VSM_included__

float linstep(float low, float high, float v)
{
    return clamp((v-low)/(high-low), 0.0, 1.0);
}
float chebyshevUpperBound(float dist, vec2 moments)
{
    float variance = max(moments.y - moments.x*moments.x, 0.002);
    float d = dist - moments.x;
    float p = smoothstep(dist-0.02, dist, moments.x);
    float p_max = linstep(0.2, 1.0, variance/(variance + d*d));
    return clamp(max(p, p_max), 0.0, 1.0);
}

float shadowVSM(float near, float far, float dist, sampler2D tex, vec4 shadowCoord)
{
    vec2 moments = texture(tex, shadowCoord.xy/shadowCoord.w).xy;
    float depth = linstep(near, far, dist);
    return chebyshevUpperBound(depth, moments);
}
float shadowVSM(float near, float far, float dist, samplerCube tex, vec4 shadowCoord)
{
    vec2 moments = texture(tex, shadowCoord.xyz/shadowCoord.w).xy;
    float depth = linstep(near, far, dist);
    return chebyshevUpperBound(depth, moments);
}
float shadowVSM(sampler2DArray tex, vec4 shadowCoord, int layer)
{
    // XXX
    vec3 wdiv = shadowCoord.xyz/shadowCoord.w;
    vec2 moments = texture(tex, vec3(wdiv.xy, float(layer))).xy;
    float depth = clamp(wdiv.z, 0.0, 1.0);
    return chebyshevUpperBound(depth, moments);
}

#endif

-- filtering.gaussian
#ifndef __SM_FILTER_GAUSS_included__
#define2 __SM_FILTER_GAUSS_included__
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
float shadowGaussian(samplerCubeShadow tex, vec4 shadowCoord)
{
    // TODO: howto handle this case ?
/*
		float ps = 8.0 / gl_LightSource[0].quadraticAttenuation;
		shadowcolor = shadowCube( texture3, cubecoord ).x;
		shadowcolor += shadowCube( texture3, vec4(cubecoord.xyz + vec3(ps,ps,ps),cubecoord.w) ).x;
		shadowcolor += shadowCube( texture3, vec4(cubecoord.xyz + vec3(-ps,ps,-ps),cubecoord.w) ).x;
		shadowcolor += shadowCube( texture3, vec4(cubecoord.xyz + vec3(-ps,-ps,-ps),cubecoord.w) ).x;
		shadowcolor += shadowCube( texture3, vec4(cubecoord.xyz + vec3(ps,-ps,ps),cubecoord.w) ).x;
		shadowcolor /= 5.0;
*/
    return shadowCube(tex, shadowCoord).x;
}
#endif

-- sampling.all
#include shadow_mapping.sampling.dir
#include shadow_mapping.sampling.point
#include shadow_mapping.sampling.spot

-- filtering.all
#ifndef __SM_FILTER_ALL_included__
#define2 __SM_FILTER_ALL_included__
#include shadow_mapping.filtering.gaussian
#include shadow_mapping.filtering.vsm
#endif

-- sampling.dir
#include shadow_mapping.filtering.all

#ifdef NUM_SHADOW_MAP_SLICES
#define __COUNT NUM_SHADOW_MAP_SLICES
// shadow map selection is done by distance of pixel to the camera.
int getShadowLayer(float depth, float shadowFar[__COUNT])
{
    for(int i=0; i<__COUNT; ++i)
        if(depth < shadowFar[i]) { return i; }
    return 0;
}
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

float dirShadowSingle(vec3 P, int layer, sampler2DArrayShadow tex, mat4 shadowMatrix)
{
    return shadow2DArray(tex, dirShadowCoord(layer, P, shadowMatrix));
}
float dirShadowGaussian(vec3 P, int layer, sampler2DArrayShadow tex, mat4 shadowMatrix)
{
    return shadowGaussian(tex, dirShadowCoord(layer, P, shadowMatrix));
}
float dirShadowVSM(vec3 P, int layer, sampler2DArray tex, mat4 shadowMatrix)
{
    //return shadowVSM(tex, dirShadowCoord(layer, posWorld, shadowMatrix));
    return shadowVSM(tex, shadowMatrix*vec4(P,1.0), layer);
}
#endif

-- sampling.point
#include shadow_mapping.filtering.all

vec4 pointShadowCoord(float n, float f, vec3 lightVec)
{
    vec3 absTexco = abs(lightVec);
    float magnitude = max(absTexco.x, max(absTexco.y, absTexco.z));
    return vec4(-lightVec, 0.5*(1.0 + (f+n)/(f-n) - (2*f*n)/(f-n)/magnitude));
}

float pointShadowSingle(vec3 P, float n, float f, vec3 lightVec, samplerCubeShadow tex)
{
    return shadowCube(tex, pointShadowCoord(n,f,lightVec)).x;
}
float pointShadowGaussian(vec3 P, float n, float f, vec3 lightVec, samplerCubeShadow tex)
{
    return shadowGaussian(tex, pointShadowCoord(n,f,lightVec));
}
float pointShadowVSM(vec3 P, float n, float f, vec3 lightVec, samplerCube tex)
{
    return shadowVSM(n, f, length(lightVec), tex, pointShadowCoord(n,f,lightVec));
}

-- sampling.spot
#include shadow_mapping.filtering.all

float spotShadowSingle(vec3 P, float near, float far, vec3 lightVec, sampler2DShadow tex, mat4 shadowMatrix)
{
    return textureProj(tex, shadowMatrix*vec4(P,1.0));
}
float spotShadowGaussian(vec3 P, float near, float far, vec3 lightVec, sampler2DShadow tex, mat4 shadowMatrix)
{
    return shadowGaussian(tex, shadowMatrix*vec4(P,1.0));
}
float spotShadowVSM(vec3 P, float near, float far, vec3 lightVec, sampler2D tex, mat4 shadowMatrix)
{
    return shadowVSM(near, far, length(lightVec), tex, shadowMatrix*vec4(P,1.0));
}

