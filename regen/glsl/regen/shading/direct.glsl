
--------------------------------------
--------------------------------------
---- Direct Lighting
--------------------------------------
--------------------------------------
-- inputs
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
uniform float in_shadowFar${__ID};
uniform float in_shadowNear${__ID};
uniform mat4 in_shadowMatrix${__ID};
uniform float in_shadowInverseSize${__ID};
  #ifndef __TEX_shadowTexture${__ID}__
  #ifdef USE_SHADOW_SAMPLER${__ID}
uniform sampler2DShadow in_shadowTexture${__ID};
  #else // USE_SHADOW_SAMPLER${__ID}
uniform sampler2D in_shadowTexture${__ID};
  #endif // USE_SHADOW_SAMPLER${__ID}
  #endif
#endif // USE_SHADOW_MAP${__ID}
#endif // LIGHT_TYPE${__ID} == SPOT

#if LIGHT_TYPE${__ID} == POINT
// point light
uniform vec3 in_lightPosition${__ID};
#ifdef USE_SHADOW_MAP${__ID}
uniform float in_shadowFar${__ID};
uniform float in_shadowNear${__ID};
uniform float in_shadowInverseSize${__ID};
uniform mat4 in_shadowMatrix${__ID}[6];
  #ifndef __TEX_shadowTexture${__ID}__
  #ifdef USE_SHADOW_SAMPLER${__ID}
uniform samplerCubeShadow in_shadowTexture${__ID};
  #else // USE_SHADOW_SAMPLER${__ID}
uniform samplerCube in_shadowTexture${__ID};
  #endif // USE_SHADOW_SAMPLER${__ID}
  #endif
#endif // USE_SHADOW_MAP${__ID}
#endif // LIGHT_TYPE${__ID} == POINT

#if LIGHT_TYPE${__ID} == DIRECTIONAL
// directional light
uniform vec3 in_lightDirection${__ID};
#ifdef USE_SHADOW_MAP${__ID}
uniform float in_shadowInverseSize${__ID};
uniform float in_shadowFar${__ID}[ NUM_SHADOW_LAYER${__ID} ];
uniform mat4 in_shadowMatrix${__ID}[ NUM_SHADOW_LAYER${__ID} ];
  #ifndef __TEX_shadowTexture${__ID}__
  #ifdef USE_SHADOW_SAMPLER${__ID}
uniform sampler2DArrayShadow in_shadowTexture${__ID};
  #else // USE_SHADOW_SAMPLER${__ID}
uniform sampler2DArray in_shadowTexture${__ID};
  #endif // USE_SHADOW_SAMPLER${__ID}
  #endif
#endif
#endif

#ifdef USE_SHADOW_MAP${__ID}
  #ifndef __TEX_shadowTexture${__ID}__
#define __TEX_shadowTexture${__ID}__
  #endif
#endif

#endfor
#endif

--------------------------------------
------ Shades meshes directly.
------ Number of lights is limited by maximum number of uniforms.
------ Multiple lights are handled using the for directive.
--------------------------------------
-- shade
struct Shading {
    vec3 diffuse;
    vec3 specular;
};

#include regen.shading.direct.inputs
#include regen.shading.utility.spotConeAttenuation
#include regen.shading.utility.radiusAttenuation
#include regen.shading.utility.specularFactor
#include regen.shading.shadow-mapping.sampling.all
#include regen.utiltiy.utility.computeCubeLayer

