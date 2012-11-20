
-- shade
#ifndef __SHADE_
#define2 __SHADE_

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
    uniform vec3 in_lightPosition${__ID};
    uniform vec2 in_lightConeAngle${__ID};
    uniform vec3 in_lightSpotDirection${__ID};
    uniform float in_lightSpotExponent${__ID};
    uniform vec3 in_lightAttenuation${__ID};
  #endif
  #if LIGHT${FOR_INDEX}_TYPE == POINT
    uniform vec3 in_lightPosition${__ID};
    uniform vec3 in_lightAttenuation${__ID};
  #endif
  #if LIGHT${FOR_INDEX}_TYPE == DIRECTIONAL
    uniform vec3 in_lightDirection${__ID};
  #endif
#endfor

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
    if(shininess > 0.0) {
        float rDotE = max( dot(
            normalize( reflect( -L, N ) ),
            normalize( -E ) ), 0.0);
        s.specular += lightSpecular * pow(rDotE, shininess);
    }
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

    vec3 lightVec = (lightPosition - P);
    vec3 L = normalize(lightVec);
    
    float nDotL = dot( N, L );
    if(nDotL <= 0.0) { return; }
    
    float dist = length(lightVec);
    float attenuation = 1.0/(
        lightAttenuation.x + lightAttenuation.y*dist + lightAttenuation.z*dist*dist );

    s.diffuse += lightDiffuse * (attenuation * nDotL);
    if(shininess > 0.0) {
        float rDotE = max( dot(
            normalize( reflect( -L, N ) ),
            normalize( -E ) ), 0.0);
        s.specular += lightSpecular * (attenuation * pow(rDotE, shininess));
    }
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
    
    vec3 lightVec = (lightPosition - P);
    vec3 L = normalize(lightVec);
    
    float nDotL = dot( N, L );
    if(nDotL <= 0.0) { return; }

    float spotEffect = dot( -L, normalize(spotDirection) );
    if(spotEffect <= coneAngle.x) { return; }

    float dist = length(lightVec);
    float attenuation = pow(spotEffect, spotExponent)/(
        lightAttenuation.x + lightAttenuation.y*dist + lightAttenuation.z*dist*dist );
    attenuation *= clamp(
        (spotEffect - coneAngle.y)/(coneAngle.x - coneAngle.y), 0.0, 1.0);

    s.diffuse += lightDiffuse * (attenuation * nDotL);
    if(shininess > 0.0) {
        float rDotE = max( dot(
            normalize( reflect( -L, N ) ),
            normalize( -E ) ), 0.0);
        s.specular += lightSpecular * (attenuation * pow(rDotE, shininess));
    }
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
#endfor
    return s;
}
#endif // __SHADE_

