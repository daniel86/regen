
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

-- boneTransformation
#ifdef HAS_BONES
#ifdef HAS_INSTANCES
in float in_boneOffset;
#endif

mat4 fetchBoneMatrix(int i) {
    int matIndex = i*4;
    return mat4(
        texelFetchBuffer(boneMatrices, matIndex),
        texelFetchBuffer(boneMatrices, matIndex+1),
        texelFetchBuffer(boneMatrices, matIndex+2),
        texelFetchBuffer(boneMatrices, matIndex+3)
    );
}

vec4 boneTransformation(vec4 v) {
    vec4 ret = vec4(0.0);
    int boneDataIndex = gl_VertexID*in_numBoneWeights;
    for(int i=0; i<in_numBoneWeights; ++i) {
        // fetch the matrix index and the weight
        vec2 d = texelFetchBuffer(boneVertexData, boneDataIndex+i).xy;
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
        vec2 d = texelFetchBuffer(boneVertexData, boneDataIndex+i).xy;
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
#ifdef HAS_MODELMAT
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
#ifdef HAS_MODELMAT
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
uniform int in_numBoneWeights;
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
#ifdef HAS_NORMAL
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
//out_norWorld *= -1; // FIXME: y?
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
    float alpha = 1.0; // XXX: no alpha
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

//#ifdef HAS_ALPHA
//    out_color.a = out_color.a * alpha;
//#endif
}

-- transparent.vs
#include mesh.vs

-- transparent.tcs
#include mesh.tcs

-- transparent.tes
#include mesh.tes

-- transparent.gs
#include mesh.gs

-- transparent.fsOutputs
layout(location = 0) out vec4 out_color;
#ifdef USE_AVG_SUM_ALPHA
layout(location = 1) out vec2 out_counter;
#endif

-- transparent.writeOutputs
void writeOutputs(vec4 color) {
#ifdef USE_AVG_SUM_ALPHA || USE_SUM_ALPHA
    out_color = vec4(color.rgb*color.a,color.a);
#else
    out_color = color;
#endif
#ifdef USE_AVG_SUM_ALPHA
    out_counter = vec2(1.0);
#endif
}

-- transparent.fs
#extension GL_EXT_gpu_shader4 : enable
#include mesh.defines
#include textures.defines

#ifdef FS_EARLY_FRAGMENT_TEST
layout(early_fragment_tests) in;
#endif

#include mesh.transparent.fsOutputs
#include mesh.transparent.writeOutputs

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

#ifdef HAS_LIGHT
#include light.shade
#endif

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
    vec4 color = in_col;
#else
    vec4 color = vec4(1.0);
#endif 
#endif // HAS_COL
#ifdef HAS_MATERIAL && HAS_ALPHA
    float alpha = in_matAlpha;
#else
    float alpha = 1.0;
#endif
    // apply textures to normal/color/alpha
    textureMappingFragment(in_posWorld, norWorld, color, alpha);
    color.a = color.a * alpha;
    // discard fragment when alpha smaller than 1/255
    if(color.a < 0.0039) { discard; }

  #ifdef HAS_MATERIAL && SHADING!=NONE
    color.rgb *= (in_matAmbient + in_matDiffuse);
    vec3 specular = in_matSpecular;
    float shininess = in_matShininess;
  #else
    vec3 specular = vec3(0.0);
    float shininess = 0.0;
  #endif
    textureMappingLight(
        in_posWorld,
        norWorld,
        color.rgb,
        specular,
        shininess);
  #ifdef HAS_MATERIAL
    shininess *= in_matShininess;
  #endif

#ifdef HAS_LIGHT
    Shading shading = shade(in_posWorld, norWorld, gl_FragCoord.z, shininess);
    color.rgb = color.rgb*(shading.ambient + shading.diffuse) + specular*shading.specular;
#endif
    
    writeOutputs(color);
}

-----------------
-----------------
-----------------

-- spriteSphere.vs
in vec3 in_pos;
in float in_radius;

out float out_sphereRadius;

#define HANDLE_IO(i)

void main() {
    gl_Position = vec4(in_pos,1.0);
    out_sphereRadius = in_radius;

    HANDLE_IO(gl_VertexID);
}


-- spriteSphere.gs
#extension GL_EXT_geometry_shader4 : enable

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;

out vec3 out_posWorld;
out vec3 out_posEye;
out vec2 out_texco;
in float in_sphereRadius[1];

#include sky.sprite

void main() {
    vec3 pos = gl_PositionIn[0].xyz;
    vec3 quadPos[4] = getSpritePoints(pos, in_sphereRadius[0]);

    out_texco = vec2(1.0,0.0);
    out_posWorld = quadPos[0];
    vec4 posEye = in_viewMatrix * vec4(out_posWorld,1.0);
    out_posEye = posEye.xyz;
    gl_Position = in_projectionMatrix * posEye;
    EmitVertex();
    
    out_texco = vec2(1.0,1.0);
    out_posWorld = quadPos[1];
    posEye = in_viewMatrix * vec4(out_posWorld,1.0);
    out_posEye = posEye.xyz;
    gl_Position = in_projectionMatrix * posEye;
    EmitVertex();
        
    out_texco = vec2(0.0,0.0);
    out_posWorld = quadPos[2];
    posEye = in_viewMatrix * vec4(out_posWorld,1.0);
    out_posEye = posEye.xyz;
    gl_Position = in_projectionMatrix * posEye;
    EmitVertex();
        
    out_texco = vec2(0.0,1.0);
    out_posWorld = quadPos[3];
    posEye = in_viewMatrix * vec4(out_posWorld,1.0);
    out_posEye = posEye.xyz;
    gl_Position = in_projectionMatrix * posEye;
    EmitVertex();
    EndPrimitive();
}

-- spriteSphere.fs
#include mesh.defines
#include textures.defines

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_specular;
layout(location = 2) out vec4 out_norWorld;
layout(location = 3) out vec3 out_posWorld;

uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;

//XXX: not interpolated right for point sprite ?
in vec3 in_posWorld;
in vec3 in_posEye;
in vec2 in_texco;

#ifdef HAS_COLOR
uniform vec4 in_col;
#endif
uniform vec3 in_cameraPosition;
#include mesh.material
#include textures.input

#include textures.mapToFragment
#include textures.mapToLight

vec3 fakeSphereNormal(vec2 texco) {
    vec2 x = texco*2.0 - vec2(1.0);
    return vec3(x, sqrt(1.0 - dot(x,x)));
}

void main() {
    // XXX: this is normal eye not world ?!?
    vec3 normal = fakeSphereNormal(in_texco);
    //if(length(in_texco*2.0 - vec2(1.0))>1.0) discard;
    //vec4 norWorld = inverse(in_viewMatrix) * vec4(normal,1.0);
    vec3 norWorld = normal;

#ifdef HAS_COL
    out_color = in_col;
#else
    out_color = vec4(1.0);
#endif
    float alpha = 1.0; // XXX: no alpha
    // apply textures to normal/color/alpha
    textureMappingFragment(in_posWorld, norWorld.xyz, out_color, alpha);
    
    // map to [0,1] for rgba buffer
    out_norWorld.xyz = normalize(norWorld.xyz)*0.5 + vec3(0.5);
    out_norWorld.w = 1.0;
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
        norWorld.xyz,
        out_color.rgb,
        out_specular.rgb,
        shininess);
  #ifdef HAS_MATERIAL
    out_specular.a = (in_matShininess * shininess)/256.0;
  #else
    out_specular.a = shininess/256.0;
  #endif
    out_color = vec4(1.0,0.0,0.0,1.0);
}



