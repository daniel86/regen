
-- defines
#ifdef HAS_nor && HAS_tan
#define HAS_TANGENT_SPACE
#endif
#if SHADER_STAGE == tes
#define SAMPLE(T,C) texture(T,INTERPOLATE_VALUE(C))
#else
#define SAMPLE(T,C) texture(T,C)
#endif
#ifndef PI
#define PI 3.14159265
#endif

-- boneTransformation
#ifdef HAS_BONES
#ifdef HAS_boneOffset
in float in_boneOffset;
#endif

mat4 fetchBoneMatrix(int i) {
    int matIndex = i*4;
    return mat4(
        texelFetchBuffer(in_boneMatrices, matIndex),
        texelFetchBuffer(in_boneMatrices, matIndex+1),
        texelFetchBuffer(in_boneMatrices, matIndex+2),
        texelFetchBuffer(in_boneMatrices, matIndex+3)
    );
}

vec4 boneTransformation(vec4 v) {
    vec4 ret = vec4(0.0);
    int boneDataIndex = gl_VertexID*in_numBoneWeights;
    for(int i=0; i<in_numBoneWeights; ++i) {
        // fetch the matrix index and the weight
        vec2 d = texelFetchBuffer(in_boneVertexData, boneDataIndex+i).xy;
#ifdef HAS_INSTANCES
        int matIndex = int(in_boneOffset + d.y);
#else
        int matIndex = int(d.y);
#endif // HAS_INSTANCES
        ret += d.x * fetchBoneMatrix(matIndex) * v;
    }
    return ret;
}
void boneTransformation(vec4 pos, vec4 nor,
        out vec4 posBone, out vec4 norBone)
{
    posBone = vec4(0.0);
    norBone = vec4(0.0);
    int boneDataIndex = gl_VertexID*in_numBoneWeights;
    for(int i=0; i<in_numBoneWeights; ++i) {
        // fetch the matrix index and the weight
        vec2 d = texelFetchBuffer(in_boneVertexData, boneDataIndex+i).xy;
#ifdef HAS_INSTANCES
        mat4 boneMat = fetchBoneMatrix(int(in_boneOffset + d.y));
#else
        mat4 boneMat = fetchBoneMatrix(int(d.y));
#endif // HAS_INSTANCES

        posBone += d.x * boneMat * pos;
        norBone += d.x * boneMat * nor;
    }
}
#endif

-- transformation
#include mesh.boneTransformation

vec4 toWorldSpace(vec4 pos) {
    vec4 pos_ws = pos;
#ifdef HAS_BONES
    pos_ws = boneTransformation(pos_ws);
#endif
#ifdef HAS_modelMatrix
    pos_ws = in_modelMatrix * pos_ws;
#endif
    return pos_ws;
}
void toWorldSpace(vec3 pos, vec3 nor,
        out vec4 posWorld, out vec3 norWorld)
{
    vec4 pos_ws = vec4(pos.xyz,1.0);
    vec4 nor_ws = vec4(nor.xyz,0.0);
#ifdef HAS_BONES
    vec4 pos_bone, nor_bone;
    boneTransformation(pos_ws, nor_ws, pos_bone, nor_bone);
    pos_ws = pos_bone;
    nor_ws = nor_bone;
#endif
#ifdef HAS_modelMatrix
    pos_ws = in_modelMatrix * pos_ws;
    nor_ws = in_modelMatrix * nor_ws;
#endif
    posWorld = pos_ws;
    norWorld = normalize(nor_ws.xyz);
}

vec4 posEyeSpace(vec4 ws) {
#ifdef IGNORE_VIEW_ROTATION
    return vec4(in_viewMatrix[3].xyz,0.0) + ws;
#elif IGNORE_VIEW_TRANSLATION
    return mat4(
        in_viewMatrix[0],
        in_viewMatrix[1],
        in_viewMatrix[2],
        vec3(0.0), 1.0) * ws;
#else
    return in_viewMatrix * ws;
#endif
}

-- vs
#extension GL_EXT_gpu_shader4 : enable
#include mesh.defines
#include textures.defines

#ifdef HAS_TANGENT_SPACE
out vec3 out_tangent;
out vec3 out_binormal;
#endif
#ifndef HAS_TESSELATION
out vec3 out_posWorld;
out vec3 out_posEye;
#endif // !HAS_TESSELATION
#ifdef HAS_nor
out vec3 out_norWorld;
#endif
#ifdef HAS_INSTANCES
out int out_instanceID;
#endif

in vec3 in_pos;
#ifdef HAS_nor
in vec3 in_nor;
#endif
#ifdef HAS_tan
in vec4 in_tan;
#endif

#ifdef HAS_BONES
uniform int in_numBoneWeights;
#endif

uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;
#ifdef HAS_modelMatrix
uniform mat4 in_modelMatrix;
#endif

#include textures.input

#include mesh.transformation

#include textures.mapToVertex

#define HANDLE_IO(i)

