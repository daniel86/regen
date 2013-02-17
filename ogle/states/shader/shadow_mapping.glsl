
-----------------------------
------ Shadow sampling ------
-----------------------------

-- filtering.vsm
#ifndef __SM_FILTER_VSM_included__
#define2 __SM_FILTER_VSM_included__
// Variance Shadow Mapping.
float chebyshevUpperBound(float dist, vec2 moments)
{
    // Surface is fully lit.
    // As the current fragment is before the light occluder
    if(dist <= moments.x) return 1.0;

    // The fragment is either in shadow or penumbra.
    // We now use chebyshev's upperBound to check
    // How likely this pixel is to be lit (p_max)
    float variance = moments.y - (moments.x*moments.x);
    variance = max(variance,0.00002);

    float d = dist - moments.x;
    return variance / (variance + d*d);
}
float fixLightBleed(float pMax, float amount)
{
    return clamp((pMax - amount) / (1.0 - amount), 0.0, 1.0);
}

float shadowVSM(sampler2D tex, vec4 shadowCoord)
{
    vec3 v = shadowCoord.xyz / shadowCoord.z;
    float shadow = chebyshevUpperBound(v.z, texture(tex, v.xy).xy);
    //shadow = fixLightBleed(shadow, 0.1);
    return shadow;
}
float shadowVSM(sampler2DArray tex, vec4 shadowCoord)
{
    vec3 v = shadowCoord.xyz / shadowCoord.z;
    float shadow = chebyshevUpperBound(v.z, texture(tex, v.xyz).xy);
    //shadow = fixLightBleed(shadow, 0.1);
    return shadow;
}
float shadowVSM(samplerCube tex, vec4 shadowCoord)
{
    vec3 v = shadowCoord.xyz / shadowCoord.z;
    float shadow = chebyshevUpperBound(v.z, texture(tex, v.xyz).xy);
    //shadow = fixLightBleed(shadow, 0.1);
    return shadow;
}
#endif

-- filtering.esm
#ifndef __SM_FILTER_ESM_included__
#define2 __SM_FILTER_ESM_included__
// Exponential Shadow Mapping.
float shadowESM(sampler2D tex, vec4 shadowCoord)
{
    const float c = 4.0;
    float shadow = texture2D(tex, shadowCoord.xy).x;
    return clamp(exp(-c * (shadowCoord.z - shadow)), 0.0, 1.0);
}
float shadowESM(sampler2DArray tex, vec4 shadowCoord)
{
    return shadow2DArray(tex, shadowCoord).x;
}
float shadowESM(samplerCube tex, vec4 shadowCoord)
{
    return shadowCube(tex, shadowCoord).x;
}
#endif

-- filtering.4tab
#ifndef __SM_FILTER_4TAB_included__
#define2 __SM_FILTER_4TAB_included__
// Bilinear weighted 4-tap filter
float shadow4Tab(sampler2DShadow tex, float texSize, vec4 shadowCoord)
{
	vec2 pos = mod( shadowCoord.xy*texSize, 1.0);
	vec2 offset = (0.5 - step( 0.5, pos)) / texSize;
	float ret = textureProj( tex, shadowCoord + vec4( offset, 0, 0)) * (pos.x) * (pos.y);
	ret += textureProj( tex, shadowCoord + vec4( offset.x, -offset.y, 0, 0)) * (pos.x) * (1-pos.y);
	ret += textureProj( tex, shadowCoord + vec4( -offset.x, offset.y, 0, 0)) * (1-pos.x) * (pos.y);
	ret += textureProj( tex, shadowCoord + vec4( -offset.x, -offset.y, 0, 0)) * (1-pos.x) * (1-pos.y);
    return ret;
}
float shadow4Tab(sampler2DArrayShadow tex, float texSize, vec4 shadowCoord)
{
	vec2 pos = mod( shadowCoord.xy*texSize, 1.0);
	vec2 offset = (0.5 - step( 0.5, pos)) / texSize;
	float ret = shadow2DArray( tex, shadowCoord + vec4( offset, 0, 0)).x * (pos.x) * (pos.y);
	ret += shadow2DArray( tex, shadowCoord + vec4( offset.x, -offset.y, 0, 0)).x * (pos.x) * (1-pos.y);
	ret += shadow2DArray( tex, shadowCoord + vec4( -offset.x, offset.y, 0, 0)).x * (1-pos.x) * (pos.y);
	ret += shadow2DArray( tex, shadowCoord + vec4( -offset.x, -offset.y, 0, 0)).x * (1-pos.x) * (1-pos.y);
    return ret;
}
float shadow4Tab(samplerCubeShadow tex, float texSize, vec4 shadowCoord)
{
	vec2 pos = mod( shadowCoord.xy*texSize, 1.0);
	vec2 offset = (0.5 - step( 0.5, pos)) / texSize;
	float ret = shadowCube( tex, shadowCoord + vec4( offset, 0, 0)).x * (pos.x) * (pos.y);
	ret += shadowCube( tex, shadowCoord + vec4( offset.x, -offset.y, 0, 0)).x * (pos.x) * (1-pos.y);
	ret += shadowCube( tex, shadowCoord + vec4( -offset.x, offset.y, 0, 0)).x * (1-pos.x) * (pos.y);
	ret += shadowCube( tex, shadowCoord + vec4( -offset.x, -offset.y, 0, 0)).x * (1-pos.x) * (1-pos.y);
    return ret;
}
#endif

