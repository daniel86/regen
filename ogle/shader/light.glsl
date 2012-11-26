
-- shade
#ifndef __SHADE_
#define2 __SHADE_

// #define SM_DEBUG_SLICES

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
#ifdef LIGHT_IS_ATTENUATED${__ID}
    uniform vec3 in_lightAttenuation${__ID};
#endif
  #if LIGHT${FOR_INDEX}_TYPE == SPOT
    // spot light
    uniform vec3 in_lightPosition${__ID};
    uniform vec2 in_lightConeAngles${__ID};
    uniform vec3 in_lightSpotDirection${__ID};
    #ifdef LIGHT${__ID}_HAS_SM
    uniform mat4 in_shadowMatrix${__ID};
    uniform sampler2DShadow shadowMap${__ID};
    #endif
  #endif
  #if LIGHT${FOR_INDEX}_TYPE == POINT
    // point light
    uniform vec3 in_lightPosition${__ID};
    #ifdef LIGHT${__ID}_HAS_SM
    uniform mat4 in_shadowMatrices${__ID}[6];
    uniform samplerCubeShadow shadowMap${__ID};
    #endif
  #endif
  #if LIGHT${FOR_INDEX}_TYPE == DIRECTIONAL
    // directional light
    uniform vec3 in_lightDirection${__ID};
    #ifdef LIGHT${__ID}_HAS_SM
    uniform float in_shadowFar${__ID}[NUM_SHADOW_MAP_SLICES];
    uniform mat4 in_shadowMatrices${__ID}[NUM_SHADOW_MAP_SLICES];
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
    return min(1.0, 1.0/(
        lightAttenuation.x +
        lightAttenuation.y*dist +
        lightAttenuation.z*dist*dist));
}

#ifdef SM_DEBUG_SLICES
void debugShadowSlice(int shadowMapIndex, inout vec3 diffuse)
{
    vec3 color[8] = vec3[8](
        vec3(1.0, 0.7, 0.7),
        vec3(0.7, 1.0, 0.7),
        vec3(0.7, 0.7, 1.0),
        vec3(1.0, 1.0, 0.7),
        vec3(1.0, 0.7, 1.0),
        vec3(0.7, 1.0, 1.0),
        vec3(1.0, 1.0, 1.0),
        vec3(0.7, 0.7, 0.7));
    diffuse *= color[shadowMapIndex];
}
#endif

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
#ifdef NUM_SHADOW_MAP_SLICES
void shadeDirectionalSM(
        inout Shading s,
        vec3 P, vec3 E, vec3 N, float depth, float shininess,
        vec3 lightDirection,
        vec3 lightAmbient,
        vec3 lightDiffuse,
        vec3 lightSpecular,
        sampler2DArrayShadow shadowMap,
        float shadowFar[NUM_SHADOW_MAP_SLICES],
        mat4 shadowMatrices[NUM_SHADOW_MAP_SLICES])
{
    s.ambient += lightAmbient;
    
    vec3 L = normalize(lightDirection);
    float nDotL = dot( N, L );
    if(nDotL <= 0.0) { return; }

    float shadow = sampleDirectionalShadow(
        P, depth, shadowMap, shadowFar, shadowMatrices);
    if(shadow < 0.0001) { return; }
    
    s.diffuse += lightDiffuse * (shadow * nDotL);
    specularLight(L,N,E,lightSpecular,shininess,shadow,s.specular);

#ifdef SM_DEBUG_SLICES
    int shadowLayer = getShadowLayer(depth, shadowFar);
    debugShadowSlice(shadowLayer, s.diffuse);
#endif
}
#endif

void shadePoint(
        inout Shading s,
        vec3 P, vec3 E, vec3 N,
        float shininess,
        float attenuation,
        vec3 lightPosition,
        vec3 lightAmbient,
        vec3 lightDiffuse,
        vec3 lightSpecular)
{
    s.ambient += lightAmbient;
    
    vec3 L = normalize(lightPosition - P);
    float nDotL = dot( N, L );
    if(nDotL <= 0.0) { return; }
    if(attenuation < 0.0001) { return; }
    
    s.diffuse += lightDiffuse * (attenuation * nDotL);
    specularLight(L,N,E,lightSpecular,shininess,attenuation,s.specular);
}
void shadePointSM(
        inout Shading s,
        vec3 P, vec3 E, vec3 N,
        float shininess,
        float attenuation,
        float depth,
        vec3 lightPosition,
        vec3 lightAmbient,
        vec3 lightDiffuse,
        vec3 lightSpecular,
        samplerCubeShadow shadowMap,
        mat4 shadowMatrices[6])
{
    s.ambient += lightAmbient;
    
    vec3 L = normalize(lightPosition - P);
    float nDotL = dot( N, L );
    if(nDotL <= 0.0) { return; }
    
    float amount = attenuation;
    if(amount < 0.0001) { return; }
    amount *= samplePointShadow(P, L, shadowMap, shadowMatrices);
    if(amount < 0.0001) { return; }
    
    s.diffuse += lightDiffuse * (amount * nDotL);
    specularLight(L,N,E,lightSpecular,shininess,amount,s.specular);
}

