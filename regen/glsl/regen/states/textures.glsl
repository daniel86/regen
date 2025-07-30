
-- eyeVectorTan
#ifndef REGEN_eyeVectorTan_included_
#define2 REGEN_eyeVectorTan_included_
vec3 eyeVectorTan()
{
    mat3 tbn = mat3(in_tangent,in_binormal,in_norWorld);
    return normalize( tbn * (in_cameraPosition.xyz-in_posWorld) );
}
#endif

-- defines
#ifndef REGEN_IS_TEX_DEF_DECLARED
#define2 REGEN_IS_TEX_DEF_DECLARED
// texture defines
  #ifndef NUM_TEXTURES
#define NUM_TEXTURES 0
// #undef HAS_TEXTURE
  #else
#define HAS_TEXTURES
  #endif
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define HAS_${TEX_MAPTO${_ID}}_MAP
#endfor
  #ifdef HAS_DISPLACEMENT_MAP || HAS_HEIGHT_MAP || HAS_VERTEX_MASK_MAP
#define HAS_VERTEX_TEXTURE
  #endif
  #ifdef HAS_COLOR_MAP || HAS_ALPHA_MAP || HAS_NORMAL_MAP
#define HAS_FRAGMENT_TEXTURE
  #endif
  #ifdef HAS_AMBIENT_MAP || HAS_EMISSION_MAP || HAS_DIFFUSE_MAP || HAS_SPECULAR_MAP || HAS_LIGHT_MAP || HAS_SHININESS_MAP
#define HAS_LIGHT_TEXTURE
  #endif
#endif // _IS_TEX_DEF_DECLARED

-- input
#ifdef HAS_TEXTURES
#ifndef REGEN_IS_TEX_INPUT_DECLARED
#define2 REGEN_IS_TEX_INPUT_DECLARED

#include regen.states.textures.defines

#if SHADER_STAGE == tes
    #define REGEN_NUM_INPUT_VERTICES TESS_NUM_VERTICES
#elif SHADER_STAGE == tcs
    #define REGEN_NUM_INPUT_VERTICES TESS_NUM_VERTICES
#elif SHADER_STAGE == gs
    #define REGEN_NUM_INPUT_VERTICES GS_NUM_VERTICES
#endif

// declare texture input
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _NAME ${TEX_NAME${_ID}}

#ifndef REGEN_TEX_${_NAME}_
#define REGEN_TEX_${_NAME}_
uniform ${TEX_SAMPLER_TYPE${_ID}} in_${_NAME};
#endif

#if TEX_MAPPING_NAME${_ID} == texco_texco && SHADER_STAGE != tcs
  #define2 _TEXCO ${TEX_TEXCO${_ID}}
  #define2 _DIM ${TEX_DIM${_ID}}
  #ifndef REGEN_TEXCO_${_TEXCO}
#define REGEN_TEXCO_${_TEXCO}
    #ifdef REGEN_NUM_INPUT_VERTICES
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

#endif // REGEN_IS_TEX_INPUT_DECLARED
#endif // HAS_TEXTURES

-- includes
#include regen.states.textures.defines
#ifndef REGEN_IS_TEXCO_DECLARED
#define2 REGEN_IS_TEXCO_DECLARED

// include texture mapping functions
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _MAPPING ${TEX_MAPPING_KEY${_ID}}
  #if ${_MAPPING} != regen.states.textures.texco_texco && TEX_MAPPING_KEY${_ID} != regen.states.textures.texco_custom
#include ${_MAPPING}
  #endif
#endfor

// include texture blending functions
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
  #ifdef TEX_BLEND_KEY${_ID}
#include ${TEX_BLEND_KEY${_ID}}
  #endif
#endfor

// include texel/texco transfer functions
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
  #ifdef TEX_TRANSFER_KEY${_ID}
#include ${TEX_TRANSFER_KEY${_ID}}
  #endif
  #ifdef TEXCO_TRANSFER_KEY${_ID}
#include ${TEXCO_TRANSFER_KEY${_ID}}
  #endif
#endfor
#endif // REGEN_IS_TEXCO_DECLARED

-- computeTexco
#if TEX_DIM${_ID}==1
    #define2 TEXCO_TYPE float
#else
    #define2 TEXCO_TYPE vec${TEX_DIM${_ID}}
#endif
#define2 _MAPPING_ ${TEX_MAPPING_NAME${_ID}}

