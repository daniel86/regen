
-- shade
#ifndef __SHADE_
#define2 __SHADE_

#define SM_NUM_SLICES 3

struct Shading {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

#for NUM_LIGHTS
#define2 __ID ${LIGHT${FOR_INDEX}_ID}
    uniform vec3 in_lightAmbient${__ID};
    uniform vec3 in_lightDiffuse${__ID};
    uniform vec3 in_lightSpecular${__ID};
  #if LIGHT${FOR_INDEX}_TYPE == SPOT
    // spot light
    uniform vec3 in_lightPosition${__ID};
    uniform vec2 in_lightConeAngle${__ID};
    uniform vec3 in_lightSpotDirection${__ID};
    uniform float in_lightSpotExponent${__ID};
    uniform vec3 in_lightAttenuation${__ID};
    #ifdef LIGHT${FOR_INDEX}_HAS_SM
    uniform mat4 shadowMatrix${__ID};
    uniform sampler2DShadow shadowMap${__ID};
    #endif
  #endif
  #if LIGHT${FOR_INDEX}_TYPE == POINT
    // point light
    uniform vec3 in_lightPosition${__ID};
    uniform vec3 in_lightAttenuation${__ID};
    #ifdef LIGHT${FOR_INDEX}_HAS_SM
    uniform mat4 shadowMatrices${__ID}[6];
    uniform samplerCubeShadow shadowMap${__ID};
    #endif
  #endif
  #if LIGHT${FOR_INDEX}_TYPE == DIRECTIONAL
    // directional light
    uniform vec3 in_lightDirection${__ID};
    #ifdef LIGHT${FOR_INDEX}_HAS_SM
    uniform float shadowFar${__ID}[SM_NUM_SLICES];
    uniform mat4 shadowMatrices${__ID}[SM_NUM_SLICES];
    uniform sampler2DArrayShadow shadowMap${__ID};
    #endif
  #endif
#endfor

#include shadow-mapping.sampling

void specularLight(vec3 L, vec3 N, vec3 E,
        vec3 lightSpecular, float shininess, float attenuation,
        inout vec3 ret)
{
    if(shininess > 0.0) {
        float rDotE = max( dot(
            normalize( reflect( -L, N ) ),
            normalize( -E ) ), 0.0);
        ret += lightSpecular * (attenuation * pow(rDotE, shininess));
    }
}
float getLightAttenuation(
        vec3 P,
        vec3 lightPosition,
        vec3 lightAttenuation)
{
    float dist = length(lightPosition - P);
    return 1.0/(lightAttenuation.x + lightAttenuation.y*dist + lightAttenuation.z*dist*dist);
}

void shadeDirectional(
        inout Shading s,
        vec3 P, vec3 E, vec3 N, float shininess,
        vec3 lightDirection,
        vec3 lightAmbient,
        vec3 lightDiffuse,
        vec3 lightSpecular)
{
    s.ambient += lightAmbient;
    
    vec3 L = normalize(lightDirection);
    float nDotL = dot( N, L );
    if(nDotL <= 0.0) { return; }

    s.diffuse += lightDiffuse * nDotL;
    specularLight(L,N,E,lightSpecular,shininess,1.0,s.specular);
}
void shadeDirectionalSM(
        inout Shading s,
        vec3 P, vec3 E, vec3 N, float shininess,
        vec3 lightDirection,
        vec3 lightAmbient,
        vec3 lightDiffuse,
        vec3 lightSpecular,
        sampler2DArrayShadow shadowMap,
        float shadowFar[SM_NUM_SLICES],
        mat4 shadowMatrices[SM_NUM_SLICES])
{
    s.ambient += lightAmbient;
    
    vec3 L = normalize(lightDirection);
    float nDotL = dot( N, L );
    if(nDotL <= 0.0) { return; }

    float shadow = sampleDirectionalShadow(
        P, shadowMap, shadowFar, shadowMatrices);
    if(shadow < 0.0001) { return; }
    
    s.diffuse += lightDiffuse * nDotL;
    specularLight(L,N,E,lightSpecular,shininess,shadow,s.specular);
}

void shadePoint(
        inout Shading s,
        vec3 P, vec3 E, vec3 N, float shininess,
        vec3 lightPosition,
        vec3 lightAmbient,
        vec3 lightDiffuse,
        vec3 lightSpecular,
        vec3 lightAttenuation)
{
    s.ambient += lightAmbient;
    
    vec3 L = normalize(lightPosition - P);
    float nDotL = dot( N, L );
    if(nDotL <= 0.0) { return; }
    
    float attenuation = getLightAttenuation(P,lightPosition,lightAttenuation);
    if(attenuation < 0.0001) { return; }
    
    s.diffuse += lightDiffuse * (attenuation * nDotL);
    specularLight(L,N,E,lightSpecular,shininess,attenuation,s.specular);
}
void shadePointSM(
        inout Shading s,
        vec3 P, vec3 E, vec3 N, float shininess,
        vec3 lightPosition,
        vec3 lightAmbient,
        vec3 lightDiffuse,
        vec3 lightSpecular,
        vec3 lightAttenuation,
        samplerCubeShadow shadowMap,
        mat4 shadowMatrices[6])
{
    s.ambient += lightAmbient;
    
    vec3 L = normalize(lightPosition - P);
    float nDotL = dot( N, L );
    if(nDotL <= 0.0) { return; }
    
    float attenuation = getLightAttenuation(P,lightPosition,lightAttenuation);
    if(attenuation < 0.0001) { return; }
    attenuation *= samplePointShadow(P, shadowMap, shadowMatrices);
    if(attenuation < 0.0001) { return; }
    
    s.diffuse += lightDiffuse * (attenuation * nDotL);
    specularLight(L,N,E,lightSpecular,shininess,attenuation,s.specular);
}

void shadeSpot(
        inout Shading s,
        vec3 P, vec3 E, vec3 N, float shininess,
        vec3 lightPosition,
        vec3 lightAmbient,
        vec3 lightDiffuse,
        vec3 lightSpecular,
        vec3 lightAttenuation,
        vec3 spotDirection,
        float spotExponent,
        vec2 coneAngle)
{
    s.ambient += lightAmbient;
    
    vec3 L = normalize(lightPosition - P);
    float nDotL = dot( N, L );
    if(nDotL <= 0.0) { return; }

    float spotEffect = dot( -L, normalize(spotDirection) );
    if(spotEffect <= coneAngle.x) { return; }

    float attenuation = getLightAttenuation(P,lightPosition,lightAttenuation);
    attenuation *= pow(spotEffect, spotExponent);
    attenuation *= clamp(
        (spotEffect - coneAngle.y)/(coneAngle.x - coneAngle.y), 0.0, 1.0);
    if(attenuation < 0.0001) { return; }

    s.diffuse += lightDiffuse * (attenuation * nDotL);
    specularLight(L,N,E,lightSpecular,shininess,attenuation,s.specular);
}
void shadeSpotSM(
        inout Shading s,
        vec3 P, vec3 E, vec3 N, float shininess,
        vec3 lightPosition,
        vec3 lightAmbient,
        vec3 lightDiffuse,
        vec3 lightSpecular,
        vec3 lightAttenuation,
        vec3 spotDirection,
        float spotExponent,
        vec2 coneAngle,
        sampler2DShadow shadowMap,
        mat4 shadowMatrix)
{
    s.ambient += lightAmbient;
    
    vec3 L = normalize(lightPosition - P);
    float nDotL = dot( N, L );
    if(nDotL <= 0.0) { return; }

    float spotEffect = dot( -L, normalize(spotDirection) );
    if(spotEffect <= coneAngle.x) { return; }

    float attenuation = getLightAttenuation(P,lightPosition,lightAttenuation);
    attenuation *= pow(spotEffect, spotExponent);
    attenuation *= clamp(
        (spotEffect - coneAngle.y)/(coneAngle.x - coneAngle.y), 0.0, 1.0);
    if(attenuation < 0.0001) { return; }
    attenuation *= sampleSpotShadow(P, shadowMap, shadowMatrix);
    if(attenuation < 0.0001) { return; }

    s.diffuse += lightDiffuse * (attenuation * nDotL);
    specularLight(L,N,E,lightSpecular,shininess,attenuation,s.specular);
}

Shading shade(vec3 P, vec3 N, float shininess)
{
  vec3 E = normalize(P - in_cameraPosition);
  Shading s;
  s.ambient = vec3(0.0);
  s.diffuse = vec3(0.0);
  s.specular = vec3(0.0);
#for NUM_LIGHTS
#define2 __ID ${LIGHT${FOR_INDEX}_ID}
#ifdef LIGHT${FOR_INDEX}_HAS_SM
  #if LIGHT${FOR_INDEX}_TYPE == SPOT
    shadeSpotSM(
        s, P, E, N, shininess,
        in_lightPosition${__ID},
        in_lightAmbient${__ID},
        in_lightDiffuse${__ID},
        in_lightSpecular${__ID},
        in_lightAttenuation${__ID},
        in_lightSpotDirection${__ID},
        in_lightSpotExponent${__ID},
        in_lightConeAngle${__ID},
        in_shadowMap${__ID},
        in_shadowMatrix${__ID}
    );
  #endif
  #if LIGHT${FOR_INDEX}_TYPE == POINT
    shadePointSM(
        s, P, E, N, shininess,
        in_lightPosition${__ID},
        in_lightAmbient${__ID},
        in_lightDiffuse${__ID},
        in_lightSpecular${__ID},
        in_lightAttenuation${__ID},
        in_shadowMap${__ID},
        in_shadowMatrices${__ID}
    );
  #endif
  #if LIGHT${FOR_INDEX}_TYPE == DIRECTIONAL
    shadeDirectionalSM(
        s, P, E, N, shininess,
        in_lightDirection${__ID},
        in_lightAmbient${__ID},
        in_lightDiffuse${__ID},
        in_lightSpecular${__ID},
        in_shadowMap${__ID},
        in_shadowFar${__ID},
        in_shadowMatrices${__ID}
    );
  #endif
#else
  #if LIGHT${FOR_INDEX}_TYPE == SPOT
    shadeSpot(
        s, P, E, N, shininess,
        in_lightPosition${__ID},
        in_lightAmbient${__ID},
        in_lightDiffuse${__ID},
        in_lightSpecular${__ID},
        in_lightAttenuation${__ID},
        in_lightSpotDirection${__ID},
        in_lightSpotExponent${__ID},
        in_lightConeAngle${__ID}
    );
  #endif
  #if LIGHT${FOR_INDEX}_TYPE == POINT
    shadePoint(
        s, P, E, N, shininess,
        in_lightPosition${__ID},
        in_lightAmbient${__ID},
        in_lightDiffuse${__ID},
        in_lightSpecular${__ID},
        in_lightAttenuation${__ID}
    );
  #endif
  #if LIGHT${FOR_INDEX}_TYPE == DIRECTIONAL
    shadeDirectional(
        s, P, E, N, shininess,
        in_lightDirection${__ID},
        in_lightAmbient${__ID},
        in_lightDiffuse${__ID},
        in_lightSpecular${__ID}
    );
  #endif
#endif
#endfor
  return s;
}
#endif // __SHADE_

