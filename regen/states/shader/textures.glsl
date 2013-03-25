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
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _NAME ${TEX_NAME${_ID}}

#ifndef __TEX_${_NAME}__
#define __TEX_${_NAME}__
uniform ${TEX_SAMPLER_TYPE${_ID}} in_${_NAME};
#endif

#if TEX_MAPPING_NAME${_ID} == texco_texco
  #define2 _TEXCO ${TEX_TEXCO${_ID}}
  #define2 _DIM ${TEX_DIM${_ID}}
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
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _MAPPING ${TEX_MAPPING_KEY${_ID}}
  #if ${_MAPPING} != textures.texco_texco && TEX_MAPPING_KEY${_ID} != textures.texco_custom
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
#endif // __IS_TEXCO_DECLARED

-- computeTexco
#if TEX_DIM${_ID}==1
#define2 TEXCO_TYPE float
#else
#define2 TEXCO_TYPE vec${TEX_DIM${_ID}}
#endif
#define2 _MAPPING_ ${TEX_MAPPING_NAME${_ID}}

#if _MAPPING_!=texco_texco
#ifndef __texco_${_MAPPING_}__
#define2 __texco_${_MAPPING_}__
    // generate texco
    ${TEXCO_TYPE} texco_${_MAPPING_} = ${_MAPPING_}(P,N);
#endif // __texco_${_MAPPING_${_ID}}__

#ifdef TEXCO_TRANSFER_NAME${_ID}
#define2 _TRANSFER_ ${TEXCO_TRANSFER_NAME${_ID}}
    // apply transfer function to texco
#ifndef __texco_${_MAPPING_}_${_TRANSFER_}__
#define2 __texco_${_MAPPING_}_${_TRANSFER_}__
    ${TEXCO_TYPE} texco_${_MAPPING_}_${_TRANSFER_} = texco_${_MAPPING_};
    ${_TRANSFER_}( texco_${_MAPPING_}_${_TRANSFER_} );
#endif // __texco_${_MAPPING_}_${TEXCO_TRANSFER_NAME${_ID}}__
#define2 __TEXCO${_ID}__ texco_${_MAPPING_}_${_TRANSFER_}
#else
#define2 __TEXCO${_ID}__ texco_${_MAPPING_}
#endif

#else // _MAPPING_==textures.texco_texco
#ifndef __${TEX_TEXCO${_ID}}__
#define2 __${TEX_TEXCO${_ID}}__
#if SHADER_STAGE==tes
    ${TEXCO_TYPE} ${TEX_TEXCO${_ID}} = INTERPOLATE_VALUE(in_${TEX_TEXCO${_ID}});
#else
    ${TEXCO_TYPE} ${TEX_TEXCO${_ID}} = in_${TEX_TEXCO${_ID}};
#endif
#endif // __${TEX_TEXCO${_ID}}__

#ifdef TEXCO_TRANSFER_NAME${_ID}
#define2 _TRANSFER_ ${TEXCO_TRANSFER_NAME${_ID}}
    // apply transfer function to texco
#ifndef __${TEX_TEXCO${_ID}}_${_TRANSFER_}__
#define2 __${TEX_TEXCO${_ID}}_${_TRANSFER_}__
    ${TEXCO_TYPE} ${TEX_TEXCO${_ID}}_${_TRANSFER_} = ${TEX_TEXCO${_ID}};
    ${_TRANSFER_}( ${TEX_TEXCO${_ID}}_${_TRANSFER_} );
#endif // __${TEX_TEXCO${_ID}}_${_TRANSFER_}__
#define2 __TEXCO${_ID}__ ${TEX_TEXCO${_ID}}_${_TRANSFER_}
#else
#define2 __TEXCO${_ID}__ ${TEX_TEXCO${_ID}}
#endif

#endif // _MAPPING_==textures.texco_texco

-- sampleTexel
    vec4 texel${INDEX} = texture(in_${TEX_NAME${_ID}}, ${__TEXCO${_ID}__});
#ifdef TEX_IGNORE_ALPHA${_ID}
    texel.a = 1.0;
#endif
#ifdef TEX_TRANSFER_NAME${_ID}
    // use a custom transfer function for the texel
    ${TEX_TRANSFER_NAME${_ID}}(texel${INDEX});
#endif // TEX_TRANSFER_NAME${_ID}

