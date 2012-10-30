-- defines
#ifndef __SHADING_DEFINES
#define __SHADING_DEFINES

  #ifndef HAS_MATERIAL
    #ifdef HAS_LIGHT
// set default shading for unspecified material
#define SHADING GOURAD
    #endif
  #endif // !HAS_MATERIAL

  #if SHADING != NONE
    #ifdef HAS_LIGHT
    #ifdef SHADING == GOURAD
      #define HAS_VERTEX_SHADING
    #else
#define HAS_FRAGMENT_SHADING
    #endif
    #endif
#define HAS_SHADING
  #else // SHADING == NONE
// #undef HAS_SHADING
  #endif

#endif // __SHADING_DEFINES

-- types
#ifndef __SHADING_TYPES
#define __SHADING_TYPES

#include light.defines

#ifdef HAS_SHADING
struct Shading {
  vec4 ambient;
  vec4 diffuse;
  vec4 specular;
  vec4 emission;
  float shininess;
};
#endif // HAS_SHADING

#ifdef HAS_LIGHT
struct LightProperties {
#for NUM_LIGHTS
#define __ID ${LIGHT${FOR_INDEX}_ID}
    vec3 lightVec${__ID};
    float attenuation${__ID};
#undef __ID
#endfor
};
#endif // HAS_LIGHT

#ifdef HAS_SHADING
void initShading(inout Shading s) {
    s.ambient = vec4(0.0);
    s.diffuse = vec4(0.0);
    s.specular = vec4(0.0);
    s.emission = vec4(0.0);
    s.shininess = 0.0;
}
#endif // HAS_SHADING
#endif // __SHADING_TYPES

-- input
#ifndef __IS_LIGHT_DECLARED
#define __IS_LIGHT_DECLARED

#include light.defines

#for NUM_LIGHTS
#define __ID ${LIGHT${FOR_INDEX}_ID}
    uniform vec4 in_lightPosition${__ID};
    uniform vec4 in_lightAmbient${__ID};
    uniform vec4 in_lightDiffuse${__ID};
    uniform vec4 in_lightSpecular${__ID};
#if LIGHT${FOR_INDEX}_TYPE == SPOT
    uniform float in_lightInnerConeAngle${__ID};
    uniform float in_lightOuterConeAngle${__ID};
    uniform vec3 in_lightSpotDirection${__ID};
    uniform float in_lightSpotExponent${__ID};
#endif
#if LIGHT${FOR_INDEX}_TYPE != DIRECTIONAL
    uniform float in_lightConstantAttenuation${__ID};
    uniform float in_lightLinearAttenuation${__ID};
    uniform float in_lightQuadricAttenuation${__ID};
#endif
#undef __ID
#endfor
#endif // __IS_LIGHT_DECLARED

-- interpolate
// interpolate shading types in TES

#ifdef HAS_LIGHT
LightProperties interpolate(LightProperties props[TESS_NUM_VERTICES]) {
    LightProperties ret;
#for NUM_LIGHTS
#define __ID ${LIGHT${FOR_INDEX}_ID}
    ret.lightVec${__ID} = INTERPOLATE_STRUCT(props,lightVec);
    ret.attenuation${__ID} = INTERPOLATE_STRUCT(props,attenuation);
#undef __ID
#endfor
    return ret;
}
#endif // HAS_LIGHT

#ifdef HAS_SHADING
Shading interpolate(Shading shading[TESS_NUM_VERTICES]) {
    Shading ret;
    ret.ambient = INTERPOLATE_STRUCT(shading,ambient);
    ret.diffuse = INTERPOLATE_STRUCT(shading,diffuse);
    ret.specular = INTERPOLATE_STRUCT(shading,specular);
    ret.emission = INTERPOLATE_STRUCT(shading,emission);
    ret.shininess = INTERPOLATE_STRUCT(shading,shininess);
    return ret;
}
#endif // HAS_SHADING

-- init
#ifndef __SHADING_PROPERTIES
#define __SHADING_PROPERTIES

#include light.types

#include light.input

void shadeProperties(inout LightProperties props, vec4 P)
{
#for NUM_LIGHTS
#define __ID ${LIGHT${FOR_INDEX}_ID}
  {
  #if LIGHT${FOR_INDEX}_TYPE == SPOT
    props.lightVec${__ID} = vec3( in_lightPosition${__ID}.xyz - P );
    float spotEffect = dot(
        normalize(in_lightSpotDirection${__ID}),
        normalize(-props.lightVec${__ID}));
    if (spotEffect > in_lightInnerConeAngle${__ID}) {
        float dist = length(props.lightVec${__ID});
        props.attenuation${__ID} = pow(spotEffect, in_lightSpotExponent${__ID})/(
            in_lightConstantAttenuation${__ID} +
            in_lightLinearAttenuation${__ID} * dist +
            in_lightQuadricAttenuation${__ID} * dist * dist);
    } else {
        props.attenuation${__ID} = 0.0;
    }

  #elif LIGHT${FOR_INDEX}_TYPE == POINT
    props.lightVec${__ID} = vec3( in_lightPosition${__ID}.xyz - P );
    float dist = length(props.lightVec[INDEX]);
    props.attenuation${__ID} = 1.0/(
        in_lightConstantAttenuation${__ID} +
        in_lightLinearAttenuation${__ID} * dist +
        in_lightQuadricAttenuation${__ID} * dist * dist );

  #else
    // spot light
    props.lightVec${__ID} = in_lightPosition${__ID}.xyz;
    props.attenuation${__ID} = 0.0;
  #endif
  }
#undef __ID

#endfor
}

