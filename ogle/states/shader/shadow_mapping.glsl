
-----------------------------
------ VSM
-----------------------------

-- vsm.compute.vs
in vec3 in_pos;

#ifdef IS_2D_SHADOW
out vec2 out_texco;
#else
out vec3 out_texco;
#endif
uniform float in_shadowLayer;

void main() {
    vec2 texco = 0.5*(in_pos.xy+vec2(1.0));
    
#ifdef IS_ARRAY_SHADOW
    out_texco = vec3(in_texco, in_shadowLayer);

#elif IS_CUBE_SHADOW
    if(in_shadowLayer<1.0) {
        out_texco = vec3(1.0, texco.xy);
    } else if(in_shadowLayer<2.0) {
        out_texco = vec3(-1.0, texco.xy);
    } else if(in_shadowLayer<3.0) {
        out_texco = vec3(texco.y, 1.0, texco.x);
    } else if(in_shadowLayer<4.0) {
        out_texco = vec3(texco.y, -1.0, texco.x);
    } else if(in_shadowLayer<5.0) {
        out_texco = vec3(texco.xy, 1.0);
    } else {
        out_texco = vec3(texco.xy, -1.0);
    }

#else // IS_2D_SHADOW
    out_texco = texco;
#endif
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- vsm.compute.fs
out vec2 output;
#ifdef IS_2D_SHADOW
in vec2 in_texco;
#else
in vec3 in_texco;
#endif

#ifdef IS_ARRAY_SHADOW
uniform sampler2DArray in_depthTexture;
#elif IS_CUBE_SHADOW
uniform samplerCube in_depthTexture;
#else // IS_2D_SHADOW
uniform sampler2D in_depthTexture;
#endif

void main()
{
    float depth = texture(in_depthTexture, in_texco).x;
	
    float moment1 = depth;
    float moment2 = depth * depth;

    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    moment2 += 0.25*(dx*dx+dy*dy);
    
    output = vec2(moment1,moment2);
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

// Variance Shadow Mapping.
float chebyshevUpperBound(float dist, vec2 moments)
{
    float variance = max(moments.y - moments.x*moments.x, -0.001);

    float d = dist - moments.x;

    float p_max = linstep(0.2, 1.0, variance / (variance + d*d));
    float p = smoothstep(dist-0.02, dist, moments.x);
    return clamp(max(p, p_max), 0.0, 1.0);
}

float shadowVSM(sampler2D tex, vec4 shadowCoord)
{
    vec2 moments = texture(tex, shadowCoord.xy).xy;
    return chebyshevUpperBound(shadowCoord.z, moments);
}
float shadowVSM(sampler2DArray tex, vec4 shadowCoord)
{
    vec2 moments = texture(tex, shadowCoord.xyz).xy;
    return chebyshevUpperBound(shadowCoord.z, moments);
}
float shadowVSM(samplerCube tex, vec4 shadowCoord)
{
    vec2 moments = texture(tex, shadowCoord.xyz).xy;
    return chebyshevUpperBound(shadowCoord.z, moments);
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
float dirShadowVSM(vec3 posWorld, float depth,
    sampler2DArray tex, float texSize,
    float shadowFar[__COUNT], mat4 shadowMatrices[__COUNT])
{
    int layer = getShadowLayer(depth, shadowFar);
    return shadowVSM(tex, dirShadowCoord(layer, posWorld, shadowMatrices[layer]));
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
float pointShadowVSM(vec3 lightVec, float f, float n, samplerCube tex, float texSize)
{
    return shadowVSM(tex, pointShadowCoord(lightVec,f,n));
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
float spotShadowVSM(vec3 posWorld, sampler2D tex, float texSize, mat4 shadowMatrix)
{
    return shadowVSM(tex, shadowMatrix*vec4(posWorld,1.0));
}

