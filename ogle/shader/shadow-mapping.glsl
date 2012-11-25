
-----------------------------
------ Shadow rendering -----
-----------------------------

-- update.vs
#include mesh.defines

in vec3 in_pos;
in vec3 in_nor;

uniform mat4 in_modelMatrix;
uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;

in vec4 in_boneWeights;
in ivec4 in_boneIndices;
uniform int in_numBoneWeights;
uniform mat4 in_boneMatrices[NUM_BONES];

vec4 boneTransformation(vec4 v) {
    if(in_numBoneWeights==1) {
        return in_boneWeights * in_boneMatrices[in_boneIndices] * v;
    }
    else if(in_numBoneWeights==2) {
        return in_boneWeights.x * in_boneMatrices[in_boneIndices.x] * v +
               in_boneWeights.y * in_boneMatrices[in_boneIndices.y] * v;
    }
    else if(in_numBoneWeights==3) {
        return in_boneWeights.x * in_boneMatrices[in_boneIndices.x] * v +
               in_boneWeights.y * in_boneMatrices[in_boneIndices.y] * v +
               in_boneWeights.z * in_boneMatrices[in_boneIndices.z] * v;
    }
    else {
        return in_boneWeights.x * in_boneMatrices[in_boneIndices.x] * v +
               in_boneWeights.y * in_boneMatrices[in_boneIndices.y] * v +
               in_boneWeights.z * in_boneMatrices[in_boneIndices.z] * v +
               in_boneWeights.w * in_boneMatrices[in_boneIndices.w] * v;
    }
}

vec4 posWorldSpace(vec3 pos) {
    vec4 pos_ws = vec4(pos.xyz,1.0);
    if(in_numBoneWeights>0) {
        pos_ws = boneTransformation(pos_ws);
    }
    if(in_hasModelMat) {
        pos_ws = in_modelMatrix * pos_ws;
    }
    return pos_ws;
}

vec4 posEyeSpace(vec4 ws) {
    if(in_ignoreViewRotation) {
        return vec4(in_viewMatrix[3].xyz,0.0) + ws;
    }
    else if(in_ignoreViewTranslation) {
        return mat4(in_viewMatrix[0], in_viewMatrix[1], in_viewMatrix[2], vec3(0.0), 1.0) * ws;
    }
    else {
        return in_viewMatrix * ws;
    }
}

#define HANDLE_IO(i)

void main() {
    vec4 posWorld = posWorldSpace_(in_pos);
    out_norWorld = norWorldSpace(in_nor);

    if(in_useTesselation) {
        gl_Position = posWorld;
    }
    else {
        textureMappingVertex(posWorld.xyz,out_norWorld);
        gl_Position = in_projectionMatrix * posEyeSpace(posWorld);
    }

    HANDLE_IO(gl_VertexID);
}

-- update.tcs
#include mesh.defines

#define ID gl_InvocationID
#define TESS_PRIMITVE triangles
#define TESS_NUM_VERTICES 3

layout(vertices=TESS_NUM_VERTICES) out;

uniform vec3 in_cameraPosition;
uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;

uniform vec2 in_viewport;

uniform bool in_useTesselation;

#include tesselation-shader.tcs

#define HANDLE_IO(i)

void main() {
    if(gl_InvocationID == 0) {
        if(in_useTesselation) {
            tesselationControl();
        } else {
            // no tesselation
            gl_TessLevelInner[0] = 0;
            gl_TessLevelOuter = float[4]( 0, 0, 0, 0 );
        }
    }
    gl_out[ID].gl_Position = gl_in[ID].gl_Position;
    HANDLE_IO(gl_InvocationID);
}

-- update.tes
#include mesh.defines
#include textures.defines

#define TESS_PRIMITVE triangles
#define TESS_SPACING equal_spacing
#define TESS_ORDERING ccw

layout(TESS_PRIMITVE, TESS_SPACING, TESS_ORDERING) in;

in vec3 in_norWorld[ ];

uniform mat4 in_modelMatrix;
uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;

in vec4 in_boneWeights[ ];
in ivec4 in_boneIndices[ ];
uniform mat4 in_boneMatrices[NUM_BONES];

#include tesselation-shader.interpolate
#include textures.mapToVertex

vec4 posEyeSpace(vec4 ws) {
    if(in_ignoreViewRotation) {
        return vec4(in_viewMatrix[3].xyz,0.0) + ws;
    }
    else if(in_ignoreViewTranslation) {
        return mat4(in_viewMatrix[0], in_viewMatrix[1], in_viewMatrix[2], vec3(0.0), 1.0) * ws;
    }
    else {
        return in_viewMatrix * ws;
    }
}

void main() {
    vec4 posWorld = INTERPOLATE_STRUCT(gl_in,gl_Position);
    vec3 norWorld = INTERPOLATE_VALUE(in_norWorld);
    textureMappingVertex(posWorld.xyz,norWorld);

    gl_Position = in_projectionMatrix * posEyeSpace(posWorld);
}

-- update.gs
#define GS_INPUT_PRIMITIVE triangles
// maximum is emitting the face 6 times,
// one time for each cube face.
#define GS_MAX_VERTICES 18

layout(GS_INPUT_PRIMITIVE) in;
layout(GS_OUTPUT_PRIMITIVE, max_vertices=GS_MAX_VERTICES) out;

uniform int in_numLayers;

void main(void) {
    for(int layer=0; layer<in_numLayers; layer++) {
        // select framebuffer layer
        gl_Layer = layer;
        // emit face
        gl_Position = gl_PositionIn[0];
        EmitVertex();
        gl_Position = gl_PositionIn[1];
        EmitVertex();
        gl_Position = gl_PositionIn[2];
        EmitVertex();
        EndPrimitive();
    }
}

-- update.fs

#ifdef FS_EARLY_FRAGMENT_TEST
layout(early_fragment_tests) in;
#endif

void main() {
    gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}

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

int getShadowFace(vec3 texco)
{
    vec3 s = abs(texco);
    if( all( greaterThanEqual( s.xx, s.yz ) ) ) {
        return int(0.5*(1.0 - sign(texco.x)));
    }
    else if( all( greaterThanEqual( s.yy, s.xz ) ) ) {
        return int(0.5*(5.0 - sign(texco.y)));
    }
    else {
        return int(0.5*(9.0 - sign(texco.z)));
    }
}
float samplePointShadow(
        vec3 posWorld,
        vec3 lightVec,
        float depth,
        samplerCubeShadow tex,
        mat4 shadowMatrices[6]
        )
{
    vec3 texco = -lightVec;

    int face = getShadowFace(texco);
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