#endif // __SHADING_PROPERTIES

-- shade
#ifndef __SHADING_SHADE_
#define __SHADING_SHADE_

#include light.types

#include light.input

#if SHADING == TOON
#include light.toon
#elif SHADING == GOURAD
#include light.gourad
#elif SHADING == BLINN
#include light.blinn
#elif SHADING == ORENNAYER
#include light.oren-nayer
#else
#include light.phong
#endif

void shade(LightProperties props, inout Shading shading, vec3 P, vec3 N) {
#for NUM_LIGHTS
#define __ID ${LIGHT${FOR_INDEX}_ID}
  {
    vec3 L = normalize(props.lightVec${__ID});
    float nDotL = max( dot( N, L ), 0.0 );
    
    shading.ambient += in_lightAmbient${__ID}; 

    if (nDotL > 0.0) {
  #if LIGHT${FOR_INDEX}_TYPE == SPOT
        shadeSpotDiffuse(props,shading,${__ID},P,N,L,nDotL);
        if(shading.shininess > 0.0) {
            shadeSpotSpecular(props,shading,${__ID},P,N,L);
        }

  #elif LIGHT${FOR_INDEX}_TYPE == POINT
        shadePointDiffuse(props,shading,${__ID},P,N,L,nDotL);
        if(shading.shininess > 0.0) {
            shadePointSpecular(props,shading,${__ID},P,N,L);
        }

  #else
        // directional light
        shadeDirectionalDiffuse(props,shading,${__ID},P,N,L,nDotL);
        if(shading.shininess > 0.0) {
            shadeDirectionalSpecular(props,shading,${__ID},P,N,L);
        }

  #endif
    }
  }
#undef __ID
#endfor
}

#endif // __SHADING_SHADE_

-- directional.diffuse
#define shadeDirectionalDiffuse(PROPS, SHADING, ID, P, N, L, nDotL) { \
    SHADING.diffuse += in_lightDiffuse ## ID * nDotL; \
}
-- directional.specular
#define shadeDirectionalSpecular(PROPS, SHADING, ID, P, N, L) { \
    vec3 P = normalize( -P.xyz ); \
    vec3 reflected = normalize( reflect( -L, N ) ); \
    float rDotE = max( dot( reflected, P ), 0.0); \
    SHADING.specular += PROPS.attenuation ## ID * \
        in_lightSpecular ## ID * pow(rDotE, SHADING.shininess); \
}

-- point.diffuse
#define shadePointDiffuse(PROPS, SHADING, ID, P, N, L, nDotL) { \
    SHADING.diffuse += PROPS.attenuation ## ID * in_lightDiffuse ## ID * nDotL; \
}
-- point.specular
#define shadePointSpecular(PROPS, SHADING, ID, P, N, L) { \
    vec3 P = normalize( -P.xyz ); \
    vec3 reflected = normalize( reflect( -L, N ) ); \
    float rDotE = max( dot( reflected, P ), 0.0); \
    SHADING.specular += PROPS.attenuation ## ID * \
        in_lightSpecular ## ID * pow(rDotE, SHADING.shininess); \
}

