
-----------------------------
------ Shadow rendering -----
-----------------------------

-- update.vs
#include mesh.defines

in vec3 in_pos;

uniform mat4 in_modelMatrix;

in vec4 in_boneWeights;
in ivec4 in_boneIndices;
uniform int in_numBoneWeights;
uniform mat4 in_boneMatrices[NUM_BONES];

void main() {
    vec4 pos_ws = vec4(in_pos.xyz,1.0);
    if(in_numBoneWeights==1) {
        vec4 pos_bs = in_boneMatrices[ in_boneIndices[0] ] * pos_ws;
        gl_Position = in_modelMatrix * pos_bs;
    }
    else {
        vec4 pos_bs = (1.0 - sign(in_numBoneWeights))*pos_ws;
        for(int i=0; i<in_numBoneWeights; ++i) {
            pos_bs += in_boneWeights[i] * in_boneMatrices[in_boneIndices[i]] * pos_ws;
        }
        gl_Position = in_modelMatrix * pos_bs;
    }
}

-- update.tcs
#include mesh.defines

#define ID gl_InvocationID
#define TESS_PRIMITVE triangles
#define TESS_NUM_VERTICES 3
#define TESS_LOD EDGE_DEVICE_DISTANCE

layout(vertices=TESS_NUM_VERTICES) out;

uniform bool in_useTesselation;
uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;

#include tesselation-shader.tcs

void main() {
    if(gl_InvocationID == 0) {
        if(in_useTesselation) {
            tesselationControl();
        } else {
            // no tesselation
            gl_TessLevelInner[0] = 0.0;
            gl_TessLevelOuter = float[4]( 0, 0, 0, 0 );
        }
    }
    gl_out[ID].gl_Position = gl_in[ID].gl_Position;
}

-- update.tes
#include mesh.defines

#define TESS_PRIMITVE triangles
#define TESS_SPACING equal_spacing
#define TESS_ORDERING ccw

layout(TESS_PRIMITVE, TESS_SPACING, TESS_ORDERING) in;

#include tesselation-shader.interpolate

void main() {
    gl_Position = INTERPOLATE_STRUCT(gl_in,gl_Position);
}

-- update.gs
#extension GL_EXT_geometry_shader4 : enable

#define GS_INPUT_PRIMITIVE triangles
#define GS_OUTPUT_PRIMITIVE triangle_strip

layout(GS_INPUT_PRIMITIVE) in;
layout(GS_OUTPUT_PRIMITIVE, max_vertices=3) out;
layout(invocations = NUM_LAYER) in;

uniform mat4 in_shadowViewProjectionMatrix[NUM_LAYER];

vec4 getPosition(vec4 ws) {
    return in_shadowViewProjectionMatrix[gl_InvocationID] * ws;
}

#ifdef USE_VSM
out vec4 out_pos;
#endif

void main(void) {
    // select framebuffer layer
    gl_Layer = gl_InvocationID;
    // emit face
    gl_Position = getPosition(gl_PositionIn[0]);
#ifdef USE_VSM
    out_pos = gl_Position;
#endif
    EmitVertex();
    gl_Position = getPosition(gl_PositionIn[1]);
#ifdef USE_VSM
    out_pos = gl_Position;
#endif
    EmitVertex();
    gl_Position = getPosition(gl_PositionIn[2]);
#ifdef USE_VSM
    out_pos = gl_Position;
#endif
    EmitVertex();
    EndPrimitive();
}

-- update.fs
#ifdef FS_EARLY_FRAGMENT_TEST
layout(early_fragment_tests) in;
#endif

#ifdef USE_VSM
out vec2 output;
in vec4 in_pos;
#endif
#ifdef USE_ESM
out float output;
#endif

void main() {
#ifdef USE_VSM
    float depth = in_pos.z / in_pos.w ;
    // Don't forget to move away from unit cube ([-1,1]) to [0,1] coordinate system
    depth = depth * 0.5 + 0.5;

    output.x = depth;
    output.y = depth * depth;
    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    output.y += 0.25*(dx*dx+dy*dy);
#endif
#ifdef USE_ESM
    float depth = in_pos.z / in_pos.w ;
    output = exp(depth);
#endif
}

-----------------------------
------ Shadow sampling ------
-----------------------------

-- filtering.vsm
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

-- filtering.esm
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

-- filtering.4tab
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

-- filtering.8tab
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

-- filtering.gaussian
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

-- sampling

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

#include shadow-mapping.filtering.4tab
#include shadow-mapping.filtering.8tab
#include shadow-mapping.filtering.gaussian
#include shadow-mapping.filtering.vsm

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

