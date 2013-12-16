
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
#endif
// TODO redundant
#ifndef RENDER_LAYER
#define RENDER_LAYER 1
#endif
#endif
#ifndef RENDER_TARGET
#define RENDER_TARGET 2D
#endif
#if RENDER_TARGET == 2D
#ifndef HAS_TESSELATION
#define VS_CAMERA_TRANSFORM
#endif
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

-- model-transformation
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
#ifdef VS_CAMERA_TRANSFORM
out vec3 out_posWorld;
out vec3 out_posEye;
#endif
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

#ifdef HAS_modelMatrix
uniform mat4 in_modelMatrix;
#endif

#include regen.states.camera.input
#include regen.meshes.mesh.model-transformation
#ifdef VS_CAMERA_TRANSFORM
#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen
#endif
#include regen.utility.textures.input
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

#ifndef HAS_TESSELATION
    // allow textures to modify position/normal
  #ifdef HAS_nor
    textureMappingVertex(posWorld.xyz,out_norWorld);
  #else
    textureMappingVertex(posWorld.xyz,vec3(0,1,0));
  #endif
#endif // HAS_TESSELATION

#ifdef VS_CAMERA_TRANSFORM
    vec4 posEye  = transformWorldToEye(posWorld,0);
    gl_Position  = transformEyeToScreen(posEye,0);
    out_posWorld = posWorld.xyz;
    out_posEye   = posEye.xyz;
#else
    gl_Position = posWorld;
#endif

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
#include regen.states.camera.input
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

#if RENDER_LAYER == 1
#define TES_CAMERA_TRANSFORM
#endif

layout(triangles, ccw, fractional_odd_spacing) in;

#ifdef TES_CAMERA_TRANSFORM
out vec3 out_posWorld;
out vec3 out_posEye;
#endif
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

#ifdef HAS_modelMatrix
uniform mat4 in_modelMatrix;
#endif
#include regen.states.camera.input

#include regen.utility.textures.input
#include regen.utility.tesselation.interpolate
#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen
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
  
#ifdef TES_CAMERA_TRANSFORM
    vec4 posEye = transformWorldToEye(posWorld,0);
    gl_Position = transformEyeToScreen(posEye,0);
    out_posWorld = posWorld.xyz;
    out_posEye = posEye.xyz;
#endif
    
#ifdef HAS_INSTANCES
    out_instanceID = in_instanceID[0];
#endif
    HANDLE_IO(0);
}
#endif
#endif

-- gs
// TODO redundant
#if RENDER_LAYER > 1
#extension GL_EXT_geometry_shader4 : enable

layout(triangles) in;
#if RENDER_TARGET == CUBE
layout(triangle_strip, max_vertices=18) out;
#elif RENDER_TARGET == 2D_ARRAY
// TODO: use ${RENDER_LAYER}*3
layout(triangle_strip, max_vertices=9) out;
#else
layout(triangle_strip, max_vertices=9) out;
#endif

#include regen.meshes.mesh.defines
#include regen.states.camera.input

out vec3 out_posWorld;
out vec3 out_posEye;

#define HANDLE_IO(i)

#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen

void emitVertex(vec4 posWorld, mat4 view, mat4 proj, int index) {
  vec4 posEye = transformWorldToEye(posWorld, view);
  out_posWorld = posWorld.xyz;
  out_posEye = posEye.xyz;
  gl_Position = proj * posEye;
  HANDLE_IO(index);
  
  EmitVertex();
}

void main() {
  mat4 view, proj;
  
#for LAYER to ${RENDER_LAYER}
#ifndef SKIP_LAYER${LAYER}
  // select framebuffer layer
  gl_Layer = ${LAYER};
#if RENDER_TARGET == CUBE
  view = in_viewMatrix[${LAYER}];
  proj = in_projectionMatrix;
#elif RENDER_TARGET == 2D_ARRAY
  view = in_viewMatrix;
  proj = in_projectionMatrix[${LAYER}];
#endif
  emitVertex(gl_PositionIn[0], view, proj, 0);
  emitVertex(gl_PositionIn[1], view, proj, 1);
  emitVertex(gl_PositionIn[2], view, proj, 2);
  EndPrimitive();
#endif
#endfor
}
#endif

-- fs
#if OUTPUT_TYPE == DEPTH
#include regen.meshes.mesh.fs-depth
#elif OUTPUT_TYPE == COLOR
#define SHADING NONE
#include regen.meshes.mesh.fs-color
#elif OUTPUT_TYPE == VARIANCE
#include regen.meshes.mesh.fs-variance
#else
#include regen.meshes.mesh.fs-deferred
#endif

-- fs-depth
// nothing to do in the fragment shader.
// TODO: texture mapping methods modifying depth
void main() {}

-- fs-moments
out vec4 out_color;

#if RENDER_TARGET != 2D_ARRAY
uniform float in_lightFar;
uniform float in_lightNear;

// TODO: redundant
float linearizeDepth(float expDepth, float n, float f) {
    float z_n = 2.0*expDepth - 1.0;
    return (2.0*n)/(f+n - z_n*(f-n));
}
#endif

void main()
{
    float depth = gl_FragDepth;
#if RENDER_TARGET == 2D_ARRAY
    // no need to linearize for ortho projection

#else
    // Perspective projection saves depth none linear.
    // Linearize it for shadow comparison.
    depth = clamp( linearizeDepth(
        depth, in_lightNear, in_lightFar), 0.0, 1.0 );
#endif

    // Rate of depth change in texture space.
    // This will actually compare local depth with the depth calculated for
    // neighbor texels.
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    out_color = vec4(depth, depth*depth + 0.25*(dx*dx+dy*dy), 1.0, 1.0);
}

-- fs-deferred
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
#include regen.states.camera.input

#include regen.meshes.mesh.material
#include regen.utility.textures.input

#define HANDLE_IO(i)

#include regen.utility.textures.mapToFragment
#include regen.utility.textures.mapToLight

void main() {
#ifdef HAS_clipPlane
    // TODO: more generic clip support
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
