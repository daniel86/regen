
-- defines
#include regen.states.camera.defines
#include regen.states.textures.defines
#include regen.defines.all
#ifndef OUTPUT_TYPE
#define OUTPUT_TYPE DEFERRED
#endif

-- vs
#extension GL_EXT_gpu_shader4 : enable
#include regen.models.mesh.defines

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
#include regen.states.textures.input

#include regen.states.model.transformModel
#ifdef VS_CAMERA_TRANSFORM
#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen
#endif

#include regen.states.textures.mapToVertex

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
#include regen.states.tesselation.tcs

-- tes
#ifdef HAS_tessellation_shader
#ifdef HAS_TESSELATION
#extension GL_EXT_gpu_shader4 : enable
#include regen.models.mesh.defines

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
#include regen.states.textures.input

#include regen.states.tesselation.interpolate

#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen

#include regen.states.textures.mapToVertex

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
#include regen.states.camera.defines
#include regen.defines.all
#if RENDER_LAYER > 1
#define2 __MAX_VERTICES__ ${${RENDER_LAYER}*3}

layout(triangles) in;
layout(triangle_strip, max_vertices=${__MAX_VERTICES__}) out;

out vec3 out_posWorld;
out vec3 out_posEye;
flat out int out_layer;

#include regen.states.camera.input
#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen

#define HANDLE_IO(i)

void emitVertex(vec4 posWorld, int index, int layer) {
  vec4 posEye = transformWorldToEye(posWorld,layer);
  out_posWorld = posWorld.xyz;
  out_posEye = posEye.xyz;
  gl_Position = transformEyeToScreen(posEye,layer);
  HANDLE_IO(index);
  
  EmitVertex();
}

void main() {
#for LAYER to ${RENDER_LAYER}
#ifndef SKIP_LAYER${LAYER}
  // select framebuffer layer
  gl_Layer = ${LAYER};
  out_layer = ${LAYER};
  emitVertex(gl_in[0].gl_Position, 0, ${LAYER});
  emitVertex(gl_in[1].gl_Position, 1, ${LAYER});
  emitVertex(gl_in[2].gl_Position, 2, ${LAYER});
  EndPrimitive();
#endif // SKIP_LAYER
#endfor
}
#endif

-- fs-outputs
#ifdef FS_EARLY_FRAGMENT_TEST
layout(early_fragment_tests) in;
#endif
#if OUTPUT_TYPE == DEFERRED
///// Deferred fragment shading
#if SHADING==NONE
out vec4 out_diffuse;
#else
layout(location = 0) out vec4 out_diffuse;
layout(location = 1) out vec4 out_specular;
layout(location = 2) out vec4 out_norWorld;
#endif
#endif
#if OUTPUT_TYPE == TRANSPARENCY
///// Direct fragment shading
#extension GL_EXT_gpu_shader4 : enable
layout(location = 0) out vec4 out_color;
#ifdef USE_AVG_SUM_ALPHA
layout(location = 1) out vec2 out_counter;
#endif
#endif
#if OUTPUT_TYPE == DIRECT
///// Direct fragment shading
#extension GL_EXT_gpu_shader4 : enable
out vec4 out_color;
#endif
#if OUTPUT_TYPE == COLOR
///// Plain color fragment shading
#extension GL_EXT_gpu_shader4 : enable
out vec4 out_color;
#endif

-- fs
#include regen.models.mesh.defines
#include regen.models.mesh.fs-outputs

#if RENDER_LAYER > 1
flat in int in_layer;
#else
#define in_layer 0
#endif

#if OUTPUT_TYPE == DEPTH
///// Depth only output
void main() {}
#endif
#if OUTPUT_TYPE == BLACK
///// Output plain black
out vec4 out_color;
void main() {
  out_color = vec4(0.0,0.0,0.0,1.0);
}
#endif
#if OUTPUT_TYPE == DEFERRED
#include regen.models.mesh.fs-shading
#endif
#if OUTPUT_TYPE == TRANSPARENCY
#include regen.models.mesh.fs-shading
#endif
#if OUTPUT_TYPE == DIRECT
#include regen.models.mesh.fs-shading
#endif
#if OUTPUT_TYPE == COLOR
#include regen.models.mesh.fs-shading
#endif
#if OUTPUT_TYPE == MOMENTS
#include regen.models.mesh.fs-moments
#endif

-- fs-moments
out vec4 out_color;

#if RENDER_TARGET != 2D_ARRAY
#include regen.states.camera.linearizeDepth
#endif

