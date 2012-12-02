
-- defines
#ifdef HAS_NORMAL && HAS_TANGENT
#define HAS_TANGENT_SPACE
#endif
#if SHADER_STAGE == tes
#define SAMPLE(T,C) texture(T,INTERPOLATE_VALUE(C))
#else
#define SAMPLE(T,C) texture(T,C)
#endif

-- material
#ifdef HAS_MATERIAL
uniform vec3 in_matAmbient;
uniform vec3 in_matDiffuse;
uniform vec3 in_matSpecular;
uniform float in_matShininess;
uniform float in_matRefractionIndex;
#ifdef HAS_ALPHA
uniform float in_matAlpha;
#endif
#endif

-- transformation
#ifdef HAS_BONES
  #ifdef USE_BONE_TBO
mat4 fetchBoneMatrix(int i) {
    int matIndex = i*4;
    return mat4(
        texelFetchBuffer(boneMatrices, matIndex),
        texelFetchBuffer(boneMatrices, matIndex+1),
        texelFetchBuffer(boneMatrices, matIndex+2),
        texelFetchBuffer(boneMatrices, matIndex+3)
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
/*
vec4 boneTransformation(vec4 v) {
    vec4 ret = vec4(0.0);
    int boneDataIndex = gl_VertexID*NUM_BONE_WEIGHTS;
    for(int i=0; i<NUM_BONE_WEIGHTS; ++i) {
        // fetch the matrix index and the weight
        vec2 d = texelFetchBuffer(boneData, boneDataIndex+i).xy;
        ret += d.x * fetchBoneMatrix(int(d.y)*4) * v;
    }
    return ret;
}
*/
#endif

vec4 posWorldSpace(vec3 pos) {
    vec4 pos_ws = vec4(pos.xyz,1.0);
#ifdef HAS_BONES
    pos_ws = boneTransformation(pos_ws);
#endif
#ifdef HAS_MODELMAT
    pos_ws = in_modelMatrix * pos_ws;
#endif
    return pos_ws;
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

vec3 norWorldSpace(vec3 nor) {
    // FIXME normal transform is wrong for scaled objects
    vec4 ws = vec4(nor.xyz,0.0);
#if HAS_BONES
    ws = boneTransformation(ws);
#endif
#ifdef HAS_MODELMAT
    ws = in_modelMatrix * ws;
#endif
    return normalize(ws.xyz);
}

--------------------------------------------
------------- Vertex Shader ----------------
--------------------------------------------

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
#ifdef HAS_NORMAL
out vec3 out_norWorld;
#endif
#ifdef HAS_INSTANCES
out int out_instanceID;
#endif

in vec3 in_pos;
#ifdef HAS_NORMAL
in vec3 in_nor;
#endif
#ifdef HAS_TANGENT
in vec4 in_tan;
#endif

#ifdef HAS_BONES
  #if NUM_BONE_WEIGHTS==1
in float in_boneWeights;
in int in_boneIndices;
  #elif NUM_BONE_WEIGHTS==2
in vec2 in_boneWeights;
in ivec2 in_boneIndices;
  #elif NUM_BONE_WEIGHTS==3
in vec3 in_boneWeights;
in ivec3 in_boneIndices;
  #else
in vec4 in_boneWeights;
in ivec4 in_boneIndices;
  #endif

  #ifndef USE_BONE_TBO
uniform mat4 in_boneMatrices[NUM_BONES];
  #endif
#endif

uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;
#ifdef HAS_MODELMAT
uniform mat4 in_modelMatrix;
#endif

#include textures.input
#ifdef HAS_VERTEX_SHADING
#include mesh.material
#endif

#include mesh.transformation

#include textures.mapToVertex

#define HANDLE_IO(i)

void main() {
    vec4 posWorld = posWorldSpace(in_pos);
#ifdef HAS_NORMAL
    out_norWorld = norWorldSpace(in_nor);
#endif

#ifdef HAS_TANGENT_SPACE
    out_tangent = normalize( in_tan.xyz );
    out_binormal = cross(in_nor, in_tan.xyz) * in_tan.w;
#endif

    // position transformation
#ifndef HAS_TESSELATION
    // allow textures to modify position/normal
  #ifdef HAS_NORMAL
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

--------------------------------------------
--------- Tesselation Control --------------
--------------------------------------------

-- tcs
#include mesh.defines

layout(vertices=TESS_NUM_VERTICES) out;

#define ID gl_InvocationID

uniform vec2 in_viewport;
uniform mat4 in_viewProjectionMatrix;
uniform vec3 in_cameraPosition;

#include tesselation-shader.tcs

#define HANDLE_IO(i)

void main() {
    tesselationControl();
    gl_out[ID].gl_Position = gl_in[ID].gl_Position;
    HANDLE_IO(gl_InvocationID);
}

--------------------------------------------
--------- Tesselation Evaluation -----------
--------------------------------------------

-- tes
#extension GL_EXT_gpu_shader4 : enable
#include mesh.defines
#include textures.defines

layout(TESS_PRIMITVE, TESS_SPACING, TESS_ORDERING) in;

out vec3 out_posWorld;
out vec3 out_posEye;
#ifdef HAS_NORMAL
out vec3 out_norWorld;
#endif
#ifdef HAS_INSTANCES
out int out_instanceID;
#endif

#ifdef HAS_INSTANCES
in int in_instanceID[ ];
#endif
#ifdef HAS_NORMAL
in vec3 in_norWorld[ ];
#endif

#ifdef HAS_BONES
  #if NUM_BONE_WEIGHTS==1
in float in_boneWeights;
in int in_boneIndices;
  #elif NUM_BONE_WEIGHTS==2
in vec2 in_boneWeights;
in ivec2 in_boneIndices;
  #elif NUM_BONE_WEIGHTS==3
in vec3 in_boneWeights;
in ivec3 in_boneIndices;
  #else
in vec4 in_boneWeights;
in ivec4 in_boneIndices;
  #endif

  #ifndef USE_BONE_TBO
uniform mat4 in_boneMatrices[NUM_BONES];
  #endif
#endif

uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;
#ifdef HAS_MODELMAT
uniform mat4 in_modelMatrix;
#endif
#include textures.input

#include tesselation-shader.interpolate

#include mesh.transformation

#include textures.mapToVertex

#define HANDLE_IO(i)

void main() {
    vec4 posWorld = INTERPOLATE_STRUCT(gl_in,gl_Position);
    // allow textures to modify texture/normal
  #ifdef HAS_NORMAL
    out_norWorld = INTERPOLATE_VALUE(in_norWorld);
    textureMappingVertex(posWorld.xyz,out_norWorld);
out_norWorld *= -1; // FIXME: y?
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

--------------------------------------------
--------- Geometry Shader ------------------
--------------------------------------------

-- gs
#include mesh.defines

layout(GS_INPUT_PRIMITIVE) in;
layout(GS_OUTPUT_PRIMITIVE, max_vertices = GS_MAX_VERTICES) out;
#if GS_NUM_INSTANCES>0
layout(invocations = GS_NUM_INSTANCES) in;
#endif

#define HANDLE_IO(i)

void main() {}

--------------------------------------------
--------- Fragment Shader ------------------
--------------------------------------------

-- fs
#include mesh.defines
#include textures.defines

#ifdef FS_EARLY_FRAGMENT_TEST
layout(early_fragment_tests) in;
#endif

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_specular;
layout(location = 2) out vec4 out_norWorld;
layout(location = 3) out vec3 out_posWorld;

in vec3 in_posWorld;
in vec3 in_posEye;
#ifdef HAS_TANGENT_SPACE
in vec3 in_tangent;
in vec3 in_binormal;
#endif
#ifdef HAS_NORMAL
in vec3 in_norWorld;
#endif

#ifdef HAS_COLOR
uniform vec4 in_col;
#endif
uniform vec3 in_cameraPosition;
#include mesh.material
#include textures.input

#include textures.mapToFragment
#include textures.mapToLight

void main() {
    vec3 norWorld;
#ifdef HAS_NORMAL
  #ifdef HAS_TWO_SIDES
    norWorld = (gl_FrontFacing ? in_norWorld : -in_norWorld);
  #else
    norWorld = in_norWorld;
  #endif
#else
    norWorld = vec3(0.0,0.0,0.0);
#endif // HAS_NORMAL

#ifdef HAS_COL
    out_color = in_col;
#else
    out_color = vec4(1.0);
#endif 
#endif // HAS_COL
#ifdef HAS_MATERIAL && HAS_ALPHA
    float alpha = in_matAlpha;
#else
    float alpha = 1.0;
#endif
    // apply textures to normal/color/alpha
    textureMappingFragment(in_posWorld, norWorld, out_color, alpha);

    // map to [0,1] for rgba buffer
    out_norWorld.xyz = normalize(norWorld)*0.5 + vec3(0.5);
  #if SHADING!=NONE
    out_norWorld.w = 1.0;
  #else
    out_norWorld.w = 0.0;
  #endif
    out_posWorld = in_posWorld;
  #ifdef HAS_MATERIAL && SHADING!=NONE
    out_color.rgb *= (in_matAmbient + in_matDiffuse);
    out_specular = vec4(in_matSpecular,0.0);
    float shininess = in_matShininess;
  #else
    out_specular = vec4(0.0);
    float shininess = 0.0;
  #endif
    textureMappingLight(
        in_posWorld,
        norWorld,
        out_color.rgb,
        out_specular.rgb,
        shininess);
  #ifdef HAS_MATERIAL
    out_specular.a = (in_matShininess * shininess)/256.0;
  #else
    out_specular.a = shininess/256.0;
  #endif

#ifdef HAS_ALPHA
    out_color.a = out_color.a * alpha;
#endif
}