-- mapToVertex
#ifndef HAS_VERTEX_TEXTURE
#define textureMappingVertex(P,N)
#else
#include textures.includes

void textureMappingVertex(inout vec3 P, inout vec3 N)
{
    // compute texco
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#if TEX_MAPTO${_ID}==HEIGHT || TEX_MAPTO${_ID}==DISPLACEMENT
#include textures.computeTexco
#endif // ifVertexMapping
#endfor
    // sample texels
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#if TEX_MAPTO${_ID}==HEIGHT || TEX_MAPTO${_ID}==DISPLACEMENT
#include textures.sampleTexel
#endif // ifVertexMapping
#endfor
    // blend texels with existing values
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _BLEND ${TEX_BLEND_NAME${_ID}}
#define2 _MAPTO ${TEX_MAPTO${_ID}}
  #if _MAPTO == HEIGHT
    ${_BLEND}( N * texel${INDEX}.x, P, ${TEX_BLEND_FACTOR${_ID}} );
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
#include textures.includes

void textureMappingFragment(in vec3 P, inout vec4 C, inout vec3 N)
{
    // compute texco
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#if TEX_MAPTO${_ID}==COLOR || TEX_MAPTO${_ID}==ALPHA || TEX_MAPTO${_ID}==NORMAL
#include textures.computeTexco
#endif // ifFragmentMapping
#endfor
    // sample texels
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#if TEX_MAPTO${_ID}==COLOR || TEX_MAPTO${_ID}==ALPHA || TEX_MAPTO${_ID}==NORMAL
#include textures.sampleTexel
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
  #elif _MAPTO == NORMAL
    ${_BLEND}( texel${INDEX}.rgb, N, ${TEX_BLEND_FACTOR${_ID}} );
  #endif
#endfor
}
#endif // HAS_FRAGMENT_TEXTURE

-- mapToFragmentUnshaded
#ifndef HAS_FRAGMENT_TEXTURE
#define textureMappingFragmentUnshaded(P,C)
#else
#include textures.includes

void textureMappingFragmentUnshaded(in vec3 P, inout vec4 C)
{
    // compute texco
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#if TEX_MAPTO${_ID}==COLOR || TEX_MAPTO${_ID}==ALPHA
#include textures.computeTexco
#endif // ifFragmentMapping
#endfor
    // sample texels
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#if TEX_MAPTO${_ID}==COLOR || TEX_MAPTO${_ID}==ALPHA
#include textures.sampleTexel
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
#define textureMappingLight(P,N,C,SPEC,SHIN)
#else
#include textures.includes

void textureMappingLight(
        in vec3 P,
        in vec3 N,
        inout vec3 color,
        inout vec3 specular,
        inout float shininess)
{
    // compute texco
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#if TEX_MAPTO${_ID}==AMBIENT || TEX_MAPTO${_ID}==DIFFUSE || TEX_MAPTO${_ID}==SPECULAR || TEX_MAPTO${_ID}==EMISSION || TEX_MAPTO${_ID}==LIGHT || TEX_MAPTO${_ID}==SHININESS
#include textures.computeTexco
#endif // ifLightMapping
#endfor
    // sample texels
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#if TEX_MAPTO${_ID}==AMBIENT || TEX_MAPTO${_ID}==DIFFUSE || TEX_MAPTO${_ID}==SPECULAR || TEX_MAPTO${_ID}==EMISSION || TEX_MAPTO${_ID}==LIGHT || TEX_MAPTO${_ID}==SHININESS
#include textures.sampleTexel
#endif // ifLightMapping
#endfor
    // blend texels with existing values
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
#define2 _BLEND ${TEX_BLEND_NAME${_ID}}
#define2 _MAPTO ${TEX_MAPTO${_ID}}
  #if _MAPTO == AMBIENT
    ${_BLEND}( texel${INDEX}.rgb, color, ${TEX_BLEND_FACTOR${_ID}} );
  #elif _MAPTO == DIFFUSE
    ${_BLEND}( texel${INDEX}.rgb, color, ${TEX_BLEND_FACTOR${_ID}} );
  #elif _MAPTO == LIGHT
    ${_BLEND}( texel${INDEX}.rgb, color, ${TEX_BLEND_FACTOR${_ID}} );
  #elif _MAPTO == SPECULAR
    ${_BLEND}( texel${INDEX}.rgb, specular, ${TEX_BLEND_FACTOR${_ID}} );
  #elif _MAPTO == SHININESS
    ${_BLEND}( texel${INDEX}.r, shininess, ${TEX_BLEND_FACTOR${_ID}} );
  #endif
#endfor
}
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

-- eyeVectorTan
#ifndef __eyeVectorTan_included__
#define2 __eyeVectorTan_included__
vec3 eyeVectorTan()
{
    mat3 tbn = mat3(
        in_tangent.x, in_binormal.x, in_norWorld.x,
        in_tangent.y, in_binormal.y, in_norWorld.y,
        in_tangent.z, in_binormal.z, in_norWorld.z
    );
    vec3 offset = normalize( tbn * (in_cameraPosition-in_posWorld) );
    offset.y *= -1;
    return offset;
}
#endif

-- depthCorrection
#ifndef __depthCorrection_included__
#define2 __depthCorrection_included__
void depthCorrection(float depth)
{
    vec3 pe = in_posEye + depth*normalize(in_posEye);
    vec4 ps = in_projectionMatrix * vec4(pe,1.0);
    gl_FragDepth = (ps.z/ps.w)*0.5 + 0.5;
}
#endif

-- parallaxTransfer
#ifndef __PARALLAX_TRANSFER__
#define2 __PARALLAX_TRANSFER__
const float in_parallaxScale = 0.1;
const float in_parallaxBias = 0.05;

#include textures.eyeVectorTan
#ifdef DEPTH_CORRECT
#include textures.depthCorrection
#endif

void parallaxTransfer(inout vec2 texco)
{
    vec3 offset = eyeVectorTan();
    // parallax mapping with offset limiting
    float height = in_parallaxBias -
        in_parallaxScale*texture(in_heightTexture, texco).r;
    texco -= height*offset.xy;
#ifdef DEPTH_CORRECT
    depthCorrection(in_parallaxScale*height*2.0);
#endif
}
#endif

-- parallaxOcclusionTransfer
#ifndef __PARALLAX_OCCLUSION_TRANSFER__
#define2 __PARALLAX_OCCLUSION_TRANSFER__
const float in_parallaxScale = 0.1;
const int in_parallaxSteps = 50;

#include textures.eyeVectorTan
#ifdef DEPTH_CORRECT
#include textures.depthCorrection
#endif

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
    vec2 ds = -offset.xy*in_parallaxScale*dh/offset.z;
    // start at height=1.0
    float height = 1.0;
    // sample current height
    float sampledHeight = texture(in_heightTexture, texco).r;
    // cast a ray, comparing heights
    while(sampledHeight < height)
    {
        height -= dh;
        texco += ds;
        sampledHeight = texture(in_heightTexture, texco).r;
    }
#ifdef DEPTH_CORRECT
    depthCorrection(-in_parallaxScale*sampledHeight*2.0);
#endif
}
#endif

-- reliefTransfer
#ifndef __RELIEF_TRANSFER__
#define2 __RELIEF_TRANSFER__

const int in_reliefLinearSteps = 20;
const int in_reliefBinarySteps = 5;
const float in_reliefScale = 0.1;

#include textures.eyeVectorTan
#include textures.depthCorrection

void reliefTransfer(inout vec2 texco)
{
    vec3 offset = eyeVectorTan();
    vec2 ds = -offset.xy*in_reliefScale/offset.z;

    float depth = 0.0, sampled=1.0;
    float delta = 1.0/float(in_reliefLinearSteps);
    // linear search
    while(sampled > depth) {
        depth += delta;
        sampled = 1.0-texture(in_heightTexture, texco + ds*depth).x;
    }
    // binary search
    for(int i=0; i<in_reliefBinarySteps; ++i)
    {
        delta *= 0.5;
        sampled = 1.0-texture(in_heightTexture, texco + ds*depth).x;
        depth -= (float(sampled<depth)*2.0-1.0)*delta;
    }
#ifdef DEPTH_CORRECT
    depthCorrection(in_reliefScale*depth*2.0);
#endif
    
    texco += ds*depth;
}
#endif

-- fisheyeTransfer
#ifndef __TEXCOTRANSFER_FISHEYE__
#define2 __TEXCOTRANSFER_FISHEYE__

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

-- texcoTransfer___
// TODO: other texco transfer algorithms
//    - cone step mapping
//        - pre-compute cone map
//        - relaxed: combine with binary search
//    - interval mapping

#endif