void main()
{
    float depth = gl_FragDepth;
#if RENDER_TARGET == 2D_ARRAY
    // no need to linearize for ortho projection
#else
    // Perspective projection saves depth none linear.
    // Linearize it for shadow comparison.
    depth = clamp( linearizeDepth(2.0*depth-1.0, __NEAR__, __FAR__), 0.0, 1.0 );
#endif
    // Rate of depth change in texture space.
    // This will actually compare local depth with the depth calculated for
    // neighbor texels.
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    out_color = vec4(depth, depth*depth + 0.25*(dx*dx+dy*dy), 1.0, 1.0);
}

-- fs-shading
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
#include regen.states.textures.input

#ifdef HAS_CLIPPING
#include regen.states.clipping.isClipped
#endif
#include regen.states.textures.mapToFragment
#include regen.states.textures.mapToLight

#include regen.models.mesh.writeOutput

#define HANDLE_IO(i)

void main() {
#ifdef HAS_CLIPPING
  if(isClipped(in_posWorld)) discard;
#endif
#if HAS_nor && HAS_TWO_SIDES
  vec3 norWorld = (gl_FrontFacing ? in_norWorld : -in_norWorld);
#elif HAS_nor
  vec3 norWorld = in_norWorld;
#else
  vec3 norWorld = vec3(0.0,0.0,0.0);
#endif
#ifdef HAS_col
  vec4 color = in_col;
#else
  vec4 color = vec4(1.0);
#endif 
#ifdef HAS_MATERIAL
  color.a *= in_matAlpha;
#endif
#endif // HAS_COL
  textureMappingFragment(in_posWorld, color, norWorld);
#ifdef DISCARD_ALPHA
  if (color.a < 0.01) discard;
#endif
  writeOutput(in_posWorld, norWorld, color);
}

-----------------------
-----------------------
-----------------------
-- writeOutput
#if OUTPUT_TYPE == DEFERRED
#include regen.models.mesh.writeOutput-deferred
#endif
#if OUTPUT_TYPE == TRANSPARENCY || OUTPUT_TYPE == DIRECT
#include regen.models.mesh.writeOutput-direct
#endif
#if OUTPUT_TYPE == DEPTH
#define writeOutput(posWorld,norWorld,color)
#endif
#if OUTPUT_TYPE == COLOR
#include regen.models.mesh.writeOutput-color
#endif

-- writeOutput-color
void writeOutput(vec3 posWorld, vec3 norWorld, vec4 color) {
  vec4 diffuse = color;
  vec3 specular = vec3(0.0);
  float shininess = 0.0;
#ifdef HAS_MATERIAL
  diffuse.rgb *= in_matDiffuse;
#endif
  textureMappingLight(posWorld, norWorld,
      diffuse.rgb, specular, shininess);
  out_color = diffuse;
}

-- writeOutput-direct
#if SHADING!=NONE
uniform vec3 in_ambientLight;
#include regen.shading.direct.shade
#endif
void writeOutput(vec3 posWorld, vec3 norWorld, vec4 color) {
  vec4 diffuse = color;
#if SHADING!=NONE
#ifdef HAS_MATERIAL
  diffuse.rgb *= in_matDiffuse;
  vec3 specular = in_matSpecular;
  float shininess = in_matShininess;
#else
  vec3 specular = vec3(0.0);
  float shininess = 0.0;
#endif
  textureMappingLight(posWorld, norWorld,
      diffuse.rgb, specular, shininess);
#ifdef HAS_MATERIAL
  shininess *= in_matShininess;
#endif
  Shading shading = shade(posWorld, norWorld, gl_FragCoord.z, shininess);
  diffuse.rgb *= shading.diffuse +
      specular*shading.specular +
      in_ambientLight*in_matAmbient;
#endif
#ifdef USE_AVG_SUM_ALPHA
  out_color = vec4(diffuse.rgb*diffuse.a,diffuse.a);
  out_counter = vec2(1.0);
#elif USE_SUM_ALPHA
  out_color = vec4(diffuse.rgb*diffuse.a,diffuse.a);
#else
  out_color = diffuse;
#endif
}

-- writeOutput-deferred
void writeOutput(vec3 posWorld, vec3 norWorld, vec4 color) {
  out_diffuse = color;
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
#endif // HAS_MATERIAL
  textureMappingLight(in_posWorld, norWorld,
      out_diffuse.rgb, out_specular.rgb, shininess);
#ifdef HAS_MATERIAL
  out_specular.a = (in_matShininess * shininess)/256.0;
#else
  out_specular.a = shininess/256.0;
#endif // HAS_MATERIAL
#endif // SHADING!=NONE
}