#if _MAPPING_!=texco_texco
    #ifndef REGEN_texco_${_TEXCO_CTX}_${_MAPPING_}_
        #define2 REGEN_texco_${_TEXCO_CTX}_${_MAPPING_}_
    // generate texco
    ${TEXCO_TYPE} texco_${_MAPPING_} = ${_MAPPING_}(P,N);
    #endif // REGEN_texco_${_MAPPING_${_ID}}_

    #ifdef TEXCO_TRANSFER_NAME${_ID}
        #define2 _TRANSFER_ ${TEXCO_TRANSFER_NAME${_ID}}
    // apply transfer function to texco
        #ifndef REGEN_texco_${_MAPPING_}_${_TEXCO_CTX}_${_TRANSFER_}_
            #define2 REGEN_texco_${_MAPPING_}_${_TEXCO_CTX}_${_TRANSFER_}_
    ${TEXCO_TYPE} texco_${_MAPPING_}_${_TRANSFER_} = texco_${_MAPPING_};
    ${_TRANSFER_}( texco_${_MAPPING_}_${_TRANSFER_} );
        #endif // REGEN_texco_${_MAPPING_}_${_TEXCO_CTX}_${TEXCO_TRANSFER_NAME${_ID}}_
        #define2 REGEN_TEXCO${_ID}_ texco_${_MAPPING_}_${_TRANSFER_}
    #else
        #define2 REGEN_TEXCO${_ID}_ texco_${_MAPPING_}
    #endif

#else // _MAPPING_!=texco_texco
    #ifndef REGEN_${TEX_TEXCO${_ID}}_${_TEXCO_CTX}_
        #define2 REGEN_${TEX_TEXCO${_ID}}_${_TEXCO_CTX}_
        #if SHADER_STAGE==tes
    ${TEXCO_TYPE} ${TEX_TEXCO${_ID}} = INTERPOLATE_VALUE(in_${TEX_TEXCO${_ID}});
        #elif SHADER_STAGE==tcs
    ${TEXCO_TYPE} ${TEX_TEXCO${_ID}} = in_${TEX_TEXCO${_ID}}[vertexIndex];
        #else
    ${TEXCO_TYPE} ${TEX_TEXCO${_ID}} = in_${TEX_TEXCO${_ID}};
        #endif
    #endif // REGEN_${TEX_TEXCO${_ID}}_${_TEXCO_CTX}_

    #ifdef TEXCO_TRANSFER_NAME${_ID}
        #define2 _TRANSFER_ ${TEXCO_TRANSFER_NAME${_ID}}
    // apply transfer function to texco
        #ifndef REGEN_${TEX_TEXCO${_ID}}_${_TEXCO_CTX}_${_TRANSFER_}_
            #define2 REGEN_${TEX_TEXCO${_ID}}_${_TEXCO_CTX}_${_TRANSFER_}_
    ${TEXCO_TYPE} ${TEX_TEXCO${_ID}}_${_TRANSFER_} = ${TEX_TEXCO${_ID}};
    ${_TRANSFER_}( ${TEX_TEXCO${_ID}}_${_TRANSFER_} );
        #endif // REGEN_${TEX_TEXCO${_ID}}_${_TEXCO_CTX}_${_TRANSFER_}_
        #define2 REGEN_TEXCO${_ID}_ ${TEX_TEXCO${_ID}}_${_TRANSFER_}
    #else
        #define2 REGEN_TEXCO${_ID}_ ${TEX_TEXCO${_ID}}
    #endif

#ifdef TEX_FLIPPING_MODE${_ID}
    #define2 _FLIP_ ${TEX_FLIPPING_MODE${_ID}}
    #define2 _TEXCO_ ${REGEN_TEXCO${_ID}_}
    #if _FLIP_==x
    ${_TEXCO_}.x = 1.0 - ${_TEXCO_}.x;
    #endif
    #if _FLIP_==y
    ${_TEXCO_}.y = 1.0 - ${_TEXCO_}.y;
    #endif
#endif // _MAPPING_==regen.states.textures.texco_texco

-- sampleTexel
    vec4 texel${INDEX} = texture(in_${TEX_NAME${_ID}}, ${REGEN_TEXCO${_ID}_});
#ifdef TEX_IGNORE_ALPHA${_ID}
    texel${INDEX}.a = 1.0;
#endif
#ifdef TEX_TRANSFER_NAME${_ID}
    // use a custom transfer function for the texel
    ${TEX_TRANSFER_NAME${_ID}}(texel${INDEX});
#endif // TEX_TRANSFER_NAME${_ID}

-- sampleHeight
#ifndef REGEN_SAMPLE_HEIGHT_INCLUDED_
#define2 REGEN_SAMPLE_HEIGHT_INCLUDED_
float sampleHeight(vec2 uv) {
    float accumulatedHeight = 0.0;
#for INDEX to NUM_TEXTURES
    #define2 _ID ${TEX_ID${INDEX}}
    #if TEX_MAPTO${_ID}==HEIGHT
    accumulatedHeight += texture(in_${TEX_NAME${_ID}}, uv).r;
    #endif
#endfor
    return accumulatedHeight;
}
#endif

--------------------------------------
--------------------------------------
---- Texture mapping functions.
--------------------------------------
--------------------------------------

