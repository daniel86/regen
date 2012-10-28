-- material
#ifdef HAS_MATERIAL
uniform vec4 in_matAmbient;
uniform vec4 in_matDiffuse;
uniform vec4 in_matSpecular;
uniform vec4 in_matEmission;
uniform float in_matShininess;
uniform float in_matShininessStrength;
uniform float in_matRefractionIndex;
#if SHADING == ORENNAYER
  uniform float in_matRoughness;
#endif
#if SHADING == MINNAERT
  uniform float in_matDarkness;
#endif
#ifdef HAS_ALPHA
  uniform float in_matAlpha;
#endif

#ifdef HAS_SHADING
void materialShading(inout Shading sh) {
    sh.ambient *= in_matAmbient;
    sh.diffuse *= in_matDiffuse;
    sh.specular *= in_matShininessStrength * in_matSpecular;
    sh.emission *= in_matEmission;
}
#endif
#endif

-- transformation

#ifdef HAS_TANGENT_SPACE
vec3 getTangent(v) {
    return normalize( in_tan.xyz );
}
vec3 getBinormal(vec3 tangent, vec3 nor) {
    return cross(nor, tangent) * in_tan.w;
}
#endif

#ifdef HAS_BONES
vec4 boneTransformation(vec4 v) {
#if NUM_BONE_WEIGTHS==1
    return in_boneWeights * in_boneMatrices[in_boneIndices] * v;
#elif NUM_BONE_WEIGTHS==2
    return in_boneWeights.x * in_boneMatrices[in_boneIndices.x] * v +
           in_boneWeights.y * in_boneMatrices[in_boneIndices.y] * v;
#elif NUM_BONE_WEIGTHS==3
    return in_boneWeights.x * in_boneMatrices[in_boneIndices.x] * v +
           in_boneWeights.y * in_boneMatrices[in_boneIndices.y] * v +
           in_boneWeights.z * in_boneMatrices[in_boneIndices.z] * v;
#else
    return in_boneWeights.x * in_boneMatrices[in_boneIndices.x] * v +
           in_boneWeights.y * in_boneMatrices[in_boneIndices.y] * v +
           in_boneWeights.z * in_boneMatrices[in_boneIndices.z] * v +
           in_boneWeights.w * in_boneMatrices[in_boneIndices.w] * v;
#endif
}
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
    return mat4(in_viewMatrix[0], in_viewMatrix[1], in_viewMatrix[2], vec3(0.0), 1.0) * ws;
#else
    return in_viewMatrix * ws;
#endif
}
#ifdef HAS_TANGENT_SPACE
vec3 posTanSpace(vec3 t, vec3 b, vec3 n, vec3 v) {
    vec3 tanSpace;
    tanSpace.x = dot( v, t );
    tanSpace.y = dot( v, b );
    tanSpace.z = dot( v, n );
    return tanSpace;
}
#endif

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

-- vs.header
#define SAMPLE(TEX,TEXCO) texture(TEX, TEXCO)

#include types.declaration

in vec3 in_pos;
#ifndef HAS_TESSELATION
out vec3 out_posWorld;
out vec3 out_posEye;
  #ifdef HAS_TANGENT_SPACE
out vec3 out_posTan;
  #endif
#endif // !HAS_TESSELATION

#ifdef HAS_BONES
uniform mat4 in_boneMatrices[NUM_BONES];
  #if NUM_BONE_WEIGTHS==1
in float in_boneWeights;
in int in_boneIndices;
  #elif NUM_BONE_WEIGTHS==2
in vec2 in_boneWeights;
in ivec2 in_boneIndices;
  #elif NUM_BONE_WEIGTHS==3
in vec3 in_boneWeights;
in ivec3 in_boneIndices;
  #else
in vec4 in_boneWeights;
in ivec4 in_boneIndices;
  #endif
#endif

#ifdef HAS_INSTANCES
out int out_instanceID;
#endif

#ifdef HAS_NORMAL
in vec3 in_nor;
out vec3 out_norWorld;
#endif

