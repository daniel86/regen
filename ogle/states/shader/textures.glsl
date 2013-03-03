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

// include texture transfer functions
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
  #ifdef TEX_TRANSFER_KEY${_ID}
#include ${TEX_TRANSFER_KEY${_ID}}
  #endif
#endfor
#endif // __IS_TEXCO_DECLARED

-- mapToVertex
#ifdef HAS_VERTEX_TEXTURE
#include textures.includes

void textureMappingVertex(inout vec3 P, inout vec3 N)
{
    // lookup texels
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
  #if TEX_MAPTO${_ID} == HEIGHT || TEX_MAPTO${_ID} == DISPLACEMENT
    #if TEX_MAPPING_KEY${_ID} == textures.texco_texco
    vec4 texel${INDEX} = SAMPLE( in_${TEX_NAME${_ID}}, in_${TEX_TEXCO${_ID}} );
    #else
    vec4 texel${INDEX} = SAMPLE( in_${TEX_NAME${_ID}}, ${TEX_MAPPING_NAME${_ID}}(P,N) );
    #endif
    #ifdef TEX_TRANSFER_NAME${_ID}
    // use a custom transfer function for the texel
    ${TEX_TRANSFER_NAME${_ID}}(texel${INDEX});
    #endif // TEX_TRANSFER_NAME${_ID}
  #endif
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
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
  #if TEX_MAPTO${_ID} == COLOR || TEX_MAPTO${_ID} == ALPHA || TEX_MAPTO${_ID} == NORMAL
    #if TEX_MAPPING_KEY${_ID} == textures.texco_texco
    vec4 texel${INDEX} = texture( in_${TEX_NAME${_ID}}, in_${TEX_TEXCO${_ID}} );
    #else
    vec4 texel${INDEX} = texture( in_${TEX_NAME${_ID}}, ${TEX_MAPPING_NAME${_ID}}(P,N) );
    #endif
    #ifdef TEX_TRANSFER_NAME${_ID}
    // use a custom transfer function for the texel
    ${TEX_TRANSFER_NAME${_ID}}(texel${INDEX});
    #endif // TEX_TRANSFER_NAME${_ID}
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
    ${_BLEND}( texel${INDEX}.x, A, ${TEX_BLEND_FACTOR${_ID}} );
  #elif _MAPTO == NORMAL
    ${_BLEND}( texel${INDEX}.rgb, N, ${TEX_BLEND_FACTOR${_ID}} );
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
        inout vec3 color,
        inout vec3 specular,
        inout float shininess)
{
    // lookup texels
#for INDEX to NUM_TEXTURES
#define2 _ID ${TEX_ID${INDEX}}
  #if TEX_MAPTO${_ID} == AMBIENT || TEX_MAPTO${_ID} == DIFFUSE || TEX_MAPTO${_ID} == SPECULAR || TEX_MAPTO${_ID} == EMISSION || TEX_MAPTO${_ID} == LIGHT || TEX_MAPTO${_ID} == SHININESS
    #if TEX_MAPPING_KEY${_ID} == textures.texco_texco
    vec4 texel${INDEX} = texture( in_${TEX_NAME${_ID}}, in_${TEX_TEXCO${_ID}} );
    #elif TEX_MAPPING_KEY${_ID} != textures.texco_custom
    vec4 texel${INDEX} = texture( in_${TEX_NAME${_ID}}, ${TEX_MAPPING_NAME${_ID}}(P,N) );
    #endif
    #ifdef TEX_TRANSFER_NAME${_ID}
    // use a custom transfer function for the texel
    ${TEX_TRANSFER_NAME${_ID}}(texel${INDEX});
    #endif // TEX_TRANSFER_NAME${_ID}
  #endif
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
#else
#define textureMappingLight(P,N,C,SPEC,SHIN)
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

-- texcoTransfer_parallax
#ifndef __TEXCOTRANSFER_PARALLAX__
#define2 __TEXCOTRANSFER_PARALLAX__
const float parallaxScale = 0.04;
const float parallaxBias = -0.03;

void texcoTransfer_parallax(inout vec2 texco)
{
    mat3 tbn = mat3(
        in_tangent,
        in_binormal,
        in_norWorld);
    vec3 v = normalize(tbn * (-in_posWorld));
    
    float height = texture(in_heightTexture, texco).r;
    height = height * parallaxScale + parallaxBias;
    texco += (height * v.xy);

    //mat3 tts = transpose( mat3(in_tangent,in_binormal,in_norWorld) );
    //vec2 offset = -normalize( tts * (in_cameraPosition - in_posWorld) ).xy;
    //offset.y = -offset.y;
    //float height = parallaxScale * texture(in_heightTexture, texco).x - parallaxBias;
    //texco = texco + height*offset;
}
#endif

-- texcoTransfer_steepParallax
#ifndef __TEXCOTRANSFER_STEEP_PARALLAX__
#define2 __TEXCOTRANSFER_STEEP_PARALLAX__
const float parallaxScale = 0.05;
const float parallaxSteps = 5.0;

void texcoTransfer_steepParallax(inout vec2 texco) {
    mat3 tts = transpose( mat3(in_tangent,in_binormal,in_norWorld) );
    vec3 offset = -normalize( tts * (in_cameraPosition - in_posWorld) );
    offset.y = -offset.y;
    float numSteps = mix(2.0*parallaxSteps, parallaxSteps, offset.z);
    float step = 1.0 / numSteps;
    vec2 delta = offset.xy * parallaxScale / (offset.z * numSteps);
    float NB = texture(in_heightTexture, offsetCoord).x;
    float height = 1.0;
    while (NB < height) {
        height -= step;
        texco += delta;
        NB = texture(in_heightTexture, offsetCoord).x;
    }
}
#endif

-- texcoTransfer_relief
#ifndef __TEXCOTRANSFER_RELIEF__
#define2 __TEXCOTRANSFER_RELIEF__

float linearSearch(sampler2D reliefMap, vec2 A, vec2 B)
{
    float t = 0.0;

    for(int i = 0; i < LINEAR_STEPS; i++)
    {
        t += 1.0 / LINEAR_STEPS;
        float d = texture(reliefMap, mix(A, B, t)).x;
        if(t > d) break;
    }

    return t;
}
float binarySearch(sampler2D reliefMap, vec2 A, vec2 B, float a, float b)
{
    float depth;

    for(int i = 0; i < BINARY_STEPS; i++)
    {
        depth = mix(a, b, 0.5);
        float d = texture(reliefMap, mix(A, B, depth)).x;
        float toggle = float(d > depth);
        a = depth*toggle;
        b = depth*(1.0-toggle);
    }

    return depth;
}
float fullSearch(sampler2D reliefMap, vec2 A, vec2 B)
{
    float depth = linearSearch(reliefMap, A, B);
    return binarySearch(reliefMap, A, B, depth-(1.0 / LINEAR_STEPS), depth);
}

void texcoTransfer_relief(inout vec2 texco)
{
    vec2 A = in_texco;
    //vector from A to the exit point (B)
    vec3 V = (to_eye / -to_eye.z) * scale;
    vec2 B = A + V.xy;

    float depth = fullSearch(A, B);

    // the intersection point in texture space
    vec3 P = vec3(mix(A, B, depth), depth);

    // normal mapping normal
    vec3 norm = texture(in_normalTexture, P.xy).rgb;
    norm = normalize((norm - 0.5) * 2.0);

#ifdef DEPTH_CORRECT
    // depth correct formula as described in the paper.
    float eyeZ = eye_to_pos.z + normalize(eye_to_pos).z * scale * depth;
    gl_FragDepth = 
        ((-in_far / (in_far - in_near)) * eyeZ +
         (-in_far * in_near / (in_far - in_near))) / -eyeZ;
#endif
}

#endif

