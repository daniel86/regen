
-- defines
#ifndef SHADING_MODEL
#define SHADING_MODEL PHONG
#endif
#ifdef IS_POINT_LIGHT
    #ifndef POINT_LIGHT_TYPE
#define POINT_LIGHT_TYPE CUBE
    #endif
#endif

-- input.deferred
#ifndef REGEN_light_inputs_included_
#define2 REGEN_light_inputs_included_
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
#ifdef USE_SHADOW_COLOR
uniform sampler2D in_shadowColorTexture;
#endif
uniform mat4 in_lightMatrix;
#endif // USE_SHADOW_MAP
#endif

#ifdef IS_POINT_LIGHT
    #if POINT_LIGHT_TYPE == CUBE
uniform vec3 in_lightPosition;
uniform vec2 in_lightRadius;
        #ifdef USE_SHADOW_MAP
uniform float in_lightFar;
uniform float in_lightNear;
uniform vec2 in_shadowInverseSize;
uniform samplerCubeShadow in_shadowTexture;
            #ifdef USE_SHADOW_COLOR
uniform samplerCube in_shadowColorTexture;
            #endif
uniform mat4 in_lightMatrix[6];
        #endif // USE_SHADOW_MAP
    #endif // POINT_LIGHT_TYPE == CUBE

    #if POINT_LIGHT_TYPE == PARABOLIC
uniform vec3 in_lightDirection;
uniform mat4 in_lightMatrix[NUM_SHADOW_LAYER];
uniform vec3 in_lightPosition;
uniform vec2 in_lightRadius;
        #ifdef USE_SHADOW_MAP
uniform float in_lightFar;
uniform float in_lightNear;
uniform vec2 in_shadowInverseSize;
uniform sampler2DArrayShadow in_shadowTexture;
            #ifdef USE_SHADOW_COLOR
uniform sampler2DArray in_shadowColorTexture;
            #endif // USE_SHADOW_COLOR
        #endif // USE_SHADOW_MAP
    #endif // POINT_LIGHT_TYPE == PARABOLIC
#endif

#ifdef IS_DIRECTIONAL_LIGHT
uniform vec3 in_lightDirection;
#ifdef USE_SKY_COLOR
uniform samplerCube in_skyColorTexture;
#endif
#ifdef USE_SHADOW_MAP
uniform sampler2DArrayShadow in_shadowTexture;
#ifdef USE_SHADOW_COLOR
uniform sampler2DArray in_shadowColorTexture;
#endif
uniform vec2 in_shadowInverseSize;
uniform mat4 in_lightMatrix[NUM_SHADOW_LAYER];
uniform float in_lightFar[NUM_SHADOW_LAYER];
#endif
#endif
#endif // REGEN_light_inputs_included_

-- input.direct
#ifndef REGEN_light_inputs_included_
#define2 REGEN_light_inputs_included_
#for INDEX to NUM_LIGHTS
#define2 REGEN_ID ${LIGHT${INDEX}_ID}
uniform vec3 in_lightDiffuse${REGEN_ID};
uniform vec3 in_lightSpecular${REGEN_ID};
#ifdef LIGHT_IS_ATTENUATED${REGEN_ID}
uniform vec2 in_lightRadius${REGEN_ID};
#endif

#if LIGHT_TYPE${REGEN_ID} == SPOT
// spot light
uniform vec3 in_lightPosition${REGEN_ID};
uniform vec2 in_lightConeAngles${REGEN_ID};
uniform vec3 in_lightDirection${REGEN_ID};
#ifdef USE_SHADOW_MAP${REGEN_ID}
uniform float in_lightFar${REGEN_ID};
uniform float in_lightNear${REGEN_ID};
uniform mat4 in_lightMatrix${REGEN_ID};
uniform vec2 in_shadowInverseSize${REGEN_ID};
uniform sampler2DShadow in_shadowTexture${REGEN_ID};
#ifdef USE_SHADOW_COLOR
uniform sampler2D in_shadowColorTexture;
#endif
#endif // USE_SHADOW_MAP${REGEN_ID}
#endif // LIGHT_TYPE${REGEN_ID} == SPOT