-- applyHeightMaps
#ifndef REGEN_applyHeightMaps_INCLUDED_
#define2 REGEN_applyHeightMaps_INCLUDED_
#include regen.states.textures.includes
// TODO: redundant with below
void applyHeightMaps(inout vec3 P, vec3 N, int vertexIndex)
{
    // compute texco
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _TEXCO_CTX textureMappingVertex
#if TEX_MAPTO${_ID}==HEIGHT
#include regen.states.textures.computeTexco
#endif // ifVertexMapping
#endfor
    // sample texels
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#if TEX_MAPTO${_ID}==HEIGHT
#include regen.states.textures.sampleTexel
#endif // ifVertexMapping
#endfor
    // blend texels with existing values
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _BLEND ${TEX_BLEND_NAME${_ID}}
#define2 _MAPTO ${TEX_MAPTO${_ID}}
  #if _MAPTO == HEIGHT
    ${_BLEND}( N * texel${INDEX}.x * ${TEX_BLEND_FACTOR${_ID}}, P, 1.0 );
  #endif
#endfor
}
#endif

-- mapToVertex
#ifndef HAS_VERTEX_TEXTURE
#define textureMappingVertex(P,N,I)
#else
#include regen.states.textures.includes

void textureMappingVertex(inout vec3 P, inout vec3 N, int vertexIndex)
{
    // compute texco
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _TEXCO_CTX textureMappingVertex
#if TEX_MAPTO${_ID}==HEIGHT || TEX_MAPTO${_ID}==DISPLACEMENT || TEX_MAPTO${_ID}==VERTEX_MASK
#include regen.states.textures.computeTexco
#endif // ifVertexMapping
#endfor
    // sample texels
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#if TEX_MAPTO${_ID}==HEIGHT || TEX_MAPTO${_ID}==DISPLACEMENT || TEX_MAPTO${_ID}==VERTEX_MASK
#include regen.states.textures.sampleTexel
#endif // ifVertexMapping
#endfor
    // blend texels with existing values
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _BLEND ${TEX_BLEND_NAME${_ID}}
#define2 _MAPTO ${TEX_MAPTO${_ID}}
  #if _MAPTO == HEIGHT
    ${_BLEND}( N * texel${INDEX}.x * ${TEX_BLEND_FACTOR${_ID}}, P, 1.0 );
  #elif _MAPTO == DISPLACEMENT
    ${_BLEND}( texel${INDEX}.xyz, P, ${TEX_BLEND_FACTOR${_ID}} );
  #elif _MAPTO == VERTEX_MASK
    out_mask = texel${INDEX}.x;
  #endif
#endfor
}
#endif // HAS_VERTEX_TEXTURE

-- mapToFragment
#ifndef HAS_FRAGMENT_TEXTURE
#define textureMappingFragment(P,C,N)
#else
#include regen.states.textures.includes

void textureMappingFragment(in vec3 P, inout vec4 C, inout vec3 N)
{
    // NOTE: the normal bump mapping must be done first, as other texture
    //       sampling methods may require bumped normals such as refraction textures.
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _TEXCO_CTX textureMappingFragment
#define2 _MAPTO ${TEX_MAPTO${_ID}}
    #if _MAPTO == NORMAL
        #ifndef FS_NO_OUTPUT
    // compute normal map texco
#include regen.states.textures.computeTexco
        #endif
    #endif
#endfor
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _MAPTO ${TEX_MAPTO${_ID}}
    #if _MAPTO == NORMAL
        #ifndef FS_NO_OUTPUT
    // sample normal map
#include regen.states.textures.sampleTexel
        #endif
    #endif
#endfor
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _BLEND ${TEX_BLEND_NAME${_ID}}
#define2 _MAPTO ${TEX_MAPTO${_ID}}
  #if _MAPTO == NORMAL
    #ifndef FS_NO_OUTPUT
    ${_BLEND}( texel${INDEX}.xyz, N, ${TEX_BLEND_FACTOR${_ID}} );
    #endif
  #endif
#endfor

    // compute texco
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _TEXCO_CTX textureMappingFragment
#define2 _MAPTO ${TEX_MAPTO${_ID}}
    #if _MAPTO == COLOR
#include regen.states.textures.computeTexco
    #elif _MAPTO == ALPHA
#include regen.states.textures.computeTexco
    #endif
#endfor
    // sample texels
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _MAPTO ${TEX_MAPTO${_ID}}
    #if _MAPTO == COLOR
#include regen.states.textures.sampleTexel
    #elif _MAPTO == ALPHA
#include regen.states.textures.sampleTexel
    #endif
#endfor
    // blend texels with existing values
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _BLEND ${TEX_BLEND_NAME${_ID}}
#define2 _MAPTO ${TEX_MAPTO${_ID}}
  #if _MAPTO == COLOR
    ${_BLEND}( texel${INDEX}, C, ${TEX_BLEND_FACTOR${_ID}} );
  #elif _MAPTO == ALPHA
    ${_BLEND}( texel${INDEX}.x, C.a, ${TEX_BLEND_FACTOR${_ID}} );
  #endif
#endfor
}
#endif // HAS_FRAGMENT_TEXTURE

