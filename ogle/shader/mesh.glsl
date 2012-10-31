
-- defines
#ifdef HAS_NORMAL && HAS_TANGENT
#define HAS_TANGENT_SPACE
#endif
#if SHADER_STAGE == tes
#define SAMPLE(T,C) texture(T,interpolate(C))
#else
#define SAMPLE(T,C) texture(T,C)
#endif

-- transformation
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
    return mat4(
        in_viewMatrix[0],
        in_viewMatrix[1],
        in_viewMatrix[2],
        vec3(0.0), 1.0) * ws;
#else
    return in_viewMatrix * ws;
#endif
}
#ifdef HAS_TANGENT_SPACE
vec3 posTanSpace(vec3 t, vec3 b, vec3 n, vec3 v) {
    return vec3(dot( v, t ), dot( v, b ), dot( v, n ));
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

-- vs
#include mesh.defines
#include light.defines
#include textures.defines

#ifdef HAS_LIGHT
#include light.types
#endif

#ifndef HAS_TESSELATION
out vec3 out_posWorld;
out vec3 out_posEye;
#ifdef HAS_TANGENT_SPACE
out vec3 out_posTan;
#endif
#endif // !HAS_TESSELATION
#ifdef HAS_NORMAL
out vec3 out_norWorld;
#endif
#ifdef HAS_INSTANCES
out int out_instanceID;
#endif
#ifdef HAS_FRAGMENT_SHADING
out LightProperties out_lightProperties;
#elif HAS_VERTEX_SHADING
out Shading out_shading;
#endif

in vec3 in_pos;
#ifdef HAS_NORMAL
in vec3 in_nor;
#endif
#ifdef HAS_TANGENT
in vec4 in_tan;
#endif
#ifdef HAS_BONES
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

uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;
#ifdef HAS_MODELMAT
uniform mat4 in_modelMatrix;
#endif
#ifdef HAS_BONES
uniform mat4 in_boneMatrices[NUM_BONES];
#endif
#include textures.input
#ifdef HAS_VERTEX_SHADING
#include material.input
#endif

#include mesh.transformation

#ifdef HAS_LIGHT
#include light.init
#endif
#ifdef HAS_VERTEX_SHADING
#include light.apply
#include material.apply
#endif

#include textures.mapToVertex

#define HANDLE_IO(i)

void main() {
    vec4 posWorld = posWorldSpace(in_pos);
#ifdef HAS_NORMAL
    out_norWorld = norWorldSpace(in_nor);
#endif

    // position transformation
#ifndef HAS_TESSELATION
    // allow textures to modify position/normal
  #ifdef HAS_NORMAL
    textureMappingVertex(posWorld,out_nor);
  #else
    textureMappingVertex(posWorld,vec3(0,1,0));
  #endif
    out_posWorld = posWorld.xyz;
    vec4 posEye = posEyeSpace(posWorld);
    out_posEye = posEye.xyz;
  #ifdef HAS_TANGENT_SPACE
    vec3 T = normalize( in_tan.xyz );
    vec3 B = cross(in_nor, in_tan.xyz) * in_tan.w;
    out_posTan = posTanSpace(T, B, out_norWorld, posWorld.xyz).xyz;
  #endif
    gl_Position = in_projectionMatrix * posEye;
#else // !HAS_TESSELATION
    gl_Position = posWorld; // let TES do the transformations
#endif // HAS_TESSELATION

#ifdef HAS_INSTANCES
    out_instanceID = gl_InstanceID;
#endif // HAS_INSTANCES

#ifdef HAS_VERTEX_SHADING
    out_shading = Shading(vec4(0.0),vec4(0.0),vec4(0.0),vec4(0.0),0.0);
    // calculate shading for FS
    LightProperties lightProperties;
    shadeProperties(lightProperties, posWorld);
    // per vertex shading
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
#endif // HAS_VERTEX_SHADING

    HANDLE_IO(gl_VertexID);
}

--------------------------------------------
--------- Tesselation Control --------------
--------------------------------------------

-- tcs
#include mesh.defines

layout(vertices=TESS_NUM_VERTICES) out;

#ifdef HAS_LIGHT
#include light.types
#endif

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
#include mesh.defines
#include light.defines
#include textures.defines

layout(TESS_PRIMITVE, TESS_SPACING, TESS_ORDERING) in;

#ifdef HAS_LIGHT
#include light.types
#endif

out vec3 out_posWorld;
out vec3 out_posEye;
#ifdef HAS_TANGENT_SPACE
out vec3 out_posTan;
#endif
#ifdef HAS_NORMAL
out vec3 out_norWorld;
#endif
#ifdef HAS_INSTANCES
out int out_instanceID;
#endif

#ifdef HAS_INSTANCES
in int in_instanceID[TESS_NUM_VERTICES];
#endif
#ifdef HAS_NORMAL
in vec3 in_norWorld[TESS_NUM_VERTICES];
#endif
#ifdef HAS_TANGENT
in vec4 in_tan[TESS_NUM_VERTICES];
#endif
#ifdef HAS_BONES
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

uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;
#ifdef HAS_MODELMAT
uniform mat4 in_modelMatrix;
#endif
#ifdef HAS_BONES
uniform mat4 in_boneMatrices[NUM_BONES];
#endif
#include textures.input
#ifdef HAS_VERTEX_SHADING
#include material.input
#endif

#include tesselation-shader.interpolate
#include light.interpolate

#include mesh.transformation

#include textures.mapToVertex

#define HANDLE_IO(i)

void main() {
    vec4 posWorld = INTERPOLATE_STRUCT(gl_in,gl_Position);
    // allow textures to modify texture/normal
  #ifdef HAS_NORMAL
    out_norWorld = INTERPOLATE_VALUE(in_norWorld);
    textureMappingVertex(posWorld.xyz,out_norWorld);
  #else
    textureMappingVertex(posWorld.xyz,vec3(0,1,0));
  #endif
    out_posWorld = posWorld.xyz;
    vec4 posEye;
    posEye = posEyeSpace(posWorld);
    out_posEye = posEye.xyz;
  #ifdef HAS_TANGENT_SPACE
    vec4 tangent = interpolate(in_tan);
    vec3 T = normalize( tangent.xyz );
    vec3 B = cross(out_norWorld, T) * tangent.w;
    out_posTan = posTanSpace(T, B, out_norWorld, posWorld.xyz).xyz;
  #endif
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

#ifdef HAS_LIGHT
#include light.types
#endif

#define HANDLE_IO(i)

void main() {}

--------------------------------------------
--------- Fragment Shader ------------------
--------------------------------------------

-- fs
#include mesh.defines
#include light.defines
#include textures.defines

#ifdef FS_EARLY_FRAGMENT_TEST
layout(early_fragment_tests) in;
#endif

#include light.types

out vec4 output;

in vec3 in_posWorld;
in vec3 in_posEye;
#ifdef HAS_TANGENT_SPACE
in vec3 in_posTan;
#endif
#ifdef HAS_NORMAL
in vec3 in_norWorld;
#endif
#ifdef HAS_FRAGMENT_SHADING
in LightProperties in_lightProperties;
#elif HAS_VERTEX_SHADING
in Shading in_shading;
#endif

#ifdef HAS_COLOR
uniform vec4 in_col;
#endif
#ifdef HAS_FOG
uniform vec4 in_fogColor;
uniform float in_fogEnd;
uniform float in_fogScale;
#endif
uniform vec3 in_cameraPosition;
#include material.input
#include light.input
#include textures.input

#ifdef HAS_FRAGMENT_SHADING
#include light.apply
#include material.apply
#endif

#include textures.mapToFragment
#include textures.mapToLight

void main() {
#ifdef HAS_FRAGMENT_SHADING && HAS_TWO_SIDES
    vec3 nor = (gl_FrontFacing ? in_norWorld : -in_norWorld);
#elif HAS_NORMAL
    vec3 nor = in_norWorld;
#else
    vec3 nor = vec3(0.0);
#endif
#ifdef HAS_MATERIAL && HAS_ALPHA
    float alpha = in_matAlpha;
#else
    float alpha = 1.0;
#endif
#ifdef HAS_COL
    output = in_col;
#else
    output = vec4(1);
#endif // HAS_COL

    // apply textures to normal/color/alpha
    textureMappingFragment(in_posWorld, nor, output, alpha);

#ifdef HAS_FRAGMENT_SHADING
    Shading sh = Shading(vec4(0.0),vec4(0.0),vec4(0.0),vec4(0.0),0.0);
  #ifdef HAS_MATERIAL
    sh.shininess = in_matShininess;
  #endif
    textureMappingLight(in_posWorld, nor, sh);
    shade(in_lightProperties,sh,in_posWorld,nor);
  #ifdef HAS_MATERIAL
    materialShading(sh);
  #endif

#elif HAS_VERTEX_SHADING
    Shading sh = in_shading;
    textureMappingLight(in_posWorld, nor, sh);

#elif HAS_LIGHT_MAPS
    Shading sh = Shading(vec4(0.0),vec4(0.0),vec4(0.0),vec4(0.0),0.0);
    textureMappingLight(in_posWorld, nor, sh);

#endif

#ifdef HAS_SHADING || HAS_LIGHT_MAPS
    output = output*(sh.emission + sh.ambient + sh.diffuse) + sh.specular;
#endif
#ifdef HAS_ALPHA
    output.a = output.a * alpha;
#endif

#ifdef HAS_FOG
    // apply fog
    float fogVar = clamp(in_fogScale*(in_fogEnd + gl_FragCoord.z), 0.0, 1.0);
    output = mix(in_fogColor, output, fogVar);
#endif
}

