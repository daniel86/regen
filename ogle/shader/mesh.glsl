
-- defines
#ifdef HAS_NORMAL && HAS_TANGENT
#define HAS_TANGENT_SPACE
#endif
#if SHADER_STAGE == tes
#define SAMPLE(T,C) texture(T,INTERPOLATE_VALUE(C))
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
#for NUM_LIGHTS
#define2 __ID ${LIGHT${FOR_INDEX}_ID}
out vec3 out_lightVec${__ID};
out float out_attenuation${__ID};
#endfor
#elif HAS_VERTEX_SHADING
out vec4 out_ambient;
out vec4 out_diffuse;
out vec4 out_specular;
out vec4 out_emission;
out float out_shininess;
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

#ifdef HAS_SHADING
  #ifdef HAS_VERTEX_SHADING
#for NUM_LIGHTS
#define2 __ID ${LIGHT${FOR_INDEX}_ID}
    vec3 lightVec${__ID} = vec3(0.0);
    float attenuation${__ID} = 0.0;
    shadeTransfer${__ID}(lightVec${__ID}, attenuation${__ID}, posWorld);
#endfor
  #else
#for NUM_LIGHTS
#define2 __ID ${LIGHT${FOR_INDEX}_ID}
    shadeTransfer${__ID}(out_lightVec${__ID}, out_attenuation${__ID}, posWorld);
#endfor
  #endif // HAS_VERTEX_SHADING

  #ifdef HAS_VERTEX_SHADING
    out_ambient = vec4(0.0);
    out_diffuse = vec4(0.0);
    out_specular = vec4(0.0);
    out_emission = vec4(0.0);
    out_shininess = 0.0;
    #ifdef HAS_MATERIAL
    out_shininess = in_matShininess;
    #endif
#for NUM_LIGHTS
#define2 __ID ${LIGHT${FOR_INDEX}_ID}
    shade${__ID}(
        lightVec${__ID},
        attenuation${__ID},
        out_ambient,
        out_diffuse,
        out_specular,
        out_emission,
        out_shininess,
        posWorld.xyz, out_norWorld);
#endfor
    #ifdef HAS_MATERIAL
    materialShading(out_ambient,
        out_diffuse,
        out_specular,
        out_emission,
        out_shininess);
    #endif
  #endif // HAS_VERTEX_SHADING
#endif // HAS_SHADING

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
#include mesh.defines
#include light.defines
#include textures.defines

layout(TESS_PRIMITVE, TESS_SPACING, TESS_ORDERING) in;

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
in int in_instanceID[ ];
#endif
#ifdef HAS_NORMAL
in vec3 in_norWorld[ ];
#endif
#ifdef HAS_TANGENT
in vec4 in_tan[ ];
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
    vec4 tangent = INTERPOLATE_VALUE(in_tan);
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
#for NUM_LIGHTS
#define2 __ID ${LIGHT${FOR_INDEX}_ID}
in vec3 in_lightVec${__ID};
in float in_attenuation${__ID};
#endfor
#elif HAS_VERTEX_SHADING
in vec4 in_ambient;
in vec4 in_diffuse;
in vec4 in_specular;
in vec4 in_emission;
in float in_shininess;
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

#ifdef HAS_SHADING || HAS_LIGHT_MAPS
#ifdef HAS_VERTEX_SHADING
    vec4 ambient = in_ambient;
    vec4 diffuse = in_diffuse;
    vec4 specular = in_specular;
    vec4 emission = in_emission;
    float shininess = in_shininess;
#else
    vec4 ambient = vec4(0.0);
    vec4 diffuse = vec4(0.0);
    vec4 specular = vec4(0.0);
    vec4 emission = vec4(0.0);
    float shininess = 0.0;
  #ifdef HAS_MATERIAL
    shininess = in_matShininess;
  #endif
#endif
    textureMappingLight(
        in_posWorld,
        nor,
        ambient,
        diffuse,
        specular,
        emission,
        shininess);
#ifdef HAS_FRAGMENT_SHADING
#for NUM_LIGHTS
#define2 __ID ${LIGHT${FOR_INDEX}_ID}
    shade${__ID}(
        in_lightVec${__ID},
        in_attenuation${__ID},
        ambient,
        diffuse,
        specular,
        emission,
        shininess,
        in_posWorld,
        nor);
#endfor
  #ifdef HAS_MATERIAL
    materialShading(
        ambient,
        diffuse,
        specular,
        emission,
        shininess);
  #endif
#endif
    output = output*(emission + ambient + diffuse) + specular;
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

