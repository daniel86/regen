
-- init
#define shadeDirectionalProperties(PROPS, P, INDEX, ID) { \
    PROPS.lightVec[INDEX] = in_lightPosition ## ID.xyz; \
    PROPS.attenuation[INDEX] = 0.0; \
}
#define shadePointProperties(PROPS, P, INDEX, ID) { \
    PROPS.lightVec[INDEX] = vec3( in_lightPosition ## ID.xyz - P ); \
    float dist = length(PROPS.lightVec[INDEX]); \
    PROPS.attenuation[INDEX] = 1.0/( \
        in_lightConstantAttenuation ## ID + \
        in_lightLinearAttenuation ## ID * dist + \
        in_lightQuadricAttenuation ## ID * dist * dist ); \
}
#define shadeSpotProperties(PROPS, P, INDEX, ID) { \
    PROPS.lightVec[INDEX] = vec3( in_lightPosition ## ID.xyz - P ); \
    float spotEffect = dot( normalize(in_lightSpotDirection ## ID), normalize(-PROPS.lightVec[INDEX])); \
    if (spotEffect > in_lightInnerConeAngle[i]) { \
        float dist = length(PROPS.lightVec[INDEX]); \
        PROPS.attenuation[INDEX] = pow(spotEffect, in_lightSpotExponent ## ID)/( \
            in_lightConstantAttenuation ## ID + \
            in_lightLinearAttenuation ## ID * dist + \
            in_lightQuadricAttenuation ## ID * dist * dist); \
    } else { \
        PROPS.attenuation[INDEX] = 0.0; \
    } \
}

-- shade
#if SHADING == TOON
#include shading.toon
#elif SHADING == GOURAD
#include shading.gourad
#elif SHADING == BLINN
#include shading.blinn
#elif SHADING == ORENNAYER
#include shading.oren-nayer
#else
#include shading.phong
#endif

#define shadeDirectionalLight(PROPS, SHADING, N, INDEX, ID) { \
    SHADING.ambient += in_lightAmbient ## ID; \
    vec3 L = normalize(PROPS.lightVec[INDEX]); \
    float nDotL = max( dot( N, L ), 0.0 ); \
    if (nDotL > 0.0) {; \
        shadeDirectionalDiffuse(PROPS,SHADING,INDEX,ID,N,L,nDotL); \
        if(SHADING.shininess > 0.0) shadeDirectionalSpecular(PROPS,SHADING,INDEX,ID,N,L); \
    } \
}
#define shadePointLight(PROPS, SHADING, N, INDEX, ID) { \
    SHADING.ambient += in_lightAmbient ## ID; \
    vec3 L = normalize(PROPS.lightVec[INDEX]); \
    float nDotL = max( dot( N, L ), 0.0 ); \
    if (nDotL > 0.0) { \
        shadePointDiffuse(PROPS,SHADING,INDEX,ID,N,L,nDotL); \
        if(SHADING.shininess > 0.0) shadePointSpecular(PROPS,SHADING,INDEX,ID,N,L); \
    } \
}
#define shadeSpotLight(PROPS, SHADING, N, INDEX, ID) { \
    SHADING.ambient += in_lightAmbient ## ID; \
    vec3 L = normalize(PROPS.lightVec[INDEX]); \
    float nDotL = max( dot( N, L ), 0.0 ); \
    if (nDotL > 0.0) { \
        shadeSpotDiffuse(PROPS,SHADING,INDEX,ID,N,L,nDotL); \
        if(SHADING.shininess > 0.0) shadeSpotSpecular(PROPS,SHADING,INDEX,ID,N,L); \
    } \
}

-- directional.diffuse
#define shadeDirectionalDiffuse(PROPS, SHADING, INDEX, ID, N, L, nDotL) { \
    SHADING.diffuse += in_lightDiffuse ## ID * nDotL; \
}
-- directional.specular
#define shadeDirectionalSpecular(PROPS, SHADING, INDEX, ID, N, L) { \
    vec3 P = normalize( -in_posWorld.xyz ); \
    vec3 reflected = normalize( reflect( -L, N ) ); \
    float rDotE = max( dot( reflected, P ), 0.0); \
    SHADING.specular += PROPS.attenuation[INDEX] * in_lightSpecular ## ID * pow(rDotE, SHADING.shininess); \
}

-- point.diffuse
#define shadePointDiffuse(PROPS, SHADING, INDEX, ID, N, L, nDotL) { \
    SHADING.diffuse += PROPS.attenuation[INDEX] * in_lightDiffuse ## ID * nDotL; \
}
-- point.specular
#define shadePointSpecular(PROPS, SHADING, INDEX, ID, N, L) { \
    vec3 P = normalize( -in_posWorld.xyz ); \
    vec3 reflected = normalize( reflect( -L, N ) ); \
    float rDotE = max( dot( reflected, P ), 0.0); \
    SHADING.specular += PROPS.attenuation[INDEX] * in_lightSpecular ## ID * pow(rDotE, SHADING.shininess); \
}

