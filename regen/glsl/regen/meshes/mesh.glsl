
-- material
#ifdef HAS_MATERIAL
uniform vec3 in_matAmbient;
uniform vec3 in_matDiffuse;
uniform vec3 in_matSpecular;
uniform float in_matShininess;
uniform float in_matRefractionIndex;
uniform float in_matAlpha;
#endif

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
in int in_boneOffset;
#endif

#ifdef USE_BONE_TBO
mat4 fetchBoneMatrix(uint i) {
#ifdef HAS_INSTANCES
    #ifdef HAS_boneOffset
    int matIndex = (NUM_BONES_PER_MESH*in_boneOffset + int(i))*4;
    #else
    int matIndex = int(i)*4;
    #endif // HAS_boneOffset
#else
    int matIndex = int(i)*4;
#endif // HAS_INSTANCES
    return mat4(
        texelFetchBuffer(in_boneMatrices, matIndex),
        texelFetchBuffer(in_boneMatrices, matIndex+1),
        texelFetchBuffer(in_boneMatrices, matIndex+2),
        texelFetchBuffer(in_boneMatrices, matIndex+3)
    );
}
#else
#define fetchBoneMatrix(i) in_boneMatrices[i]
#endif

vec4 boneTransformation(vec4 v) {
#if NUM_BONE_WEIGHTS==1
    return fetchBoneMatrix(in_boneIndices) * v;
#elif NUM_BONE_WEIGHTS==2
    return in_boneWeights.x * fetchBoneMatrix(in_boneIndices.x) * v +
           in_boneWeights.y * fetchBoneMatrix(in_boneIndices.y) * v;
#elif NUM_BONE_WEIGHTS==3
    return in_boneWeights.x * fetchBoneMatrix(in_boneIndices.x) * v +
           in_boneWeights.y * fetchBoneMatrix(in_boneIndices.y) * v +
           in_boneWeights.z * fetchBoneMatrix(in_boneIndices.z) * v;
#else
    return in_boneWeights.x * fetchBoneMatrix(in_boneIndices.x) * v +
           in_boneWeights.y * fetchBoneMatrix(in_boneIndices.y) * v +
           in_boneWeights.z * fetchBoneMatrix(in_boneIndices.z) * v +
           in_boneWeights.w * fetchBoneMatrix(in_boneIndices.w) * v;
#endif
}
void boneTransformation(vec4 v, vec4 nor,
        out vec4 posBone, out vec4 norBone)
{
  posBone = boneTransformation(v);
  norBone = boneTransformation(nor);
}

#endif

-- transformation
#include regen.meshes.mesh.boneTransformation

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

--------------------------------
--------------------------------
----- Render mesh to GBuffer targets ignoring alpha component.
--------------------------------
--------------------------------
-- vs
#extension GL_EXT_gpu_shader4 : enable
#include regen.meshes.mesh.defines
#include regen.utility.textures.defines

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
#if NUM_BONE_WEIGHTS==1
in float in_boneWeights;
in uint in_boneIndices;
#elif NUM_BONE_WEIGHTS==2
in vec2 in_boneWeights;
in uvec2 in_boneIndices;
#elif NUM_BONE_WEIGHTS==3
in vec3 in_boneWeights;
in uvec3 in_boneIndices;
#else
in vec4 in_boneWeights;
in uvec4 in_boneIndices;
#endif
#ifndef USE_BONE_TBO
uniform mat4 in_boneMatrices[NUM_BONES];
#endif
uniform int in_numBoneWeights;
#endif

uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;
#ifdef HAS_modelMatrix
uniform mat4 in_modelMatrix;
#endif

#include regen.utility.textures.input

#include regen.meshes.mesh.transformation

#include regen.utility.textures.mapToVertex

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
#ifdef HAS_tessellation_shader
#ifdef TESS_IS_ADAPTIVE
#include regen.meshes.mesh.defines

layout(vertices=TESS_NUM_VERTICES) out;

#define ID gl_InvocationID

uniform vec2 in_viewport;
uniform mat4 in_viewProjectionMatrix;
uniform vec3 in_cameraPosition;

#include regen.utility.tesselation.tcs

#define HANDLE_IO(i)

void main() {
    tesselationControl();
    gl_out[ID].gl_Position = gl_in[ID].gl_Position;
    HANDLE_IO(gl_InvocationID);
}
#endif
#endif

-- tes
#ifdef HAS_tessellation_shader
#ifdef HAS_TESSELATION
#extension GL_EXT_gpu_shader4 : enable
#include regen.meshes.mesh.defines
#include regen.utility.textures.defines

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

#include regen.utility.textures.input
#include regen.utility.tesselation.interpolate
#include regen.meshes.mesh.transformation
#include regen.utility.textures.mapToVertex

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
#include regen.meshes.mesh.defines
#include regen.utility.textures.defines

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

#include regen.meshes.mesh.material
#include regen.utility.textures.input

#define HANDLE_IO(i)

#include regen.utility.textures.mapToFragment
#include regen.utility.textures.mapToLight

void main() {
#ifdef HAS_clipPlane
    if(dot(in_posWorld,in_clipPlane.xyz)-in_clipPlane.w<=0.0) discard;
#endif
    HANDLE_IO(0);
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
    textureMappingFragment(in_posWorld, out_diffuse, norWorld);

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