-- mapToFragmentUnshaded
#ifndef HAS_FRAGMENT_TEXTURE
#define textureMappingFragmentUnshaded(P,C)
#else
#include regen.states.textures.includes

void textureMappingFragmentUnshaded(in vec3 P, inout vec4 C)
{
    // compute texco
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _TEXCO_CTX textureMappingFragmentUnshaded
#if TEX_MAPTO${_ID}==COLOR || TEX_MAPTO${_ID}==ALPHA
#include regen.states.textures.computeTexco
#endif // ifFragmentMapping
#endfor
    // sample texels
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#if TEX_MAPTO${_ID}==COLOR || TEX_MAPTO${_ID}==ALPHA
#include regen.states.textures.sampleTexel
#endif // ifFragmentMapping
#endfor
    // blend texels with existing values
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _BLEND ${TEX_BLEND_NAME${_ID}}
#define2 _MAPTO ${TEX_MAPTO${_ID}}
  #if _MAPTO == COLOR
    ${_BLEND}( texel${INDEX}, C, ${TEX_BLEND_FACTOR${_ID}} );
  #elif _MAPTO == ALPHA
    ${_BLEND}( texel${INDEX}.x, C.a, ${TEX_BLEND_FACTOR${_ID}} );
  #endif
#endfor
}
#endif // HAS_FRAGMENT_TEXTURE

-- mapToLight
#ifndef HAS_LIGHT_TEXTURE
#define textureMappingLight(P,N,Material)
#include regen.states.material.type
#else
#include regen.states.textures.includes
#include regen.states.material.type

void textureMappingLight(
        in vec3 P,
        in vec3 N,
        inout Material mat)
{
    // compute texco
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _TEXCO_CTX textureMappingLight
#if TEX_MAPTO${_ID}==AMBIENT || TEX_MAPTO${_ID}==DIFFUSE || TEX_MAPTO${_ID}==SPECULAR || TEX_MAPTO${_ID}==EMISSION || TEX_MAPTO${_ID}==LIGHT || TEX_MAPTO${_ID}==SHININESS
#include regen.states.textures.computeTexco
#endif // ifLightMapping
#endfor
    // sample texels
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#if TEX_MAPTO${_ID}==AMBIENT || TEX_MAPTO${_ID}==DIFFUSE || TEX_MAPTO${_ID}==SPECULAR || TEX_MAPTO${_ID}==EMISSION || TEX_MAPTO${_ID}==LIGHT || TEX_MAPTO${_ID}==SHININESS
#include regen.states.textures.sampleTexel
#endif // ifLightMapping
#endfor
    // blend texels with existing values
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _BLEND ${TEX_BLEND_NAME${_ID}}
#define2 _MAPTO ${TEX_MAPTO${_ID}}
  #if _MAPTO == AMBIENT
    // The texture is combined with the result of the ambient lighting equation.
    ${_BLEND}( texel${INDEX}.rgb, mat.ambient, ${TEX_BLEND_FACTOR${_ID}} );
  #elif _MAPTO == DIFFUSE
    // The texture is combined with the result of the diffuse lighting equation.
    ${_BLEND}( texel${INDEX}.rgb, mat.diffuse, ${TEX_BLEND_FACTOR${_ID}} );
  #elif _MAPTO == LIGHT
    // Lightmap texture (aka Ambient Occlusion)
    ${_BLEND}( texel${INDEX}.r, mat.occlusion, ${TEX_BLEND_FACTOR${_ID}} );
  #elif _MAPTO == EMISSION
    // The texture is added to the result of the lighting calculation.
    ${_BLEND}( texel${INDEX}.rgb, mat.emission, ${TEX_BLEND_FACTOR${_ID}} );
  #elif _MAPTO == SPECULAR
    // The texture is combined with the result of the specular lighting equation.
    ${_BLEND}( texel${INDEX}.rgb, mat.specular, ${TEX_BLEND_FACTOR${_ID}} );
  #elif _MAPTO == SHININESS
    ${_BLEND}( texel${INDEX}.r, mat.shininess, ${TEX_BLEND_FACTOR${_ID}} );
  #endif
#endfor
}
#endif // HAS_LIGHT_TEXTURE

--------------------------------------
--------------------------------------
---- Texture coordinates generators.
--------------------------------------
--------------------------------------

-- texco_xz_plane
#ifndef REGEN_TEXCO_XZ_PLANE_
#define2 REGEN_TEXCO_XZ_PLANE_
const vec2 in_planeSize = vec2(10.0,10.0);
// generate texture coordinates based on the xz components of the position
vec2 texco_xz_plane(vec3 P)
{
    return clamp(P.xz / in_planeSize + vec2(0.5), 0.0, 1.0);
}
vec2 texco_xz_plane(vec3 P, vec3 N)
{
    return texco_xz_plane(P);
}
#endif