-- filtering.8tab
#ifndef __SM_FILTER_8TAB_included__
#define2 __SM_FILTER_8TAB_included__
// 8tab
float shadow8TabRand(sampler2DShadow tex, float texSize, vec4 shadowCoord)
{
	float ret = textureProj(tex, shadowCoord);
	for(int i=0; i<7; i++) {
	    ret += textureProj(tex, vec4(
            shadowCoord.xy + shadowOffsetsRand8[i]*2.0/texSize,
            shadowCoord.zw));
	}
    return ret/8.0;
}
float shadow8TabRand(sampler2DArrayShadow tex, float texSize, vec4 shadowCoord)
{
	float ret = shadow2DArray(tex, shadowCoord).x;
	for(int i=0; i<7; i++) {
	    ret += shadow2DArray(tex, vec4(
            shadowCoord.xy + shadowOffsetsRand8[i]*2.0/texSize,
            shadowCoord.zw)).x;
	}
    return ret/8.0;
}
float shadow8TabRand(samplerCubeShadow tex, float texSize, vec4 shadowCoord)
{
	float ret = shadowCube(tex, shadowCoord).x;
	for(int i=0; i<7; i++) {
	    ret += shadowCube(tex, vec4(
            shadowCoord.xy + shadowOffsetsRand8[i]*2.0/texSize,
            shadowCoord.zw)).x;
	}
    return ret/8.0;
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
// offset vector for random sampling
const vec2 shadowOffsetsRand8[7] = vec2[](
    vec2(0.079821, 0.165750),
    vec2(-0.331500, 0.159642),
    vec2(-0.239463, -0.497250),
    vec2(0.662999, -0.319284),
    vec2(0.399104, 0.828749),
    vec2(-0.994499, 0.478925),
    vec2(-0.558746, -1.160249)
);

#include shadow_mapping.filtering.4tab
#include shadow_mapping.filtering.8tab
#include shadow_mapping.filtering.gaussian
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
float dirShadowSingle(vec3 posWorld, float depth,
    sampler2DArrayShadow tex, float texSize,
    float shadowFar[__COUNT], mat4 shadowMatrices[__COUNT])
{
    int layer = getShadowLayer(depth, shadowFar);
    return shadow2DArray(tex, dirShadowCoord(layer, posWorld, shadowMatrices[layer])).x;
}
float dirShadow8TabRand(vec3 posWorld, float depth,
    sampler2DArrayShadow tex, float texSize,
    float shadowFar[__COUNT], mat4 shadowMatrices[__COUNT])
{
    int layer = getShadowLayer(depth, shadowFar);
    return shadow8TabRand(tex, texSize, dirShadowCoord(layer, posWorld, shadowMatrices[layer]));
}
float dirShadow4Tab(vec3 posWorld, float depth,
    sampler2DArrayShadow tex, float texSize,
    float shadowFar[__COUNT], mat4 shadowMatrices[__COUNT])
{
    int layer = getShadowLayer(depth, shadowFar);
    return shadow4Tab(tex, texSize, dirShadowCoord(layer, posWorld, shadowMatrices[layer]));
}
float dirShadowGaussian(vec3 posWorld, float depth,
    sampler2DArrayShadow tex, float texSize,
    float shadowFar[__COUNT], mat4 shadowMatrices[__COUNT])
{
    int layer = getShadowLayer(depth, shadowFar);
    return shadowGaussian(tex, dirShadowCoord(layer, posWorld, shadowMatrices[layer]));
}
#endif

-- sampling.point
#include shadow_mapping.filtering.all

vec4 pointShadowCoord(vec3 lightVec, float f, float n)
{
    vec3 absTexco = abs(lightVec);
    float magnitude = max(absTexco.x, max(absTexco.y, absTexco.z));
    return vec4(-lightVec,
        0.5*(1.0 + (f+n)/(f-n) - (2*f*n)/(f-n)/magnitude));
}
float pointShadowSingle(vec3 lightVec, float f, float n, samplerCubeShadow tex, float texSize)
{
    return shadowCube(tex, pointShadowCoord(lightVec,f,n)).x;
}
float pointShadow8TabRand(vec3 lightVec, float f, float n, samplerCubeShadow tex, float texSize)
{
    return shadow8TabRand(tex, texSize, pointShadowCoord(lightVec,f,n));
}
float pointShadow4Tab(vec3 lightVec, float f, float n, samplerCubeShadow tex, float texSize)
{
    return shadow4Tab(tex, texSize, pointShadowCoord(lightVec,f,n));
}
float pointShadowGaussian(vec3 lightVec, float f, float n, samplerCubeShadow tex, float texSize)
{
    return shadowGaussian(tex, pointShadowCoord(lightVec,f,n));
}

-- sampling.spot
#include shadow_mapping.filtering.all

float spotShadowSingle(vec3 posWorld, sampler2DShadow tex, float texSize, mat4 shadowMatrix)
{
    return textureProj(tex, shadowMatrix*vec4(posWorld,1.0));
}
float spotShadow8TabRand(vec3 posWorld, sampler2DShadow tex, float texSize, mat4 shadowMatrix)
{
    return shadow8TabRand(tex, texSize, shadowMatrix*vec4(posWorld,1.0));
}
float spotShadow4Tab(vec3 posWorld, sampler2DShadow tex, float texSize, mat4 shadowMatrix)
{
    return shadow4Tab(tex, texSize, shadowMatrix*vec4(posWorld,1.0));
}
float spotShadowGaussian(vec3 posWorld, sampler2DShadow tex, float texSize, mat4 shadowMatrix)
{
    return shadowGaussian(tex, shadowMatrix*vec4(posWorld,1.0));
}

---------------------
----- Debugging -----
---------------------

-- debug.vs
#version 150

in vec3 in_pos;
out vec2 out_texco;
void main() {
    out_texco = (0.5*vec4(in_pos,1.0) + vec4(0.5)).xy;
    gl_Position = vec4(in_pos,1.0);
}

-- debugDirectional.fs
#version 150
#extension GL_EXT_gpu_shader4 : enable

in vec2 in_texco;
out vec4 output;

uniform float in_shadowLayer;
uniform sampler2DArray in_shadowMap;

void main() {
    output = texture2DArray(in_shadowMap, vec3(in_texco, in_shadowLayer));
}

-- debugPoint.fs
#version 150
#extension GL_EXT_gpu_shader4 : enable

in vec2 in_texco;
out vec4 output;

uniform float in_shadowLayer;
uniform samplerCube in_shadowMap;

void main() {
    if(in_shadowLayer<1.0) {
        output = texture(in_shadowMap, vec3(1.0, in_texco.x, in_texco.y));
    } else if(in_shadowLayer<2.0) {
        output = texture(in_shadowMap, vec3(-1.0, in_texco.x, in_texco.y));
    } else if(in_shadowLayer<3.0) {
        output = texture(in_shadowMap, vec3(in_texco.y, 1.0, in_texco.x));
    } else if(in_shadowLayer<4.0) {
        output = texture(in_shadowMap, vec3(in_texco.y, -1.0, in_texco.x));
    } else if(in_shadowLayer<5.0) {
        output = texture(in_shadowMap, vec3(in_texco.x, in_texco.y, 1.0));
    } else {
        output = texture(in_shadowMap, vec3(in_texco.x, in_texco.y, -1.0));
    }
}

-- debugSpot.fs
#version 150
#extension GL_EXT_gpu_shader4 : enable

in vec2 in_texco;
out vec4 output;

uniform sampler2D in_shadowMap;

void main() {
    output = texture2D(in_shadowMap, in_texco);
}