-- spot.diffuse
#define shadeSpotDiffuse(PROPS, SHADING, INDEX, ID, N, L, nDotL) { \
    vec3 normalizedSpotDir = normalize(in_lightSpotDirection ## ID); \
    float spotEffect = dot( -L, normalizedSpotDir ); \
    float coneDiff = (in_lightInnerConeAngle ## ID - in_lightOuterConeAngle ## ID); \
    float falloff = clamp((spotEffect - in_lightOuterConeAngle ## ID) / coneDiff, 0.0, 1.0); \
    SHADING.diffuse += PROPS.attenuation[INDEX] * in_lightDiffuse ## ID * nDotL * falloff; \
}
-- spot.specular
#define shadeSpotSpecular(PROPS, SHADING, INDEX, ID, N, L) { \
    vec3 P = normalize( -in_posWorld.xyz ); \
    vec3 reflected = normalize( reflect( -L, N ) ); \
    float rDotE = max( dot( reflected, P ), 0.0); \
    SHADING.specular += in_lightSpecular ## ID * pow(rDotE, SHADING.shininess); \
}

-- phong
#include shading.point.diffuse
#include shading.point.specular
#include shading.spot.diffuse
#include shading.spot.specular
#include shading.directional.diffuse
#include shading.directional.specular

-- gourad
#include shading.point.diffuse
#include shading.point.specular
#include shading.spot.diffuse
#include shading.spot.specular
#include shading.directional.diffuse
#include shading.directional.specular

-- blinn
#include shading.point.diffuse
#include shading.spot.diffuse
#include shading.directional.diffuse
#define shadePointSpecular(PROPS, SHADING, INDEX, ID, N, L) { \
    vec3 P = normalize( -in_posWorld.xyz ); \
    vec3 halfVecNormalized = normalize( L + P ); \
    SHADING.specular += in_lightSpecular ## ID * pow( max(0.0,dot(N,halfVecNormalized)), SHADING.shininess); \
}
#define shadeSpotSpecular shadePointSpecular
#define shadeDirectionalSpecular shadePointSpecular

-- oren-nayer
#include shading.point.specular
#include shading.spot.specular
#include shading.directional.specular
#define shadePointDiffuse(PROPS, SHADING, INDEX, ID, N, L, nDotL) { \
    vec3 V = normalize( -in_posWorld.xyz ); \
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
#include shading.point.specular
#include shading.spot.specular
#include shading.directional.specular
#define shadePointDiffuse(PROPS, SHADING, INDEX, ID, N, L, nDotL) { \
    SHADING.diffuseVar += in_lightDiffuse ## ID * pow(nDotL, in_matDarkness); \
}
#define shadeSpotDiffuse shadePointDiffuse
#define shadeDirectionalDiffuse shadePointDiffuse

-- cook-torrance
#include shading.point.diffuse
#include shading.spot.diffuse
#include shading.directional.diffuse
#define shadePointSpecular(PROPS, SHADING, INDEX, ID, N, L) { \
    vec3 P = normalize( -in_posWorld.xyz ); \
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
#define shadePointDiffuse(PROPS, SHADING, INDEX, ID, N, L, nDotL) { \
    float diffuseFactor; \
    float intensity = dot( L , N ); \
    if( intensity > 0.95 ) diffuseFactor = 1.0; \
    else if( intensity > 0.5  ) diffuseFactor = 0.7; \
    else if( intensity > 0.25 ) diffuseFactor = 0.4; \
    else diffuseFactor = 0.0; \
    SHADING.diffuse = in_lightDiffuse ## ID * vec4(vec3(diffuseFactor), 1.0); \
}
#define shadeSpotDiffuse shadePointDiffuse
#define shadeDirectionalDiffuse shadePointDiffuse

#define shadePointSpecular(PROPS, SHADING, INDEX, ID, N, L) { \
    const float size=1.0; \
    const float tsmooth=0.25; \
    vec3 h = normalize(L + in_posWorld.xyz); \
    float specfac = dot(h, N); \
    float ang = acos(specfac); \
    if(ang < size) specfac = 1.0; \
    else if(ang >= (size + tsmooth) || tsmooth == 0.0) specfac = 0.0; \
    else specfac = 1.0 - ((ang - size)/ tsmooth); \
    SHADING.specular += in_lightSpecular ## ID * specfac; \
}
#define shadeSpotSpecular shadePointSpecular
#define shadeDirectionalSpecular shadePointSpecular

