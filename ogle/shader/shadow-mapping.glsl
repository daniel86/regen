
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
        tesselationControl();
        // no tesselation
        gl_TessLevelInner[0] *= float(in_useTesselation);
        gl_TessLevelOuter *= float(in_useTesselation);
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

void main(void) {
    // select framebuffer layer
    gl_Layer = gl_InvocationID;
    // emit face
    gl_Position = getPosition(gl_PositionIn[0]);
    EmitVertex();
    gl_Position = getPosition(gl_PositionIn[1]);
    EmitVertex();
    gl_Position = getPosition(gl_PositionIn[2]);
    EmitVertex();
    EndPrimitive();
}

-- update.fs
#ifdef FS_EARLY_FRAGMENT_TEST
layout(early_fragment_tests) in;
#endif
void main() {}

-----------------------------
------ Shadow sampling ------
-----------------------------

-- sampling
#ifdef NUM_SHADOW_MAP_SLICES
int getShadowLayer(float depth, float shadowFar[NUM_SHADOW_MAP_SLICES])
{
    for(int i=0; i<NUM_SHADOW_MAP_SLICES; ++i) {
        if(depth < shadowFar[i]) { return i; }
    }
    return 0;
}
float sampleDirectionalShadow(
        vec3 posWorld,
        float depth,
        sampler2DArrayShadow tex,
        float shadowFar[NUM_SHADOW_MAP_SLICES],
        mat4 shadowMatrices[NUM_SHADOW_MAP_SLICES]
        )
{
    // shadow map selection is done by distance of pixel to the camera.
    int shadowMapIndex = getShadowLayer(depth, shadowFar);
    // transform this fragment's position from world space to scaled light clip space
    // such that the xy coordinates are in [0;1]
    vec4 shadowCoord = shadowMatrices[shadowMapIndex]*vec4(posWorld,1.0);
    shadowCoord.w = shadowCoord.z;
    // tell glsl in which layer to do the look up
    shadowCoord.z = float(shadowMapIndex);

    return shadow2DArray(tex, shadowCoord).x;
}
#endif

float sampleSpotShadow(
        vec3 posWorld,
        sampler2DShadow tex,
        mat4 shadowMatrix
        )
{
    return textureProj(tex, shadowMatrix*vec4(posWorld,1.0));
}

float samplePointShadow(
        vec3 posWorld,
        vec3 lightVec,
        samplerCubeShadow tex,
        mat4 shadowMatrices[6]
        )
{
    vec3 texco = -lightVec;

    // TODO: better depth calculation ?
    // i don't like how the face is selected
    vec3 s = abs(texco);
    // select face by max magnitude of texco
    // note that i optimized out if statements
    int face =int(0.5*(
         int( s.x>=s.y && s.x>=s.z)*(1.0 - sign(texco.x)) +
         int(  s.y>s.x && s.y>=s.z)*(5.0 - sign(texco.y)) +
         int(  s.z>s.x &&  s.z>s.y)*(9.0 - sign(texco.z))));
    vec4 shadowCoord = shadowMatrices[face]*vec4(posWorld,1.0);
    float depthLS = shadowCoord.z/shadowCoord.w;

    return shadowCube(tex, vec4(texco,depthLS)).x;
}

---------------------
----- Debugging -----
---------------------

-- debug.vs
#version 150

in vec3 in_pos;
out vec2 out_texco;
void main() {
    out_texco = (vec4(0.5)*vec4(in_pos,1.0) + vec4(0.5)).xy;
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

