
--------------------------------
--------------------------------
----- Render mesh to TBuffer targets using alpha component.
--------------------------------
--------------------------------
-- vs
#include regen.meshes.mesh.vs
-- tcs
#include regen.meshes.mesh.tcs
-- tes
#include regen.meshes.mesh.tes
-- gs
#include regen.meshes.mesh.gs
-- fs
#extension GL_EXT_gpu_shader4 : enable
#include regen.meshes.mesh.defines
#include regen.utility.textures.defines

#ifdef FS_EARLY_FRAGMENT_TEST
layout(early_fragment_tests) in;
#endif

layout(location = 0) out vec4 out_color;
#ifdef USE_AVG_SUM_ALPHA
layout(location = 1) out vec2 out_counter;
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
#include regen.meshes.mesh.material
#if SHADING!=NONE
#include regen.shading.direct.inputs
uniform vec3 in_ambientLight;
#endif
#include regen.utility.textures.input

#include regen.utility.textures.mapToFragment
#include regen.utility.textures.mapToLight

#if SHADING!=NONE
#include regen.shading.direct.shade
#endif
#include regen.shading.transparency.writeOutputs

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
    vec4 color = in_col;
#else
    vec4 color = vec4(1.0);
#endif 
#endif // HAS_COL
#ifdef HAS_MATERIAL
    color.a = in_matAlpha;
#endif
    textureMappingFragment(in_posWorld, color, norWorld);
    // discard fragment when alpha smaller than 1/255
    if(color.a < 0.0039) { discard; }

#if SHADING!=NONE
  #ifdef HAS_MATERIAL
    color.rgb *= in_matDiffuse;
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
    
    Shading shading = shade(in_posWorld, norWorld, gl_FragCoord.z, shininess);
    color.rgb *= shading.diffuse;
    color.rgb += specular*shading.specular;
    color.rgb += in_ambientLight*in_matAmbient;
#endif
    
    writeOutputs(color);
}