#ifdef HAS_MODELMAT
uniform mat4 in_modelMatrix;
#endif
uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;

#ifdef HAS_FRAGMENT_SHADING
out LightProperties out_lightProperties;
#elif HAS_VERTEX_SHADING
out Shading out_shading;
#endif

#ifdef HAS_VERTEX_SHADING
#include mesh.material
#endif

#include mesh.transformation

#ifdef HAS_VERTEX_SHADING
#include shading.shade
#endif
#include shading.init

-- vs.main
void main() {
    vec4 posWorld = posWorldSpace(in_pos);
#ifdef HAS_NORMAL
    out_norWorld = norWorldSpace(in_nor);
#endif

    // position transformation
#ifndef HAS_TESSELATION
    // allow textures to modify texture/normal
  #ifdef HAS_NORMAL
    modifyTransformation(posWorld,out_nor);
  #else
    modifyTransformation(posWorld,vec3(0,1,0));
  #endif
    out_posWorld = posWorld.xyz;
    vec4 posEye = posEyeSpace(posWorld);
    out_posEye = posEye.xyz;
  #ifdef HAS_TANGENT_SPACE
    out_posTan = posTanSpace(getTangent(), getBinormal(), out_nor, posWorld).xyz;
  #endif
    gl_Position = in_projectionMatrix * posEye;
#else // !HAS_TESSELATION
    gl_Position = posWorld; // let TES do the transformations
#endif // HAS_TESSELATION

#ifdef HAS_INSTANCES
    out_instanceID = gl_InstanceID;
#endif // HAS_INSTANCES

#ifdef HAS_VERTEX_SHADING
    // calculate shading for FS
    LightProperties lightProperties;
    shadeProperties(lightProperties, posWorld);
  #ifdef HAS_TANGENT_SPACE
    // TODO: lightVec to tan space...
  #endif
    // gourad shading
  #ifdef HAS_MATERIAL
    out_shading.shininess = in_matShininess;
  #endif
    shade(lightProperties, out_shading, posWorld.xyz, out_norWorld);
  #ifdef HAS_MATERIAL
    materialShading(out_shading);
  #endif
#elif HAS_FRAGMENT_SHADING
    // calculate light properties for FS
    shadeProperties(out_lightProperties, posWorld);
  #ifdef HAS_TANGENT_SPACE
    // TODO: lightVec to tan space...
  #endif
#endif // HAS_VERTEX_SHADING

    HANDLE_IO(gl_VertexID);
}

--------------------------------------------
--------- Tesselation Control --------------
--------------------------------------------

-- tcs.header
layout(vertices=TESS_NUM_VERTICES) out;

uniform vec2 in_viewport;
uniform mat4 in_viewProjectionMatrix;
uniform vec3 in_cameraPosition;

#include tesselation.tesselationControl
#include types.declaration

-- tcs.main
void main() {
    tesselationControl();
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    HANDLE_IO(gl_InvocationID);
}

--------------------------------------------
--------- Tesselation Evaluation -----------
--------------------------------------------

-- tes.header
#include tesselation.tes
#include types.declaration

out vec3 out_posWorld;
out vec3 out_posEye;
#ifdef HAS_TANGENT_SPACE
out vec3 out_posTan;
#endif
#ifdef HAS_INSTANCES
flat in int in_instanceID[TESS_NUM_VERTICES];
flat out int out_instanceID;
#endif
#ifdef HAS_NORMAL
in vec3 in_norWorld[TESS_NUM_VERTICES];
out vec3 out_norWorld;
#endif

#ifdef HAS_MODELMAT
uniform mat4 in_modelMatrix;
#endif
uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;

#include types.tes.interpolate
#include mesh.transformation

