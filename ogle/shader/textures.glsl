-- defines
#ifndef __IS_TEX_DEF_DECLARED
#define2 __IS_TEX_DEF_DECLARED
// texture defines
  #ifndef NUM_TEXTURES
#define NUM_TEXTURES 0
// #undef HAS_TEXTURE
  #else
#define HAS_TEXTURES
  #endif
#for NUM_TEXTURES
#define2 _TEX TEXTURE${FOR_INDEX}
#define HAS_${${_TEX}_MAPTO}_MAP
#endfor
  #ifdef HAS_DISPLACEMENT_MAP || HAS_HEIGHT_MAP
#define HAS_VERTEX_TEXTURE
  #endif
  #ifdef HAS_COLOR_MAP || HAS_ALPHA_MAP || HAS_NORMAL_MAP
#define HAS_FRAGMENT_TEXTURE
  #endif
  #ifdef HAS_AMBIENT_MAP || HAS_EMISSION_MAP || HAS_DIFFUSE_MAP || HAS_SPECULAR_MAP || HAS_LIGHT_MAP || HAS_SHININESS_MAP
#define HAS_LIGHT_TEXTURE
  #endif
#endif // __IS_TEX_DEF_DECLARED

-- input
#ifdef HAS_TEXTURES
#ifndef __IS_TEX_INPUT_DECLARED
#define2 __IS_TEX_INPUT_DECLARED

#include textures.defines

#if SHADER_STAGE == tes
#define __NUM_INPUT_VERTICES TESS_NUM_VERTICES
#elif SHADER_STAGE == gs
#define __NUM_INPUT_VERTICES GS_NUM_VERTICES
#endif

// declare texture input
#for NUM_TEXTURES
#define2 _TEX TEXTURE${FOR_INDEX}
#define2 _NAME ${${_TEX}_NAME};

#ifndef __TEX_${_NAME}__
#define __TEX_${_NAME}__
uniform ${${_TEX}_SAMPLER_TYPE} ${_NAME};
#endif

#if ${_TEX}_MAPPING_NAME == texco_texco
  #define2 _TEXCO ${${_TEX}_TEXCO}
  #define2 _DIM ${${_TEX}_DIM}
  #ifndef __TEXCO_${_TEXCO}
#define __TEXCO_${_TEXCO}
    #ifdef __NUM_INPUT_VERTICES
      #if _DIM == 1
in float in_${_TEXCO}[ ];
      #else
in vec${_DIM} in_${_TEXCO}[ ];
      #endif
    #else
      #if _DIM == 1
in float in_${_TEXCO};
      #else
in vec${_DIM} in_${_TEXCO};
      #endif
    #endif // !_ARRAY
  #endif
#endif
#endfor

#endif // __IS_TEX_INPUT_DECLARED
#endif // HAS_TEXTURES

-- includes
#include textures.defines
#ifndef __IS_TEXCO_DECLARED
#define2 __IS_TEXCO_DECLARED

// include texture mapping functions
#for NUM_TEXTURES
#define2 _TEX TEXTURE${FOR_INDEX}
#define2 _MAPPING ${${_TEX}_MAPPING_KEY}
  #if ${_MAPPING} != textures.texco_texco
#include ${_MAPPING}
  #endif
#endfor

// include texture blending functions
#for NUM_TEXTURES
#define2 _TEX TEXTURE${FOR_INDEX}
  #ifdef ${_TEX}_BLEND_KEY
#include ${${_TEX}_BLEND_KEY}
  #endif
#endfor

// include texture transfer functions
#for NUM_TEXTURES
#define2 _TEX TEXTURE${FOR_INDEX}
  #ifdef ${_TEX}_TRANSFER_KEY
#include ${${_TEX}_TRANSFER_KEY}
  #endif
#endfor
#endif // __IS_TEXCO_DECLARED

-- mapToVertex
#ifdef HAS_VERTEX_TEXTURE
#include textures.includes

