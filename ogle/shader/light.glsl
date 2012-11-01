-- defines
#ifndef __SHADING_DEFINES
#define2 __SHADING_DEFINES
// light defines
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

-- input
#ifndef __IS_LIGHT_DECLARED
#ifdef HAS_LIGHT
#define2 __IS_LIGHT_DECLARED

#include light.defines

#for NUM_LIGHTS
#define2 __ID ${LIGHT${FOR_INDEX}_ID}
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
#endfor
#endif
#endif // __IS_LIGHT_DECLARED

-- init
#include light.input
#for NUM_LIGHTS
#define2 __ID ${LIGHT${FOR_INDEX}_ID}
void shadeTransfer${__ID}(out vec3 lightVec, out float attenuation, vec4 P)
{
#if LIGHT${FOR_INDEX}_TYPE == SPOT
    lightVec = vec3( in_lightPosition${__ID}.xyz - P );
    float spotEffect = dot(
        normalize(in_lightSpotDirection${__ID}),
        normalize(-lightVec));
    if (spotEffect > in_lightInnerConeAngle${__ID}) {
        float dist = length(lightVec);
        attenuation = pow(spotEffect, in_lightSpotExponent${__ID})/(
            in_lightConstantAttenuation${__ID} +
            in_lightLinearAttenuation${__ID} * dist +
            in_lightQuadricAttenuation${__ID} * dist * dist);
    } else {
        attenuation = 0.0;
    }
#elif LIGHT${FOR_INDEX}_TYPE == POINT
    lightVec = vec3( in_lightPosition${__ID}.xyz - P );
    float dist = length(lightVec);
    attenuation = 1.0/(
        in_lightConstantAttenuation${__ID} +
        in_lightLinearAttenuation${__ID} * dist +
        in_lightQuadricAttenuation${__ID} * dist * dist );
#else
    // spot light
    lightVec = in_lightPosition${__ID}.xyz;
    attenuation = 0.0;
#endif
}
#endfor

#endif // __SHADING_PROPERTIES

-- apply
#ifndef __SHADING_SHADE_
#define2 __SHADING_SHADE_

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

#for NUM_LIGHTS
#define2 __ID ${LIGHT${FOR_INDEX}_ID}
void shade${__ID}(
        vec3 lightVec,
        float attenuation,
        inout vec4 ambient,
        inout vec4 diffuse,
        inout vec4 specular,
        inout vec4 emission,
        inout float shininess,
        vec3 P,
        vec3 N)
{
    vec3 L = normalize(lightVec);
    float nDotL = max( dot( N, L ), 0.0 );
    
    ambient += in_lightAmbient${__ID}; 

    if (nDotL > 0.0) {
  #if LIGHT${FOR_INDEX}_TYPE == SPOT
        shadeSpotDiffuse(lightVec,attenuation,diffuse,${__ID},P,N,L,nDotL);
        if(shininess > 0.0) {
            shadeSpotSpecular(lightVec,attenuation,specular,shininess,${__ID},P,N,L);
        }
  #elif LIGHT${FOR_INDEX}_TYPE == POINT
        shadePointDiffuse(lightVec,attenuation,diffuse,${__ID},P,N,L,nDotL);
        if(shininess > 0.0) {
            shadePointSpecular(lightVec,attenuation,specular,shininess,${__ID},P,N,L);
        }
  #else
        // directional light
        shadeDirectionalDiffuse(lightVec,attenuation,diffuse,${__ID},P,N,L,nDotL);
        if(shininess > 0.0) {
            shadeDirectionalSpecular(lightVec,attenuation,specular,shininess,${__ID},P,N,L);
        }
  #endif
    }
}
#endfor

#endif // __SHADING_SHADE_

-- directional.diffuse
#define shadeDirectionalDiffuse(lightVec,attenuation,diffuse,ID, P, N, L, nDotL) { \
    diffuse += in_lightDiffuse ## ID * nDotL; \
}
-- directional.specular
#define shadeDirectionalSpecular(lightVec,attenuation,specular,shininess,ID, P, N, L) { \
    vec3 P = normalize( -P.xyz ); \
    vec3 reflected = normalize( reflect( -L, N ) ); \
    float rDotE = max( dot( reflected, P ), 0.0); \
    specular += attenuation * \
        in_lightSpecular ## ID * pow(rDotE, shininess); \
}

-- point.diffuse
#define shadePointDiffuse(lightVec,attenuation,diffuse,ID, P, N, L, nDotL) { \
    diffuse += attenuation * in_lightDiffuse ## ID * nDotL; \
}
-- point.specular
#define shadePointSpecular(lightVec,attenuation,specular,shininess,ID, P, N, L) { \
    vec3 P = normalize( -P.xyz ); \
    vec3 reflected = normalize( reflect( -L, N ) ); \
    float rDotE = max( dot( reflected, P ), 0.0); \
    specular += attenuation * \
        in_lightSpecular ## ID * pow(rDotE, shininess); \
}