-- texco_cube
#ifndef REGEN_TEXCO_CUBE_
#define2 REGEN_TEXCO_CUBE_
vec3 texco_cube(vec3 P, vec3 N)
{
    return reflect(-P, N);
}
#endif

-- texco_sphere
#ifndef REGEN_TEXCO_SPHERE_
#define2 REGEN_TEXCO_SPHERE_
vec2 texco_sphere(vec3 P, vec3 N)
{
    vec3 incident = normalize(P - in_cameraPosition.xyz );
    vec3 r = reflect(incident, N);
    float m = 2.0 * sqrt( r.x*r.x + r.y*r.y + (r.z+1.0)*(r.z+1.0) );
    return vec2(r.x/m + 0.5, r.y/m + 0.5);
}
#endif

-- texco_tube
#ifndef REGEN_TEXCO_TUBE_
#define2 REGEN_TEXCO_TUBE_
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
#ifndef REGEN_TEXCO_FLAT_
#define2 REGEN_TEXCO_FLAT_
vec2 texco_flat(vec3 P, vec3 N)
{
    vec3 r = reflect(normalize(P), N);
    return vec2( (r.x + 1.0)/2.0, (r.y + 1.0)/2.0);
}
#endif

-- texco_refraction
#ifndef REGEN_TEXCO_REFR_
#define2 REGEN_TEXCO_REFR_
#include regen.states.camera.transformWorldToTexco
const float in_refractionScale = 0.1;
vec2 texco_refraction(vec3 P, vec3 N)
{
    vec2 uv = transformWorldToTexco(vec4(P,1.0), in_layer).xy;
    // "N" is in world space, so we need to transform it to tangent space
    // because we want to apply offset in xy-plane independently of face orientation.
    mat3 tbn = mat3(in_tangent,in_binormal,in_norWorld);
    uv += (transpose(tbn) * N).xy * in_refractionScale;
    return uv;
}
#endif

-- texco_instance_refraction
#ifndef REGEN_TEXCO_INSTANCE_REFR_
#define2 REGEN_TEXCO_INSTANCE_REFR_
#include regen.states.textures.texco_refraction
vec3 texco_instance_refraction(vec3 P, vec3 N)
{
    return vec3(texco_refraction(P,N), in_instanceID);
}
#endif

-- texco_cube_refraction
#ifndef REGEN_TEXCO_CUBE_REFR_
#define2 REGEN_TEXCO_CUBE_REFR_
vec3 texco_cube_refraction(vec3 P, vec3 N)
{
    vec3 incident = normalize(P - in_cameraPosition.xyz );
    return refract(incident, N, in_matRefractionIndex);
}
#endif

-- texco_cube_reflection
#ifndef REGEN_TEXCO_CUBE_REFL_
#define2 REGEN_TEXCO_CUBE_REFL_
vec3 texco_cube_reflection(vec3 P, vec3 N)
{
  vec3 incident = normalize(P - in_cameraPosition.xyz );
  return reflect(incident.xyz, N);
}
#endif

-- texco_paraboloid_reflection
#ifndef REGEN_TEXCO_PARABOLOID_REFL_
#define2 REGEN_TEXCO_PARABOLOID_REFL_

vec3 texco_paraboloid_reflection(vec3 P, vec3 N)
{
#ifdef IS_PARABOLOID_DUAL
    vec3 P_ = (in_viewProjectionMatrix_Reflection[0] * vec4(P,1.0)).xyz;
    vec3 N_ = (in_viewProjectionMatrix_Reflection[0] * vec4(N,0.0)).xyz;
#else
    vec3 P_ = (in_viewProjectionMatrix_Reflection * vec4(P,1.0)).xyz;
    vec3 N_ = (in_viewProjectionMatrix_Reflection * vec4(N,0.0)).xyz;
#endif
    vec3 R = normalize( reflect(P_, N_) );
    float layer = float(R.z>0.0);
    R.z *= (2.0*layer - 1.0);
    R.x *= (1.0 - 2.0*layer);
    R.y *= -1.0;
    float k = 1.0/(2.0*(1.0 + R.z));
    vec2 uv = R.xy*k + vec2(0.5);
    return vec3(uv,layer);
}
#endif

-- texco_paraboloid_refraction
#ifndef REGEN_TEXCO_PARABOLOID_REFR_
#define2 REGEN_TEXCO_PARABOLOID_REFR_