void textureMappingVertex(inout vec3 P, inout vec3 N)
{
    // lookup texels
#for NUM_TEXTURES
#define2 _TEX TEXTURE${FOR_INDEX}
  #if ${_TEX}_MAPTO == HEIGHT || ${_TEX}_MAPTO == DISPLACEMENT
    #if ${_TEX}_MAPPING_KEY == textures.texco_texco
    vec4 texel${FOR_INDEX} = SAMPLE( ${${_TEX}_NAME}, in_${${_TEX}_TEXCO} );
    #else
    vec4 texel${FOR_INDEX} = SAMPLE( ${${_TEX}_NAME}, ${${_TEX}_MAPPING_NAME}(P,N) );
    #endif
    #ifdef ${_TEX}_TRANSFER_NAME
    // use a custom transfer function for the texel
    ${${_TEX}_TRANSFER_NAME}(texel${FOR_INDEX});
    #endif // ${_TEX}_TRANSFER_NAME
  #endif
#endfor
    // blend texels with existing values
#for NUM_TEXTURES
#define2 _TEX TEXTURE${FOR_INDEX}
#define2 _BLEND ${${_TEX}_BLEND_NAME}
#define2 _MAPTO ${${_TEX}_MAPTO}
  #if _MAPTO == HEIGHT
    ${_BLEND}( N * texel${FOR_INDEX}.x, P, ${${_TEX}_BLEND_FACTOR} );
  #elif _MAPTO == DISPLACEMENT
    ${_BLEND}( texel${FOR_INDEX}.xyz, P, ${${_TEX}_BLEND_FACTOR} );
  #endif
#endfor
}
#else
#define textureMappingVertex(P,N)
#endif // HAS_VERTEX_TEXTURE

-- mapToFragment
#ifdef HAS_FRAGMENT_TEXTURE
#include textures.includes

void textureMappingFragment(
        inout vec3 P,
        inout vec3 N,
        inout vec4 C, // rgb color
        inout float A // alpha value
) {
    // lookup texels
#for NUM_TEXTURES
#define2 _TEX TEXTURE${FOR_INDEX}
  #if ${_TEX}_MAPTO == COLOR || ${_TEX}_MAPTO == ALPHA || ${_TEX}_MAPTO == NORMAL
    #if ${_TEX}_MAPPING_KEY == textures.texco_texco
    vec4 texel${FOR_INDEX} = texture( ${${_TEX}_NAME}, in_${${_TEX}_TEXCO} );
    #else
    vec4 texel${FOR_INDEX} = texture( ${${_TEX}_NAME}, ${${_TEX}_MAPPING_NAME}(P,N) );
    #endif
    #ifdef ${_TEX}_TRANSFER_NAME
    // use a custom transfer function for the texel
    ${${_TEX}_TRANSFER_NAME}(texel${FOR_INDEX});
    #endif // ${_TEX}_TRANSFER_NAME
  #endif
#endfor
    // blend texels with existing values
#for NUM_TEXTURES
#define2 _TEX TEXTURE${FOR_INDEX}
#define2 _BLEND ${${_TEX}_BLEND_NAME}
#define2 _MAPTO ${${_TEX}_MAPTO}
  #if _MAPTO == COLOR
    ${_BLEND}( texel${FOR_INDEX}, C, ${${_TEX}_BLEND_FACTOR} );
  #elif _MAPTO == ALPHA
    ${_BLEND}( texel${FOR_INDEX}.x, A, ${${_TEX}_BLEND_FACTOR} );
  #elif _MAPTO == NORMAL
    ${_BLEND}( texel${FOR_INDEX}.rgb, N, ${${_TEX}_BLEND_FACTOR} );
  #endif
#endfor
}
#else
#define textureMappingFragment(P,N,C,A)
#endif // HAS_FRAGMENT_TEXTURE

-- mapToLight
#ifdef HAS_LIGHT_TEXTURE
#include textures.includes