#define innerAngle coneAngles.x
#define outerAngle coneAngles.y
void shadeSpot(
        inout Shading s,
        vec3 P, vec3 E, vec3 N,
        float shininess,
        float attenuation,
        vec3 lightPosition,
        vec3 lightAmbient,
        vec3 lightDiffuse,
        vec3 lightSpecular,
        vec3 spotDirection,
        vec2 coneAngles)
{
    s.ambient += lightAmbient;
    
    vec3 L = normalize(lightPosition - P);
    float nDotL = dot( N, L );
    if(nDotL <= 0.0) { return; }

    float spotEffect = dot( -L, normalize(spotDirection) );
    if(spotEffect <= innerAngle) { return; }

    float amount = attenuation;
    amount *= clamp(
        1.0 - (spotEffect-outerAngle)/(innerAngle-outerAngle), 0.0, 1.0);
    if(amount < 0.0001) { return; }

    s.diffuse += lightDiffuse * (amount * nDotL);
    specularLight(L,N,E,lightSpecular,shininess,amount,s.specular);
}
void shadeSpotSM(
        inout Shading s,
        vec3 P, vec3 E, vec3 N,
        float shininess,
        float attenuation,
        vec3 lightPosition,
        vec3 lightAmbient,
        vec3 lightDiffuse,
        vec3 lightSpecular,
        vec3 spotDirection,
        vec2 coneAngles,
        sampler2DShadow shadowMap,
        mat4 shadowMatrix)
{
    s.ambient += lightAmbient;
    
    vec3 L = normalize(lightPosition - P);
    float nDotL = dot( N, L );
    if(nDotL <= 0.0) { return; }

    float spotEffect = dot( -L, normalize(spotDirection) );
    if(spotEffect <= innerAngle) { return; }

    float amount = attenuation * clamp(
        1.0 - (spotEffect-outerAngle)/(innerAngle-outerAngle), 0.0, 1.0);
    amount *= sampleSpotShadow(P, shadowMap, shadowMatrix);
    if(amount < 0.0001) { return; }

    s.diffuse += lightDiffuse * (amount * nDotL);
    specularLight(L,N,E,lightSpecular,shininess,amount,s.specular);
}
#undef innerAngle
#undef outerAngle

Shading shade(vec3 P, vec3 N, float depth, float shininess)
{
  float attenuation;
  vec3 E = normalize(P - in_cameraPosition);
  Shading s;
  s.ambient = vec3(0.0);
  s.diffuse = vec3(0.0);
  s.specular = vec3(0.0);
#for NUM_LIGHTS
#define2 __ID ${LIGHT${FOR_INDEX}_ID}
#if LIGHT${FOR_INDEX}_TYPE == DIRECTIONAL
  #ifdef LIGHT${__ID}_HAS_SM
    shadeDirectionalSM(
        s, P, E, N, depth, shininess,
        in_lightDirection${__ID},
        in_lightAmbient${__ID},
        in_lightDiffuse${__ID},
        in_lightSpecular${__ID},
        shadowMap${__ID},
        in_shadowFar${__ID},
        in_shadowMatrices${__ID}
    );
  #else
    shadeDirectional(
        s, P, E, N, shininess,
        in_lightDirection${__ID},
        in_lightAmbient${__ID},
        in_lightDiffuse${__ID},
        in_lightSpecular${__ID}
    );
  #endif
#else
  #ifdef LIGHT_IS_ATTENUATED${__ID}
    attenuation = getLightAttenuation(
        P,in_lightPosition${__ID},in_lightAttenuation${__ID});
  #else
    attenuation = 1.0;
  #endif

  #ifdef LIGHT${__ID}_HAS_SM
    #if LIGHT${FOR_INDEX}_TYPE == SPOT
    shadeSpotSM(
        s, P, E, N, shininess, attenuation,
        in_lightPosition${__ID},
        in_lightAmbient${__ID},
        in_lightDiffuse${__ID},
        in_lightSpecular${__ID},
        in_lightSpotDirection${__ID},
        in_lightConeAngles${__ID},
        shadowMap${__ID},
        in_shadowMatrix${__ID}
    );
    #endif
    #if LIGHT${FOR_INDEX}_TYPE == POINT
    shadePointSM(
        s, P, E, N, shininess, attenuation, depth,
        in_lightPosition${__ID},
        in_lightAmbient${__ID},
        in_lightDiffuse${__ID},
        in_lightSpecular${__ID},
        shadowMap${__ID},
        in_shadowMatrices${__ID}
    );
    #endif
  #else
    #if LIGHT${FOR_INDEX}_TYPE == SPOT
    shadeSpot(
        s, P, E, N, shininess, attenuation,
        in_lightPosition${__ID},
        in_lightAmbient${__ID},
        in_lightDiffuse${__ID},
        in_lightSpecular${__ID},
        in_lightSpotDirection${__ID},
        in_lightConeAngles${__ID}
    );
    #endif
    #if LIGHT${FOR_INDEX}_TYPE == POINT
    shadePoint(
        s, P, E, N, shininess, attenuation,
        in_lightPosition${__ID},
        in_lightAmbient${__ID},
        in_lightDiffuse${__ID},
        in_lightSpecular${__ID}
    );
  #endif
#endif
#endif
#endfor

  return s;
}
#endif // __SHADE_