#ifdef IS_PARABOLOID_DUAL
vec3 texco_paraboloid_refraction(vec3 P, vec3 N)
{
#ifdef IS_PARABOLOID_DUAL
    vec3 P_ = (in_viewProjectionMatrix_Reflection[0] * vec4(P,1.0)).xyz;
    vec3 N_ = (in_viewProjectionMatrix_Reflection[0] * vec4(N,0.0)).xyz;
#else
    vec3 P_ = (in_viewProjectionMatrix_Reflection * vec4(P,1.0)).xyz;
    vec3 N_ = (in_viewProjectionMatrix_Reflection * vec4(N,0.0)).xyz;
#endif
    vec3 R = normalize( refract(P_, N_, in_matRefractionIndex) );
    float layer = float(R.z>0.0);
    R.z *= (2.0*layer - 1.0);
    R.x *= (1.0 - 2.0*layer);
    R.y *= -1.0;
    float k = 1.0/(2.0*(1.0 + R.z));
    vec2 uv = R.xy*k + vec2(0.5);
    return vec3(uv,layer);
}
#endif

-- texco_planar_reflection
#include regen.states.camera.transformScreenToTexco

#ifndef REGEN_TEXCO_PLANE_REFL_
#define2 REGEN_TEXCO_PLANE_REFL_
vec2 texco_planar_reflection(vec3 P, vec3 N)
{
    return transformScreenToTexco(in_viewProjectionMatrix_Reflection * vec4(P,1.0)).xy;
}
#endif

--------------------------------------
--------------------------------------
---- Texel transfer functions.
--------------------------------------
--------------------------------------

-- transfer.texel_norTan
#ifndef REGEN_TRANSFER_NORMAL_TANGENT_
#define2 REGEN_TRANSFER_NORMAL_TANGENT_
void texel_norTan(inout vec4 normal) {
    // Input: normal in tangent space
    // Output: normal in world space
    #if SHADER_STAGE == fs
    // FIXME: function declaration may cause problems, in TES etc where tangent might be array
    mat3 tbn = mat3(in_tangent,in_binormal,in_norWorld);
    normal.xyz = normal.xyz*2.0 - vec3(1.0);
    normal.xyz = normalize( tbn * normal.xyz );
    #endif
}
#endif

-- transfer.texel_norEye
#ifndef REGEN_TRANSFER_NORMAL_EYE_
#define2 REGEN_TRANSFER_NORMAL_EYE_
#include regen.states.camera.transformEyeToWorld
void texel_norEye(inout vec4 normal) {
    // Input: normal in eye space (normalized to [0,1] range)
    // Output: normal in world space
    normal.xyz = normal.xyz*2.0 - vec3(1.0);
    normal.xyz = normalize( normal.xyz );
    normal.xyz = transformEyeToWorld(vec4(normal.xyz,0.0), in_layer).xyz;
}
#endif

-- transfer.texel_norWorld
#ifndef REGEN_TRANSFER_NORMAL_WORLD_
#define2 REGEN_TRANSFER_NORMAL_WORLD_
#include regen.models.tf.transformModel
void texel_norWorld(inout vec4 normal) {
    // Input: normal in world space (normalized to [0,1] range)
    // Output: normal in world space
    normal.xyz = normal.xyz*2.0 - vec3(1.0);
    normal.xyz = normalize( normal.xyz );
#ifdef HAS_modelMatrix
    normal.xyz = normalize(mat3(in_modelMatrix) * normal.xyz);
#endif
}
#endif

-- transfer.texel_norUnity
#ifndef REGEN_TRANSFER_NORMAL_UNITY_
#define2 REGEN_TRANSFER_NORMAL_UNITY_
void texel_norUnity(inout vec4 normal) {
    // Input: normal in unity style (normalized to [0,1] range)
    // Output: normal in world space
    normal.xy = vec2(normal.a, normal.g) * 2.0 - 1.0;
    normal.z  = sqrt(1.0 - clamp(dot(normal.xy, normal.xy), 0.0, 1.0));
}
#endif

-- transfer.texel_invert
#ifndef REGEN_TRANSFER_TEXEL_INVERT_
#define2 REGEN_TRANSFER_TEXEL_INVERT_
void texel_invert(inout vec4 texel)
{
    texel.rgb = 1.0 - texel.rgb;
}
#endif

-- transfer.texel_grayscale
#ifndef REGEN_TEXEL_TRANSFER_GRAYSCALE_
#define2 REGEN_TEXEL_TRANSFER_GRAYSCALE_
void texel_grayscale(inout vec4 texel)
{
    float gray = dot(texel.rgb, vec3(0.299, 0.587, 0.114));
    texel.rgb = vec3(gray);
}
#endif

-- transfer.texel_sepia
#ifndef REGEN_TEXEL_TRANSFER_SEPIA_
#define2 REGEN_TEXEL_TRANSFER_SEPIA_
void texel_sepia(inout vec4 texel)
{
    vec3 sepia = vec3(
        dot(texel.rgb, vec3(0.393, 0.769, 0.189)),
        dot(texel.rgb, vec3(0.349, 0.686, 0.168)),
        dot(texel.rgb, vec3(0.272, 0.534, 0.131))
    );
    texel.rgb = sepia;
}
#endif