void textureMappingLight(
        inout vec3 P,
        inout vec3 N,
        inout vec4 ambient,
        inout vec4 diffuse,
        inout vec4 specular,
        inout vec4 emission,
        inout float shininess)
{
    // lookup texels
#for NUM_TEXTURES
#define2 _TEX TEXTURE${FOR_INDEX}
  #if ${_TEX}_MAPTO == AMBIENT || ${_TEX}_MAPTO == DIFFUSE || ${_TEX}_MAPTO == SPECULAR || ${_TEX}_MAPTO == EMISSION || ${_TEX}_MAPTO == LIGHT || ${_TEX}_MAPTO == SHININESS
    #if ${_TEX}_MAPPING_KEY == textures.texco_texco
    vec4 texel${FOR_INDEX} = texture( ${${_TEX}_NAME}, in_${${_TEX}_TEXCO} );
    #else
    vec4 texel${FOR_INDEX} = texture( ${${_TEX}_NAME}, ${${_TEX}_MAPPING_NAME}(P,N) );
    #endif
    #ifdef ${_TEX}_TRANSFER_NAME
    // use a custom transfer function for the texel
    ${${_TEX}_TRANSFER_NAME}(texel${FOR_INDEX});
    #endif // ${_TEX}_TRANSFER_NAME
  #endif
#endfor
    // blend texels with existing values
#for NUM_TEXTURES
#define2 _TEX TEXTURE${FOR_INDEX}
#define2 _BLEND ${${_TEX}_BLEND_NAME}
#define2 _MAPTO ${${_TEX}_MAPTO}
  #if _MAPTO == AMBIENT
    ${_BLEND}( texel${FOR_INDEX}, ambient, ${${_TEX}_BLEND_FACTOR} );
  #elif _MAPTO == DIFFUSE
    ${_BLEND}( texel${FOR_INDEX}, diffuse, ${${_TEX}_BLEND_FACTOR} );
  #elif _MAPTO == SPECULAR
    ${_BLEND}( texel${FOR_INDEX}, specular, ${${_TEX}_BLEND_FACTOR} );
  #elif _MAPTO == EMISSION
    ${_BLEND}( texel${FOR_INDEX}, emission, ${${_TEX}_BLEND_FACTOR} );
  #elif _MAPTO == SHININESS
    ${_BLEND}( texel${FOR_INDEX}.r, shininess, ${${_TEX}_BLEND_FACTOR} );
  #elif _MAPTO == LIGHT
    ${_BLEND}( texel${FOR_INDEX}, ambient, ${${_TEX}_BLEND_FACTOR} );
    ${_BLEND}( texel${FOR_INDEX}, diffuse, ${${_TEX}_BLEND_FACTOR} );
  #endif
#endfor
}
#else
#define textureMappingLight(P,N,A,D,SPEC,E,SHIN)
#endif // HAS_LIGHT_TEXTURE

-- texco_cube
#ifndef __TEXCO_CUBE__
#define2 __TEXCO_CUBE__
vec3 texco_cube(vec3 P, vec3 N)
{
    return reflect(-P, N);
}
#endif

-- texco_sphere
#ifndef __TEXCO_SPHERE__
#define2 __TEXCO_SPHERE__
vec2 texco_sphere(vec3 P, vec3 N)
{
    vec3 incident = normalize(P - in_cameraPosition.xyz );
    vec3 r = reflect(incident, N);
    float m = 2.0 * sqrt( r.x*r.x + r.y*r.y + (r.z+1.0)*(r.z+1.0) );
    return vec2(r.x/m + 0.5, r.y/m + 0.5);
}
#endif

-- texco_tube
#ifndef __TEXCO_TUBE__
#define2 __TEXCO_TUBE__
vec2 texco_tube(vec3 P, vec3 N)
{
    float PI = 3.14159265358979323846264;
    vec3 r = reflect(normalize(P), N);
    float u,v;
    float len = sqrt(r.x*r.x + r.y*r.y);
    v = (r.z + 1.0f) / 2.0f;
    if(len > 0.0f) u = ((1.0 - (2.0*atan(r.x/len,r.y/len) / PI)) / 2.0);
    else u = 0.0f;
    return vec2(u,v);
}
#endif

-- texco_flat
#ifndef __TEXCO_FLAT__
#define2 __TEXCO_FLAT__
vec2 texco_flat(vec3 P, vec3 N)
{
    vec3 r = reflect(normalize(P), N);
    return vec2( (r.x + 1.0)/2.0, (r.y + 1.0)/2.0);
}
#endif

-- texco_refraction
#ifndef __TEXCO_REFR__
#define2 __TEXCO_REFR__
vec3 texco_refraction(vec3 P, vec3 N)
{
    vec3 incident = normalize(P - in_cameraPosition.xyz );
    return refract(incident, N, in_matRefractionIndex);
}
#endif

-- texco_reflection
#ifndef __TEXCO_REFL__
#define2 __TEXCO_REFL__
vec3 texco_reflection(vec3 P, vec3 N)
{
    vec3 incident = normalize(P - in_cameraPosition.xyz );
    return reflect(incident.xyz, N);
}
#endif