-- spot.diffuse
#define shadeSpotDiffuse(lightVec,attenuation,diffuse,ID, P, N, L, nDotL) { \
    vec3 normalizedSpotDir = normalize(in_lightSpotDirection ## ID); \
    float spotEffect = dot( -L, normalizedSpotDir ); \
    float coneDiff = (in_lightInnerConeAngle ## ID - in_lightOuterConeAngle ## ID); \
    float falloff = clamp((spotEffect - in_lightOuterConeAngle ## ID) / coneDiff, 0.0, 1.0); \
    diffuse += attenuation * \
        in_lightDiffuse ## ID * nDotL * falloff; \
}
-- spot.specular
#define shadeSpotSpecular(lightVec,attenuation,specular,shininess, ID, P, N, L) { \
    vec3 P = normalize( -P.xyz ); \
    vec3 reflected = normalize( reflect( -L, N ) ); \
    float rDotE = max( dot( reflected, P ), 0.0); \
    specular += in_lightSpecular ## ID * pow(rDotE, shininess); \
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
#define shadePointSpecular(lightVec,attenuation,specular,shininess,ID, P, N, L) { \
    vec3 P = normalize( -P.xyz ); \
    vec3 halfVecNormalized = normalize( L + P ); \
    specular += in_lightSpecular ## ID * \
        pow( max(0.0,dot(N,halfVecNormalized)), shininess); \
}
#define shadeSpotSpecular shadePointSpecular
#define shadeDirectionalSpecular shadePointSpecular

-- oren-nayer
#include light.point.specular
#include light.spot.specular
#include light.directional.specular
#define shadePointDiffuse(lightVec,attenuation,diffuse, ID, P, N, L, nDotL) { \
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
    diffuse += in_lightDiffuse ## ID * cos_theta_i * (a+b); \
}
#define shadeSpotDiffuse shadePointDiffuse
#define shadeDirectionalDiffuse shadePointDiffuse

-- minnaert
#include light.point.specular
#include light.spot.specular
#include light.directional.specular
#define shadePointDiffuse(lightVec,attenuation,diffuse,ID, P, N, L, nDotL) { \
    diffuse += in_lightDiffuse ## ID * pow(nDotL, in_matDarkness); \
}
#define shadeSpotDiffuse shadePointDiffuse
#define shadeDirectionalDiffuse shadePointDiffuse

-- cook-torrance
#include light.point.diffuse
#include light.spot.diffuse
#include light.directional.diffuse
#define shadePointSpecular(lightVec,attenuation,specular,shininess,ID, P, N, L) { \
    vec3 P = normalize( -P.xyz ); \
    vec3 halfVecNormalized = normalize( L + P ); \
    float nDotH = dot(N, halfVecNormalized); \
    if(nDotH >= 0.0) { \
        float nDotV = max(dot(N, P), 0.0); \
        float specularFactor = pow(nDotH, shininess); \
        specularFactor = specularFactor/(0.1+nDotV); \
        specular += in_lightSpecular ## ID * specularFactor; \
    }
}
#define shadeSpotSpecular shadePointSpecular
#define shadeDirectionalSpecular shadePointSpecular

-- toon
#define shadePointDiffuse(lightVec,attenuation, diffuse, ID, P, N, L, nDotL) { \
    float diffuseFactor; \
    float intensity = dot( L , N ); \
    if( intensity > 0.95 ) diffuseFactor = 1.0; \
    else if( intensity > 0.5  ) diffuseFactor = 0.7; \
    else if( intensity > 0.25 ) diffuseFactor = 0.4; \
    else diffuseFactor = 0.0; \
    diffuse += in_lightDiffuse${__ID} * vec4(vec3(diffuseFactor), 1.0); \
}
#define shadeSpotDiffuse shadePointDiffuse
#define shadeDirectionalDiffuse shadePointDiffuse

#define shadePointSpecular(lightVec,attenuation,specular,shininess,ID, P, N, L) { \
    const float size=1.0; \
    const float tsmooth=0.25; \
    vec3 h = normalize(L + P.xyz); \
    float specfac = dot(h, N); \
    float ang = acos(specfac); \
    if(ang < size) specfac = 1.0; \
    else if(ang >= (size + tsmooth) || tsmooth == 0.0) specfac = 0.0; \
    else specfac = 1.0 - ((ang - size)/ tsmooth); \
    specular += in_lightSpecular ## ID * specfac; \
}
#define shadeSpotSpecular shadePointSpecular
#define shadeDirectionalSpecular shadePointSpecular