-- spot.diffuse
#define shadeSpotDiffuse(PROPS, SHADING, ID, P, N, L, nDotL) { \
    vec3 normalizedSpotDir = normalize(in_lightSpotDirection ## ID); \
    float spotEffect = dot( -L, normalizedSpotDir ); \
    float coneDiff = (in_lightInnerConeAngle ## ID - in_lightOuterConeAngle ## ID); \
    float falloff = clamp((spotEffect - in_lightOuterConeAngle ## ID) / coneDiff, 0.0, 1.0); \
    SHADING.diffuse += PROPS.attenuation ## ID * \
        in_lightDiffuse ## ID * nDotL * falloff; \
}
-- spot.specular
#define shadeSpotSpecular(PROPS, SHADING, ID, P, N, L) { \
    vec3 P = normalize( -P.xyz ); \
    vec3 reflected = normalize( reflect( -L, N ) ); \
    float rDotE = max( dot( reflected, P ), 0.0); \
    SHADING.specular += in_lightSpecular ## ID * pow(rDotE, SHADING.shininess); \
}

-- phong
#include light.point.diffuse
#include light.point.specular
#include light.spot.diffuse
#include light.spot.specular
#include light.directional.diffuse
#include light.directional.specular

-- gourad
#include light.point.diffuse
#include light.point.specular
#include light.spot.diffuse
#include light.spot.specular
#include light.directional.diffuse
#include light.directional.specular

-- blinn
#include light.point.diffuse
#include light.spot.diffuse
#include light.directional.diffuse
#define shadePointSpecular(PROPS, SHADING, ID, P, N, L) { \
    vec3 P = normalize( -P.xyz ); \
    vec3 halfVecNormalized = normalize( L + P ); \
    SHADING.specular += in_lightSpecular ## ID * \
        pow( max(0.0,dot(N,halfVecNormalized)), SHADING.shininess); \
}
#define shadeSpotSpecular shadePointSpecular
#define shadeDirectionalSpecular shadePointSpecular

-- oren-nayer
#include light.point.specular
#include light.spot.specular
#include light.directional.specular
#define shadePointDiffuse(PROPS, SHADING, ID, P, N, L, nDotL) { \
    vec3 V = normalize( -P.xyz ); \
    float vDotN = dot(V, N); \
    float cos_theta_i = nDotL; \
    float theta_r = acos(vDotN); \
    float theta_i = acos(cos_theta_i); \
    float cos_phi_diff = dot(normalize(V-N*vDotN), normalize(L-N*nDotL)); \
    float alpha = max(theta_i,theta_r); \
    float beta = min(theta_i,theta_r); \
    float r = in_matRoughness*in_matRoughness; \
    float a = 1.0-0.5*r/(r+0.33); \
    float b = 0.45*r/(r+0.09); \
    if (cos_phi_diff>=0) { \
        b*=sin(alpha)*tan(beta); \
    } else { \
        b=0.0; \
    } \
    SHADING.diffuse += in_lightDiffuse ## ID * cos_theta_i * (a+b); \
}
#define shadeSpotDiffuse shadePointDiffuse
#define shadeDirectionalDiffuse shadePointDiffuse

-- minnaert
#include light.point.specular
#include light.spot.specular
#include light.directional.specular
#define shadePointDiffuse(PROPS, SHADING, ID, P, N, L, nDotL) { \
    SHADING.diffuseVar += in_lightDiffuse ## ID * pow(nDotL, in_matDarkness); \
}
#define shadeSpotDiffuse shadePointDiffuse
#define shadeDirectionalDiffuse shadePointDiffuse

-- cook-torrance
#include light.point.diffuse
#include light.spot.diffuse
#include light.directional.diffuse
#define shadePointSpecular(PROPS, SHADING, ID, P, N, L) { \
    vec3 P = normalize( -P.xyz ); \
    vec3 halfVecNormalized = normalize( L + P ); \
    float nDotH = dot(N, halfVecNormalized); \
    if(nDotH >= 0.0) { \
        float nDotV = max(dot(N, P), 0.0); \
        float specularFactor = pow(nDotH, SHADING.shininess); \
        specularFactor = specularFactor/(0.1+nDotV); \
        SHADING.specular += in_lightSpecular ## ID * specularFactor; \
    }
}
#define shadeSpotSpecular shadePointSpecular
#define shadeDirectionalSpecular shadePointSpecular

-- toon
#define shadePointDiffuse(PROPS, SHADING, ID, P, N, L, nDotL) { \
    float diffuseFactor; \
    float intensity = dot( L , N ); \
    if( intensity > 0.95 ) diffuseFactor = 1.0; \
    else if( intensity > 0.5  ) diffuseFactor = 0.7; \
    else if( intensity > 0.25 ) diffuseFactor = 0.4; \
    else diffuseFactor = 0.0; \
    SHADING.diffuse = in_lightDiffuse${__ID} * vec4(vec3(diffuseFactor), 1.0); \
}
#define shadeSpotDiffuse shadePointDiffuse
#define shadeDirectionalDiffuse shadePointDiffuse

#define shadePointSpecular(PROPS, SHADING, ID, P, N, L) { \
    const float size=1.0; \
    const float tsmooth=0.25; \
    vec3 h = normalize(L + P.xyz); \
    float specfac = dot(h, N); \
    float ang = acos(specfac); \
    if(ang < size) specfac = 1.0; \
    else if(ang >= (size + tsmooth) || tsmooth == 0.0) specfac = 0.0; \
    else specfac = 1.0 - ((ang - size)/ tsmooth); \
    SHADING.specular += in_lightSpecular ## ID * specfac; \
}
#define shadeSpotSpecular shadePointSpecular
#define shadeDirectionalSpecular shadePointSpecular

