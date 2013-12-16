
-- defines
#include regen.states.camera.defines
#include regen.utility.textures.defines
#include regen.defines.all

-- vs
#extension GL_EXT_gpu_shader4 : enable
#include regen.meshes.mesh.defines

in vec3 in_pos;
#ifdef HAS_nor
in vec3 in_nor;
#endif
#ifdef HAS_tan
in vec4 in_tan;
#endif
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

#include regen.states.camera.input
#include regen.utility.textures.input

#include regen.states.model.transformModel
#ifdef VS_CAMERA_TRANSFORM
#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen
#endif

#include regen.utility.textures.mapToVertex

#define HANDLE_IO(i)

void main() {
    vec4 posWorld = transformModel(vec4(in_pos.xyz,1.0));
#ifdef HAS_nor
    out_norWorld = normalize(transformModel(vec4(in_nor.xyz,0.0)).xyz);
#endif
#ifdef HAS_TANGENT_SPACE
    vec4 tanw = transformModel( vec4(in_tan.xyz,0.0) );
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
#include regen.utility.tesselation.tcs

-- tes
#ifdef HAS_tessellation_shader
#ifdef HAS_TESSELATION
#extension GL_EXT_gpu_shader4 : enable
#include regen.meshes.mesh.defines

layout(triangles, ccw, fractional_odd_spacing) in;

#ifdef HAS_INSTANCES
in int in_instanceID[ ];
#endif
#ifdef HAS_nor
in vec3 in_norWorld[ ];
#endif
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
#endif // HAS_TESSELATION
#endif // HAS_tessellation_shader

-- gs
#include regen.default.gs-geometry

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
// Nothing to do in the fragment shader.
// TODO: There could be methods modifying gl_FragDepth
void main() {}

-- fs-moments
out vec4 out_color;

#if RENDER_TARGET != 2D_ARRAY
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
    depth = clamp( linearizeDepth(depth, __NEAR__, __FAR__), 0.0, 1.0 );
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
#include regen.states.material.input
#include regen.states.clipping.input
#include regen.utility.textures.input

#ifdef HAS_CLIPPING
#include regen.states.clipping.isClipped
#endif

#include regen.utility.textures.mapToFragment
#include regen.utility.textures.mapToLight

#define HANDLE_IO(i)

void main() {
#ifdef HAS_CLIPPING
    if(isClipped(in_posWorld)) discard;
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