void main() {
#ifdef HAS_nor
    vec4 posWorld;
    toWorldSpace(in_pos, in_nor, posWorld, out_norWorld);
#else
    vec4 posWorld = toWorldSpace(vec4(in_pos.xyz,1.0));
#endif

#ifdef HAS_TANGENT_SPACE
    vec4 tanw = toWorldSpace( vec4(in_tan.xyz,0.0) );
    out_tangent = normalize( tanw.xyz );
    out_binormal = normalize( cross(out_norWorld.xyz, out_tangent.xyz) * in_tan.w );
#endif

    // position transformation
#ifndef HAS_TESSELATION
    // allow textures to modify position/normal
  #ifdef HAS_nor
    textureMappingVertex(posWorld.xyz,out_norWorld);
  #else
    textureMappingVertex(posWorld.xyz,vec3(0,1,0));
  #endif
    out_posWorld = posWorld.xyz;
    vec4 posEye = posEyeSpace(posWorld);
    out_posEye = posEye.xyz;
    gl_Position = in_projectionMatrix * posEye;
#else // !HAS_TESSELATION
    gl_Position = posWorld; // let TES do the transformations
#endif // HAS_TESSELATION

#ifdef HAS_INSTANCES
    out_instanceID = gl_InstanceID;
#endif // HAS_INSTANCES

    HANDLE_IO(gl_VertexID);
}

-- tcs
#ifdef GL_ARB_tessellation_shader
#ifdef TESS_IS_ADAPTIVE
#include mesh.defines

layout(vertices=TESS_NUM_VERTICES) out;

#define ID gl_InvocationID

uniform vec2 in_viewport;
uniform mat4 in_viewProjectionMatrix;
uniform vec3 in_cameraPosition;

#include tesselation_shader.tcs

#define HANDLE_IO(i)

void main() {
    tesselationControl();
    gl_out[ID].gl_Position = gl_in[ID].gl_Position;
    HANDLE_IO(gl_InvocationID);
}
#endif
#endif

-- tes
#ifdef GL_ARB_tessellation_shader
#ifdef HAS_TESSELATION
#extension GL_EXT_gpu_shader4 : enable
#include mesh.defines
#include textures.defines

layout(triangles, ccw, fractional_odd_spacing) in;

out vec3 out_posWorld;
out vec3 out_posEye;
#ifdef HAS_nor
out vec3 out_norWorld;
#endif
#ifdef HAS_INSTANCES
out int out_instanceID;
#endif

#ifdef HAS_INSTANCES
in int in_instanceID[ ];
#endif
#ifdef HAS_nor
in vec3 in_norWorld[ ];
#endif

uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;
#ifdef HAS_modelMatrix
uniform mat4 in_modelMatrix;
#endif
#include textures.input

#include tesselation_shader.interpolate

#include mesh.transformation

#include textures.mapToVertex

#define HANDLE_IO(i)

void main() {
    vec4 posWorld = INTERPOLATE_STRUCT(gl_in,gl_Position);
    // allow textures to modify texture/normal
  #ifdef HAS_nor
    out_norWorld = INTERPOLATE_VALUE(in_norWorld);
    textureMappingVertex(posWorld.xyz,out_norWorld);
  #else
    textureMappingVertex(posWorld.xyz,vec3(0,1,0));
  #endif
    out_posWorld = posWorld.xyz;
    vec4 posEye;
    posEye = posEyeSpace(posWorld);
    out_posEye = posEye.xyz;
    gl_Position = in_projectionMatrix * posEye;
#ifdef HAS_INSTANCES
    out_instanceID = in_instanceID[0];
#endif
    HANDLE_IO(0);
}
#endif
#endif

-- fs
#include mesh.defines
#include textures.defines

#ifdef FS_EARLY_FRAGMENT_TEST
layout(early_fragment_tests) in;
#endif

#if SHADING==NONE
out vec4 out_diffuse;
#else
layout(location = 0) out vec4 out_diffuse;
layout(location = 1) out vec4 out_specular;
layout(location = 2) out vec4 out_norWorld;
#endif

in vec3 in_posWorld;
in vec3 in_posEye;
#ifdef HAS_TANGENT_SPACE
in vec3 in_tangent;
in vec3 in_binormal;
#endif
#ifdef HAS_nor
in vec3 in_norWorld;
#endif

#ifdef HAS_col
uniform vec4 in_col;
#endif
uniform vec3 in_cameraPosition;
uniform mat4 in_projectionMatrix;
uniform mat4 in_viewMatrix;
uniform mat4 in_viewProjectionMatrix;

#include material.declaration
#include textures.input

#include textures.mapToFragment
#include textures.mapToLight

void main() {
    vec3 norWorld;
#ifdef HAS_nor
  #ifdef HAS_TWO_SIDES
    norWorld = (gl_FrontFacing ? in_norWorld : -in_norWorld);
  #else
    norWorld = in_norWorld;
  #endif
#else
    norWorld = vec3(0.0,0.0,0.0);
#endif // HAS_NORMAL

#ifdef HAS_col
    out_diffuse = in_col;
#else
    out_diffuse = vec4(1.0);
#endif 
#endif // HAS_COL
    float alpha = 1.0; // XXX: no alpha
    // apply textures to normal/color/alpha
    textureMappingFragment(in_posWorld, norWorld, out_diffuse, alpha);

#if SHADING!=NONE
    // map to [0,1] for rgba buffer
    out_norWorld.xyz = normalize(norWorld)*0.5 + vec3(0.5);
    out_norWorld.w = 1.0;
  #ifdef HAS_MATERIAL
    out_diffuse.rgb *= (in_matAmbient + in_matDiffuse);
    out_specular = vec4(in_matSpecular,0.0);
    float shininess = in_matShininess;
  #else
    out_specular = vec4(0.0);
    float shininess = 0.0;
  #endif
    textureMappingLight(
        in_posWorld,
        norWorld,
        out_diffuse.rgb,
        out_specular.rgb,
        shininess);
  #ifdef HAS_MATERIAL
    out_specular.a = (in_matShininess * shininess)/256.0;
  #else
    out_specular.a = shininess/256.0;
  #endif
#endif
}