-- tes.main
void main() {
    vec4 posWorld = INTERPOLATE_STRUCT(gl_in,gl_Position);
    // allow textures to modify texture/normal
  #ifdef HAS_NORMAL
    out_norWorld = INTERPOLATE_VALUE(in_norWorld);
    modifyTransformation(posWorld,out_norWorld);
  #else
    modifyTransformation(posWorld,vec3(0,1,0));
  #endif
    out_posWorld = posWorld.xyz;
    vec4 posEye;
    posEye = posEyeSpace(posWorld);
    out_posEye = posEye.xyz;
  #ifdef HAS_TANGENT_SPACE
    out_posTan = posTanSpace(getTangent(), getBinormal(), out_norWorld, posWorld);
  #endif
    gl_Position = in_projectionMatrix * posEye;
#ifdef HAS_INSTANCES
    // flat interpolation
    out_instanceID = in_instanceID[0];
#endif
    HANDLE_IO(0);
}

--------------------------------------------
--------- Geometry Shader ------------------
--------------------------------------------

-- gs.header
layout(GS_INPUT_PRIMITIVE) in;
layout(GS_OUTPUT_PRIMITIVE, max_vertices = GS_MAX_VERTICES) out;
#if GS_NUM_INSTANCES>0
layout(invocations = GS_NUM_INSTANCES) in;
#endif

-- gs.main
void main() {}

--------------------------------------------
--------- Fragment Shader ------------------
--------------------------------------------

-- fs.header
#define SAMPLE(TEX,TEXCO) texture(TEX, TEXCO)

#include types.declaration

#ifdef FS_EARLY_FRAGMENT_TEST
layout(early_fragment_tests) in;
#endif

in vec3 in_posWorld;
in vec3 in_posEye;
#ifdef HAS_TANGENT_SPACE
in vec3 in_posTan;
#endif
#ifdef HAS_INSTANCES
flat in int in_instanceID;
#endif
#ifdef HAS_NORMAL
in vec3 in_norWorld;
#endif
#ifdef HAS_COLOR
uniform vec4 in_col;
#endif
#ifdef HAS_FRAGMENT_SHADING
in LightProperties in_lightProperties;
#elif HAS_VERTEX_SHADING
in Shading in_shading;
#endif

#ifdef HAS_FOG
uniform vec4 in_fogColor;
uniform float in_fogEnd;
uniform float in_fogScale;
#endif

uniform vec3 in_cameraPosition;

#include mesh.material

#ifdef HAS_FRAGMENT_SHADING
#include shading.shade
#endif

out vec4 output;

-- fs.main
void main() {
#ifdef HAS_FRAGMENT_SHADING
  #ifdef HAS_TWO_SIDES
    vec3 nor = (gl_FrontFacing ? in_norWorld : -in_norWorld);
  #else
    vec3 nor = in_norWorld;
  #endif
    modifyNormal(nor);
#endif // HAS_FRAGMENT_SHADING

#ifdef HAS_ALPHA
  #ifdef HAS_MATERIAL
    float alpha = in_matAlpha;
  #else
    float alpha = 1.0;
  #endif
    modifyAlpha(alpha);
#endif // HAS_ALPHA
    
#ifdef HAS_COL
    output = in_col;
#else
    output = vec4(1);
#endif
    modifyColor(output);

#ifdef HAS_FRAGMENT_SHADING
    Shading sh;
  #ifdef HAS_MATERIAL
    sh.shininess = in_matShininess;
  #endif
    modifyLight(sh);
    shade(in_lightProperties,sh,in_posWorld,nor);
  #ifdef HAS_MATERIAL
    materialShading(sh);
  #endif
#elif HAS_VERTEX_SHADING
    Shading sh = in_shading;
    modifyLight(sh);
#elif HAS_LIGHT_MAPS
    Shading sh;
    modifyLight(sh);
#endif

#ifdef HAS_SHADING || HAS_LIGHT_MAPS
    output = output*(sh.emission + sh.ambient + sh.diffuse) + sh.specular;
#endif

#ifdef HAS_ALPHA
    output.a = output.a * alpha;
#endif

#ifdef HAS_FOG
    float fogVar = clamp(in_fogScale*(in_fogEnd + gl_FragCoord.z), 0.0, 1.0);
    output = mix(in_fogColor, output, fogVar);
#endif
}

