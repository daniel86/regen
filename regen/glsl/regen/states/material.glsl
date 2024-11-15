
-- defines
#ifndef material_defines_DEFINED
#define material_defines_DEFINED
#include regen.states.textures.defines
#ifdef HAS_matEmission || HAS_EMISSION_MAP
#define HAS_MATERIAL_EMISSION
#endif
#endif

-- input
#ifndef material_input_DEFINED
#define material_input_DEFINED
#ifdef HAS_MATERIAL
uniform vec3 in_matAmbient;
uniform vec3 in_matDiffuse;
uniform vec3 in_matSpecular;
//uniform vec3 in_matEmission;
uniform float in_matShininess;
uniform float in_matRefractionIndex;
uniform float in_matAlpha;
#endif
#endif

-- type
#ifndef Material_STRUCT_DEFINED
#define Material_STRUCT_DEFINED
#include regen.states.material.defines
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    float occlusion;
#ifdef HAS_MATERIAL_EMISSION
    vec3 emission;
#endif
};
#endif