#if LIGHT_TYPE${REGEN_ID} == POINT
    #if POINT_LIGHT_TYPE${REGEN_ID} == CUBE
// point light
uniform vec3 in_lightPosition${REGEN_ID};
        #ifdef USE_SHADOW_MAP${REGEN_ID}
uniform float in_lightFar${REGEN_ID};
uniform float in_lightNear${REGEN_ID};
uniform vec2 in_shadowInverseSize${REGEN_ID};
uniform mat4 in_lightMatrix${REGEN_ID}[6];
uniform samplerCubeShadow in_shadowTexture${REGEN_ID};
            #ifdef USE_SHADOW_COLOR
uniform samplerCube in_shadowColorTexture;
            #endif
        #endif // USE_SHADOW_MAP${REGEN_ID}
    #endif // POINT_LIGHT_TYPE${REGEN_ID} == CUBE

    #if POINT_LIGHT_TYPE${REGEN_ID} == PARABOLIC
// point light
uniform vec3 in_lightPosition${REGEN_ID};
        #ifdef USE_SHADOW_MAP${REGEN_ID}
uniform float in_lightFar${REGEN_ID};
uniform float in_lightNear${REGEN_ID};
uniform vec2 in_shadowInverseSize${REGEN_ID};
uniform mat4 in_lightMatrix${REGEN_ID}[ NUM_SHADOW_LAYER${REGEN_ID} ];
uniform vec3 in_lightDirection${REGEN_ID}[ NUM_SHADOW_LAYER${REGEN_ID} ];
uniform sampler2DArrayShadow in_shadowTexture${REGEN_ID};
            #ifdef USE_SHADOW_COLOR
uniform sampler2DArray in_shadowColorTexture;
            #endif
        #endif // USE_SHADOW_MAP${REGEN_ID}
    #endif // LIGHT_TYPE${REGEN_ID} == PARABOLIC
#endif // LIGHT_TYPE${REGEN_ID} == POINT

#if LIGHT_TYPE${REGEN_ID} == DIRECTIONAL
// directional light
uniform vec3 in_lightDirection${REGEN_ID};
#ifdef USE_SHADOW_MAP${REGEN_ID}
uniform vec2 in_shadowInverseSize${REGEN_ID};
uniform float in_lightFar${REGEN_ID}[ NUM_SHADOW_LAYER${REGEN_ID} ];
uniform mat4 in_lightMatrix${REGEN_ID}[ NUM_SHADOW_LAYER${REGEN_ID} ];
uniform sampler2DArrayShadow in_shadowTexture${REGEN_ID};
#ifdef USE_SHADOW_COLOR
uniform sampler2DArray in_shadowColorTexture;
#endif
#endif
#endif

#ifdef USE_SHADOW_MAP${REGEN_ID}
  #ifndef REGEN_TEX_shadowTexture${REGEN_ID}_
#define REGEN_TEX_shadowTexture${REGEN_ID}_
  #endif
#endif
#endfor
#endif // REGEN_light_inputs_included_

-- radiusAttenuation
#ifndef REGEN_radiusAttenuation_Included_
#define2 REGEN_radiusAttenuation_Included_
float radiusAttenuation(float d, float innerRadius, float outerRadius) {
    return 1.0 - smoothstep(innerRadius, outerRadius, d);
}
#endif

-- spotConeAttenuation
#ifndef REGEN_spotConeAttenuation_Included_
#define2 REGEN_spotConeAttenuation_Included_
float spotConeAttenuation(vec3 L, vec3 dir, vec2 coneAngles) {
    float spotEffect = dot( -L, normalize(dir) );
    float spotFade = 1.0 - (spotEffect-coneAngles.x)/(coneAngles.y-coneAngles.x);
    return max(0.0, sign(spotEffect-coneAngles.y))*clamp(spotFade, 0.0, 1.0);
}
#endif

-- specularFactor
#ifndef REGEN_specularFactor_Included_
#define2 REGEN_specularFactor_Included_
float specularFactor(vec3 P, vec3 L, vec3 N) {
    return max(
            dot( reflect( L, -N ) ,
            normalize( P - in_cameraPosition ) ), 0.0);
}
#endif
