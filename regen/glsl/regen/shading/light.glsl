
-- input.deferred
#ifndef __light_inputs_included_
#define2 __light_inputs_included_
uniform vec3 in_lightDiffuse;
uniform vec3 in_lightSpecular;

#ifdef IS_SPOT_LIGHT
uniform vec3 in_lightPosition;
uniform vec2 in_lightRadius;
uniform vec3 in_lightDirection;
uniform vec2 in_lightConeAngles;
uniform mat4 in_modelMatrix;
#ifdef USE_SHADOW_MAP
uniform float in_lightFar;
uniform float in_lightNear;
uniform vec2 in_shadowInverseSize;
uniform sampler2DShadow in_shadowTexture;
uniform mat4 in_lightMatrix;
#endif // USE_SHADOW_MAP
#endif

#ifdef IS_POINT_LIGHT
uniform vec3 in_lightPosition;
uniform vec2 in_lightRadius;
#ifdef USE_SHADOW_MAP
uniform float in_lightFar;
uniform float in_lightNear;
uniform vec2 in_shadowInverseSize;
uniform samplerCubeShadow in_shadowTexture;
uniform mat4 in_lightMatrix[6];
#endif // USE_SHADOW_MAP
#endif

#ifdef IS_DIRECTIONAL_LIGHT
uniform vec3 in_lightDirection;
#ifdef USE_SKY_COLOR
uniform samplerCube in_skyColorTexture;
#endif
#ifdef USE_SHADOW_MAP
uniform sampler2DArrayShadow in_shadowTexture;
uniform vec2 in_shadowInverseSize;
uniform mat4 in_lightMatrix[NUM_SHADOW_LAYER];
uniform float in_lightFar[NUM_SHADOW_LAYER];
#endif
#endif
#endif // __light_inputs_included_

-- input.direct
#ifndef __light_inputs_included_
#define2 __light_inputs_included_
#for INDEX to NUM_LIGHTS
#define2 __ID ${LIGHT${INDEX}_ID}
uniform vec3 in_lightDiffuse${__ID};
uniform vec3 in_lightSpecular${__ID};
#ifdef LIGHT_IS_ATTENUATED${__ID}
uniform vec2 in_lightRadius${__ID};
#endif

#if LIGHT_TYPE${__ID} == SPOT
// spot light
uniform vec3 in_lightPosition${__ID};
uniform vec2 in_lightConeAngles${__ID};
uniform vec3 in_lightDirection${__ID};
#ifdef USE_SHADOW_MAP${__ID}
uniform float in_lightFar${__ID};
uniform float in_lightNear${__ID};
uniform mat4 in_lightMatrix${__ID};
uniform vec2 in_shadowInverseSize${__ID};
  #ifndef __TEX_shadowTexture${__ID}__
uniform sampler2DShadow in_shadowTexture${__ID};
  #endif
#endif // USE_SHADOW_MAP${__ID}
#endif // LIGHT_TYPE${__ID} == SPOT

#if LIGHT_TYPE${__ID} == POINT
// point light
uniform vec3 in_lightPosition${__ID};
#ifdef USE_SHADOW_MAP${__ID}
uniform float in_lightFar${__ID};
uniform float in_lightNear${__ID};
uniform vec2 in_shadowInverseSize${__ID};
uniform mat4 in_lightMatrix${__ID}[6];
  #ifndef __TEX_shadowTexture${__ID}__
uniform samplerCubeShadow in_shadowTexture${__ID};
  #endif
#endif // USE_SHADOW_MAP${__ID}
#endif // LIGHT_TYPE${__ID} == POINT

#if LIGHT_TYPE${__ID} == DIRECTIONAL
// directional light
uniform vec3 in_lightDirection${__ID};
#ifdef USE_SHADOW_MAP${__ID}
uniform vec2 in_shadowInverseSize${__ID};
uniform float in_lightFar${__ID}[ NUM_SHADOW_LAYER${__ID} ];
uniform mat4 in_lightMatrix${__ID}[ NUM_SHADOW_LAYER${__ID} ];
  #ifndef __TEX_shadowTexture${__ID}__
uniform sampler2DArrayShadow in_shadowTexture${__ID};
  #endif
#endif
#endif

#ifdef USE_SHADOW_MAP${__ID}
  #ifndef __TEX_shadowTexture${__ID}__
#define __TEX_shadowTexture${__ID}__
  #endif
#endif
#endfor
#endif // __light_inputs_included_

-- radiusAttenuation
#ifndef __radiusAttenuation_Included__
#define2 __radiusAttenuation_Included__
float radiusAttenuation(float d, float innerRadius, float outerRadius) {
    return 1.0 - smoothstep(innerRadius, outerRadius, d);
}
#endif

-- spotConeAttenuation
#ifndef __spotConeAttenuation_Included__
#define2 __spotConeAttenuation_Included__
float spotConeAttenuation(vec3 L, vec3 dir, vec2 coneAngles) {
    float spotEffect = dot( -L, normalize(dir) );
    float spotFade = 1.0 - (spotEffect-coneAngles.x)/(coneAngles.y-coneAngles.x);
    return max(0.0, sign(spotEffect-coneAngles.y))*clamp(spotFade, 0.0, 1.0);
}
#endif

-- specularFactor
#ifndef __specularFactor_Included__
#define2 __specularFactor_Included__
float specularFactor(vec3 P, vec3 L, vec3 N) {
    return max( dot(
            normalize( reflect( L, -N ) ),
            normalize( P - in_cameraPosition ) ), 0.0);
}
#endif