Shading shade(vec3 P, vec3 N, float depth, float shininess)
{
  // accumulates lighting
  Shading s;
  s.diffuse = vec3(0.0);
  s.specular = vec3(0.0);
  vec3 L, lightVec;
  float attenuation;
  float nDotL;

#for INDEX to NUM_LIGHTS
#define2 __ID ${LIGHT${INDEX}_ID}
#if LIGHT_TYPE${__ID} == DIRECTIONAL
    L = normalize(in_lightDirection${__ID});
#else
    lightVec = in_lightPosition${__ID} - P;
    L = normalize(lightVec);
#endif
    nDotL = dot( N, L );
    if(nDotL > 0.0) {
#ifdef LIGHT_IS_ATTENUATED${__ID}
        // calculate attenuation based on distance
        attenuation = radiusAttenuation(
            length(lightVec), in_lightRadius${__ID}.x, in_lightRadius${__ID}.y);
#else
        attenuation = 1.0;
#endif
#if LIGHT_TYPE${__ID} == SPOT
        // calculate attenuation based on cone angles
        attenuation *= spotConeAttenuation(L,
            in_lightDirection${__ID}, in_lightConeAngles${__ID});
#endif
#ifdef USE_SHADOW_MAP${__ID}
        // calculate shadow attenuation
  #if LIGHT_TYPE${__ID} == DIRECTIONAL
        // find the texture layer
        int shadowLayer = 0;
        for(int i=0; i<NUM_SHADOW_LAYER${__ID}; ++i) {
            if(depth<in_shadowFar${__ID}[i]) {
                shadowLayer = i;
                break;
            }
        }
        // compute texture lookup coordinate
        vec4 shadowCoord = dirShadowCoord(shadowLayer, P,
            in_shadowMatrix${__ID}[shadowLayer]);
        // compute filtered shadow
        attenuation *= dirShadow${SHADOW_MAP_FILTER}(in_shadowTexture, shadowCoord);
  #endif
  #if LIGHT_TYPE${__ID} == POINT
        float shadowDepth = (
            in_shadowMatrix${__ID}[computeCubeLayer(lightVec)]*
            vec4(lightVec,1.0)).z;
        attenuation *= pointShadow${SHADOW_MAP_FILTER${__ID}}(
            in_shadowTexture${__ID},
            lightVec,
            shadowDepth,
            in_shadowNear${__ID},
            in_shadowFar${__ID},
            in_shadowInverseSize${__ID});
  #endif
  #if LIGHT_TYPE${__ID} == SPOT
        attenuation *= spotShadow${SHADOW_MAP_FILTER${__ID}}(
            in_shadowTexture${__ID},
            in_shadowMatrix${__ID}*vec4(P,1.0),
            lightVec,
            in_shadowNear${__ID},
            in_shadowFar${__ID});
  #endif
#endif
        s.diffuse += in_lightDiffuse${__ID} * (attenuation * nDotL);
        s.specular += in_lightSpecular${__ID} * (attenuation * specularFactor(P,L,N));
    }

#endfor

  return s;
}

--------------------------------------
------ Shades meshes directly.
------ Number of lights is limited by maximum number of uniforms.
------ Multiple lights are handled using the for directive.
------ Specular light is ignored.
--------------------------------------
-- diffuse
#include regen.shading.direct.inputs
#include regen.shading.utility.spotConeAttenuation
#include regen.shading.utility.radiusAttenuation
#include regen.shading.shadow-mapping.sampling.all
#include regen.utiltiy.utility.computeCubeLayer

vec3 getDiffuseLight(vec3 P, float depth)
{
    vec3 diff = vec3(0.0);
#for INDEX to NUM_LIGHTS
#define2 __ID ${LIGHT${INDEX}_ID}
    {
    // LIGHT${INDEX}
#ifdef LIGHT_IS_ATTENUATED${__ID}
        float attenuation = radiusAttenuation(
            length(P - in_lightPosition${__ID}),
            in_lightRadius${__ID}.x, in_lightRadius${__ID}.y);
#else
        float attenuation = 1.0;
#endif
#if LIGHT_TYPE${__ID} != DIRECTIONAL
        vec3 lightVec = in_lightPosition${__ID} - P;
#endif
#if LIGHT_TYPE${__ID} == SPOT
        attenuation *= spotConeAttenuation(
            normalize(lightVec),
            in_lightDirection${__ID},
            in_lightConeAngles${__ID});
#endif
#ifdef IS_SHADOW_RECEIVER
#ifdef USE_SHADOW_MAP${__ID}
  #if LIGHT_TYPE${__ID} == DIRECTIONAL
        // find the texture layer
        int shadowLayer = 0;
        for(int i=0; i<NUM_SHADOW_LAYER${__ID}; ++i) {
            if(depth<in_shadowFar${__ID}[i]) {
                shadowLayer = i;
                break;
            }
        }
        // compute texture lookup coordinate
        vec4 shadowCoord = dirShadowCoord(shadowLayer, P,
            in_shadowMatrix${__ID}[shadowLayer]);
        // compute filtered shadow
        attenuation *= dirShadow${SHADOW_MAP_FILTER}(in_shadowTexture, shadowCoord);
  #endif
  #if LIGHT_TYPE${__ID} == POINT
        float shadowDepth = (
            in_shadowMatrix${__ID}[computeCubeLayer(lightVec)]*
            vec4(lightVec,1.0)).z;
        attenuation *= pointShadow${SHADOW_MAP_FILTER${__ID}}(
                in_shadowTexture${__ID},
                lightVec,
                shadowDepth,
                in_shadowNear${__ID},
                in_shadowFar${__ID},
                in_shadowInverseSize${__ID});
  #endif
  #if LIGHT_TYPE${__ID} == SPOT
        attenuation *= spotShadow${SHADOW_MAP_FILTER${__ID}}(
                in_shadowTexture${__ID},
                in_shadowMatrix${__ID}*vec4(P,1.0),
                lightVec,
                in_shadowNear${__ID},
                in_shadowFar${__ID});
  #endif
#endif
#endif
        diff += in_lightDiffuse${__ID} * attenuation;
    }
#endfor
    
    return diff;
}