-- transfer.texel_brightness
#ifndef REGEN_TEXEL_TRANSFER_BRIGHTNESS_
#define2 REGEN_TEXEL_TRANSFER_BRIGHTNESS_
const vec3 in_brightness = vec3(0.0);
void texel_brightness(inout vec4 texel)
{
    texel.rgb += in_brightness;
}
#endif

-- transfer.texel_contrast
#ifndef REGEN_TEXEL_TRANSFER_CONTRAST_
#define2 REGEN_TEXEL_TRANSFER_CONTRAST_
const float in_contrast = 1.0;
void texel_contrast(inout vec4 texel)
{
    texel.rgb = (texel.rgb - 0.5) * in_contrast + 0.5;
}
#endif

-- transfer.texel_gamma
#ifndef REGEN_TEXEL_TRANSFER_GAMMA_
#define2 REGEN_TEXEL_TRANSFER_GAMMA_
const float in_gamma = 2.2;
void texel_gamma(inout vec4 texel)
{
    texel.rgb = pow(texel.rgb, in_gamma);
}
#endif

-- transfer.texel_saturation
#ifndef REGEN_TEXEL_TRANSFER_SATURATION_
#define2 REGEN_TEXEL_TRANSFER_SATURATION_
const float in_saturation = 1.0;
void texel_saturation(inout vec4 texel)
{
    float gray = dot(texel.rgb, vec3(0.299, 0.587, 0.114));
    texel.rgb = mix(vec3(gray), texel.rgb, in_saturation);
}
#endif

-- transfer.texel_hue
#ifndef REGEN_TEXEL_TRANSFER_HUE_
#define2 REGEN_TEXEL_TRANSFER_HUE_
void texel_hue(inout vec4 texel)
{
    float angle = in_hue * 3.14159265358979323846264;
    float s = sin(angle);
    float c = cos(angle);
    vec3 k = vec3(0.299, 0.587, 0.114);
    vec3 y = vec3(dot(k, texel.rgb));
    vec3 u = vec3(-0.147, -0.289, 0.436);
    vec3 v = vec3(0.615, -0.515, -0.100);
    vec3 u1 = vec3(c + s, c - s, 1.0);
    vec3 v1 = vec3(-s, c, 1.0);
    texel.rgb = y + u * u1 + v * v1;
}
#endif

--------------------------------------
--------------------------------------
---- Texture coordinate transfer functions.
--------------------------------------
--------------------------------------

-- transfer.texco_noise
#ifndef REGEN_NOISE_TRANSFER_
#define2 REGEN_NOISE_TRANSFER_
const float in_uvNoiseScale = 0.1;

#include regen.noise.random2D.a

void texco_noise(inout vec2 texco)
{
    texco += in_uvNoiseScale * random2D(texco);
}
#endif

-- transfer.texco_parallax
#ifndef REGEN_PARALLAX_TRANSFER_
#define2 REGEN_PARALLAX_TRANSFER_
const float in_parallaxScale = 0.1;
const float in_parallaxBias = 0.05;

#include regen.states.textures.eyeVectorTan
#ifdef DEPTH_CORRECT
  #include regen.states.camera.depthCorrection
#endif
#include regen.states.textures.sampleHeight

void texco_parallax(inout vec2 texco)
{
    vec3 offset = eyeVectorTan();
    // parallax mapping with offset limiting
    float height = in_parallaxBias - in_parallaxScale*sampleHeight(texco);
    texco -= height*offset.xy / offset.z;
#ifdef DEPTH_CORRECT
    depthCorrection(in_parallaxScale*height*2.0,in_layer);
#endif
    //if(texco.x > 1.0 || texco.y > 1.0 || texco.x < 0.0 || texco.y < 0.0) {
    //    discard;
    //}
}
#endif

-- transfer.texco_parallax_occlusion
#ifndef REGEN_PARALLAX_OCCLUSION_TRANSFER_
#define2 REGEN_PARALLAX_OCCLUSION_TRANSFER_
const float in_parallaxScale = 0.1;
const int in_parallaxSteps = 50;

#include regen.states.textures.eyeVectorTan
#ifdef DEPTH_CORRECT
  #include regen.states.camera.depthCorrection
#endif
#include regen.states.textures.sampleHeight

void texco_parallax_occlusion(inout vec2 texco)
{
    vec3 offset = eyeVectorTan();
    // step in height each frame
#if 0
    // Increase steps at oblique angles. Note: offset.z = N dot V
    float dh = 1.0/mix(2.0*in_parallaxSteps, in_parallaxSteps, offset.z);
#else
    float dh = 1.0/float(in_parallaxSteps);
#endif
    // step in tbn space each step
    vec2 ds = -offset.xy*in_parallaxScale*dh;
    // start at height=1.0
    float height = 1.0;
    // sample current height
    float sampledHeight = sampleHeight(texco);
    // cast a ray, comparing heights
    while(sampledHeight < height)
    {
        height -= dh;
        texco += ds;
        sampledHeight = sampleHeight(texco);
    }
#ifdef DEPTH_CORRECT
    depthCorrection(-in_parallaxScale*sampledHeight*2.0,in_layer);
#endif
}
#endif

