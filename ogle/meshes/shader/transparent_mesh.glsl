
-- vs
#include mesh.vs

-- tcs
#include mesh.tcs

-- tes
#include mesh.tes

-- fs
#extension GL_EXT_gpu_shader4 : enable
#include mesh.defines
#include textures.defines

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
#include material.declaration
#ifdef HAS_MATERIAL
uniform float in_matAlpha;
#endif
#include textures.input

#include textures.mapToFragment
#include textures.mapToLight

#if SHADING!=NONE
#include shading.direct.shade
#endif
#include transparency.writeOutputs

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
    float alpha = in_matAlpha;
#else
    float alpha = 1.0;
#endif
    // apply textures to normal/color/alpha
    textureMappingFragment(in_posWorld, norWorld, color, alpha);
    color.a = color.a * alpha;
    // discard fragment when alpha smaller than 1/255
    if(color.a < 0.0039) { discard; }

#if SHADING!=NONE
  #ifdef HAS_MATERIAL
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

    // TODO: ambient
    Shading shading = shade(in_posWorld, norWorld, gl_FragCoord.z, shininess);
    color.rgb *= shading.diffuse;
    color.rgb += specular*shading.specular;
#endif
    
    writeOutputs(color);
}

