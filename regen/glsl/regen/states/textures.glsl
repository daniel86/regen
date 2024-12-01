
-- eyeVectorTan
#ifndef REGEN_eyeVectorTan_included_
#define2 REGEN_eyeVectorTan_included_
vec3 eyeVectorTan()
{
    mat3 tbn = mat3(in_tangent,in_binormal,in_norWorld);
    return normalize( tbn * (in_cameraPosition-in_posWorld) );
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
  #ifdef HAS_DISPLACEMENT_MAP || HAS_HEIGHT_MAP
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

#if TEX_MAPPING_NAME${_ID} == texco_texco
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

-- normalTBNTransfer
#ifndef REGEN_normalTBNTransfer_INCLUDED_
#define2 REGEN_normalTBNTransfer_INCLUDED_
void normalTBNTransfer(inout vec4 texel)
{
#if SHADER_STAGE==fs
    mat3 tbn = mat3(in_tangent,in_binormal,in_norWorld);
    texel.xyz = normalize( tbn * ( texel.xyz*2.0 - vec3(1.0) ) );
#endif
}
#endif

--------------------------------------
--------------------------------------
---- Texture mapping functions.
--------------------------------------
--------------------------------------

-- mapToVertex
#ifndef HAS_VERTEX_TEXTURE
#define textureMappingVertex(P,N)
#else
#include regen.states.textures.includes

void textureMappingVertex(inout vec3 P, inout vec3 N)
{
    // compute texco
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _TEXCO_CTX textureMappingVertex
#if TEX_MAPTO${_ID}==HEIGHT || TEX_MAPTO${_ID}==DISPLACEMENT
#include regen.states.textures.computeTexco
#endif // ifVertexMapping
#endfor
    // sample texels
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#if TEX_MAPTO${_ID}==HEIGHT || TEX_MAPTO${_ID}==DISPLACEMENT
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
    // compute texco
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _TEXCO_CTX textureMappingFragment
#define2 _MAPTO ${TEX_MAPTO${_ID}}
    #if _MAPTO == COLOR
#include regen.states.textures.computeTexco
    #elif _MAPTO == ALPHA
#include regen.states.textures.computeTexco
    #elif _MAPTO == NORMAL
        #ifndef FS_NO_OUTPUT
#include regen.states.textures.computeTexco
        #endif
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
    #elif _MAPTO == NORMAL
        #ifndef FS_NO_OUTPUT
#include regen.states.textures.sampleTexel
        #endif
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
  #elif _MAPTO == NORMAL
    #ifndef FS_NO_OUTPUT
    mat3 tbn${INDEX} = mat3(in_tangent,in_binormal,in_norWorld);
    // Expand the range of the normal value from (0, +1) to (-1, +1).
    vec3 bump${INDEX} = (texel${INDEX}.rgb * 2.0f) - 1.0f;
    // Calculate the normal from the data in the normal map.
    bump${INDEX} = normalize(tbn${INDEX} * bump${INDEX});
    ${_BLEND}( bump${INDEX}, N, ${TEX_BLEND_FACTOR${_ID}} );
    #endif
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
#include regen.states.camera.transformScreenToTexco
vec2 texco_refraction(vec3 P, vec3 N)
{
    vec2 uv = transformScreenToTexco(vec4(P,1.0)).xy;
    //N = (N.xyz * 2.0f) - 1.0f;
    return uv + N.xy * in_refractionScale;
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

#ifdef IS_PARABOLOID_DUAL
vec3 texco_paraboloid_reflection(vec3 P, vec3 N)
#else
vec2 texco_paraboloid_reflection(vec3 P, vec3 N)
#endif
{
#ifdef IS_PARABOLOID_DUAL
  vec3 P_ = (in_reflectionMatrix[0] * vec4(P,1.0)).xyz;
  vec3 N_ = (in_reflectionMatrix[0] * vec4(N,0.0)).xyz;
#else
  vec3 P_ = (in_reflectionMatrix * vec4(P,1.0)).xyz;
  vec3 N_ = (in_reflectionMatrix * vec4(N,0.0)).xyz;
#endif
  vec3 R = normalize( reflect(P_, N_) );
  float layer = float(R.z>0.0);
    
  R.z *= (2.0*layer - 1.0);
  R.x *= (1.0 - 2.0*layer);
  R.y *= -1.0;
    
  float k = 1.0/(2.0*(1.0 + R.z));
  vec2 uv = R.xy*k + vec2(0.5);
#ifdef IS_PARABOLOID_DUAL
  return vec3(uv,layer);
#else
  return uv;
#endif
}
#endif

-- texco_paraboloid_refraction
#ifndef REGEN_TEXCO_PARABOLOID_REFR_
#define2 REGEN_TEXCO_PARABOLOID_REFR_

-- texco_planar_reflection
#include regen.states.camera.transformScreenToTexco

#ifndef REGEN_TEXCO_PLANE_REFL_
#define2 REGEN_TEXCO_PLANE_REFL_
vec2 texco_planar_reflection(vec3 P, vec3 N)
{
    return transformScreenToTexco(in_reflectionMatrix * vec4(P,1.0)).xy;
}
#endif

--------------------------------------
--------------------------------------
---- Texture coordinate transfer functions.
--------------------------------------
--------------------------------------

-- parallaxTransfer
#ifndef REGEN_PARALLAX_TRANSFER_
#define2 REGEN_PARALLAX_TRANSFER_
const float in_parallaxScale = 0.1;
const float in_parallaxBias = 0.05;

#include regen.states.textures.eyeVectorTan
#ifdef DEPTH_CORRECT
  #include regen.states.camera.depthCorrection
#endif
#include regen.states.textures.sampleHeight

void parallaxTransfer(inout vec2 texco)
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

-- parallaxOcclusionTransfer
#ifndef REGEN_PARALLAX_OCCLUSION_TRANSFER_
#define2 REGEN_PARALLAX_OCCLUSION_TRANSFER_
const float in_parallaxScale = 0.1;
const int in_parallaxSteps = 50;

#include regen.states.textures.eyeVectorTan
#ifdef DEPTH_CORRECT
  #include regen.states.camera.depthCorrection
#endif
#include regen.states.textures.sampleHeight

void parallaxOcclusionTransfer(inout vec2 texco)
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

-- reliefTransfer
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

void reliefTransfer(inout vec2 texco)
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

-- fisheyeTransfer
#ifndef REGEN_TEXCOTRANSFER_FISHEYE_
#define2 REGEN_TEXCOTRANSFER_FISHEYE_

const float in_fishEyeTheta=0.5;

void fisheyeTransfer(inout vec2 texco)
{
    vec2 uv = texco - vec2(0.5);
    float z = sqrt(1.0 - uv.x*uv.x - uv.y*uv.y);
    float a = 1.0 / (z * tan(in_fishEyeTheta * 0.5));
    //float a = (z * tan(in_fishEyeTheta * 0.5)) / 1.0; // reverse lens
    texco = 2.0*a*uv;
}
#endif

-- wavingTransfer
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