-- transfer.texco_relief
#ifndef REGEN_RELIEF_TRANSFER_
#define2 REGEN_RELIEF_TRANSFER_

const int in_reliefLinearSteps = 20;
const int in_reliefBinarySteps = 5;
const float in_reliefScale = 0.01;

#include regen.states.textures.eyeVectorTan
#ifdef DEPTH_CORRECT
    #include regen.states.camera.depthCorrection
#endif
#include regen.states.textures.sampleHeight

void texco_relief(inout vec2 texco)
{
    vec3 offset = eyeVectorTan();
    vec2 ds = -offset.xy*in_reliefScale/offset.z;

    float depth = 0.0, sampled=1.0;
    float delta = 1.0/float(in_reliefLinearSteps);
    // linear search
    while(sampled > depth) {
        depth += delta;
        sampled = 1.0-sampleHeight(texco + ds*depth);
    }
    // binary search
    for(int i=0; i<in_reliefBinarySteps; ++i)
    {
        delta *= 0.5;
        sampled = 1.0-sampleHeight(texco + ds*depth);
        depth -= (float(sampled<depth)*2.0-1.0)*delta;
    }
#ifdef DEPTH_CORRECT
    depthCorrection(in_reliefScale*depth*2.0,in_layer);
#endif
    
    texco += ds*depth;
}
#endif

-- transfer.texco_fisheye
#ifndef REGEN_TEXCOTRANSFER_FISHEYE_
#define2 REGEN_TEXCOTRANSFER_FISHEYE_

const float in_fishEyeTheta=0.5;

void texco_fisheye(inout vec2 texco)
{
    vec2 uv = texco - vec2(0.5);
    float z = sqrt(1.0 - uv.x*uv.x - uv.y*uv.y);
    float a = 1.0 / (z * tan(in_fishEyeTheta * 0.5));
    //float a = (z * tan(in_fishEyeTheta * 0.5)) / 1.0; // reverse lens
    texco = 2.0*a*uv;
}
#endif

-- transfer.texco_waving
#ifndef REGEN_TEXCOTRANSFER_WAVING_
#define2 REGEN_TEXCOTRANSFER_WAVING_

const float in_wavingFrequency = 1.0;
const float in_wavingAmplitude = 0.2;
const float in_wavingSpeed = 1.0;
const vec2 in_waveBase = vec2(0.5, 1.0);

void wavingTransfer(inout vec2 texco)
{
    float time = in_time * in_wavingSpeed;
    float wave_x = sin(texco.y * in_wavingFrequency + time) * in_wavingAmplitude;
    //float wave_y = sin(texco.x * in_wavingFrequency + time) * in_wavingAmplitude;
#ifdef HAS_waveBase
    wave_x *= length(texco - in_waveBase);
    //wave_y *= length(texco - in_waveBase);
#endif
    texco.x += wave_x;
    //texco.y += wave_y;
}
#endif

-- displacementTransfer
#ifndef REGEN_TEXCOTRANSFER_DISPLACEMENT_
#define2 REGEN_TEXCOTRANSFER_DISPLACEMENT_

const float in_displacementScale = 0.1;

float displacementTransfer(inout vec3 p)
{
    return 0.30*p.x + 0.59*p.y + 0.11*p.z;
}
#endif

-- timeArrayTransfer
#ifndef REGEN_TEXCOTRANSFER_TIME_ARRAY_
#define2 REGEN_TEXCOTRANSFER_TIME_ARRAY_

const float in_fps = 15.0;
const int in_numFrames = 10;

vec3 timeArrayTransfer(vec2 uv, int numArrayLayers, float fps) {
    return vec3(uv, mod(in_time*fps, float(numArrayLayers)));
}
vec3 timeArrayTransfer(vec2 uv)
{
    return timeArrayTransfer(uv, in_numFrames, in_fps);
}
#endif

-- rampCoordinate
#ifndef REGEN_RAMP_COORDINATE_INCLUDED_
#define2 REGEN_RAMP_COORDINATE_INCLUDED_

vec2 rampCoordinate(float rampPosition)
{
    return vec2(rampPosition, 0.5);
}
vec2 rampCoordinate(float rampPosition, float texelSize)
{
    float adjusted =
        // skip first half texel
        0.5*texelSize +
        // scale from [0,1] to [0,1-texelSize]
        mod(rampPosition, 1.0) / (1.0 - texelSize) +
        // add floor of rampPosition
        floor(rampPosition);
    return vec2(adjusted, 0.5);
}
#endif
