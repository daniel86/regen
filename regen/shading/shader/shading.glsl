
-- fetchNormal
vec3 fetchNormal(vec2 texco) {
    vec4 N = texture(in_gNorWorldTexture, texco);
    if( N.w==0.0 ) discard;
    return N.xyz*2.0 - vec3(1.0);
}

-- fetchPosition
#include utility.texcoToWorldSpace

vec3 fetchPosition(vec2 texco) {
    float depth = texture(in_gDepthTexture, in_texco).r;
    return texcoToWorldSpace(in_texco, depth);
}

-- radiusAttenuation
float radiusAttenuation(float d, float innerRadius, float outerRadius) {
    return 1.0 - smoothstep(innerRadius, outerRadius, d);
}

-- spotConeAttenuation
float spotConeAttenuation(vec3 L, vec3 dir, vec2 coneAngles) {
    float spotEffect = dot( -L, normalize(dir) );
    float spotFade = 1.0 - (spotEffect-coneAngles.x)/(coneAngles.y-coneAngles.x);
    return max(0.0, sign(spotEffect-coneAngles.y))*clamp(spotFade, 0.0, 1.0);
}

-- specularFactor
float specularFactor(vec3 P, vec3 L, vec3 N) {
    return max( dot(
            normalize( reflect( -L, N ) ),
            normalize( P - in_cameraPosition ) ), 0.0);
}

--------------------------------------
--------------------------------------
---- Ambient Light Shading. Input mesh should be a unit-quad.
--------------------------------------
--------------------------------------
-- deferred.ambient.vs
#include utility.fullscreen.vs

-- deferred.ambient.fs
out vec4 out_color;
in vec2 in_texco;

uniform sampler2D in_gDepthTexture;
uniform sampler2D in_gNorWorldTexture;
uniform sampler2D in_gDiffuseTexture;

uniform vec3 in_lightAmbient;

#include shading.fetchNormal

void main() {
    vec3 N = fetchNormal(in_texco);
    vec4 diff = texture(in_gDiffuseTexture, in_texco);
    out_color.rgb = diff.rgb*in_lightAmbient;
    out_color.a = 0.0;
}

--------------------------------------
--------------------------------------
---- Deferred Directional Light Shading. Input mesh should be a unit-quad.
--------------------------------------
--------------------------------------
-- deferred.directional.vs
#include utility.fullscreen.vs

-- deferred.directional.fs
#extension GL_EXT_gpu_shader4 : enable

out vec4 out_color;
in vec2 in_texco;

uniform sampler2D in_gNorWorldTexture;
uniform sampler2D in_gDiffuseTexture;
uniform sampler2D in_gSpecularTexture;
uniform sampler2D in_gDepthTexture;

uniform vec3 in_cameraPosition;
uniform mat4 in_viewMatrix;
uniform mat4 in_inverseViewProjectionMatrix;

uniform vec3 in_lightDirection;
uniform vec3 in_lightSpecular;
#ifdef USE_SKY_COLOR
uniform samplerCube in_skyColorTexture;
#else
uniform vec3 in_lightDiffuse;
#endif

#ifdef USE_SHADOW_MAP
  #ifdef USE_SHADOW_SAMPLER
uniform sampler2DArrayShadow in_shadowTexture;
  #else
uniform sampler2DArray in_shadowTexture;
  #endif
uniform float in_shadowInverseSize;
uniform mat4 in_shadowMatrix[NUM_SHADOW_LAYER];
uniform float in_shadowFar[NUM_SHADOW_LAYER];
#endif

#include utility.texcoToWorldSpace
#include shading.fetchNormal
#include shading.specularFactor
#ifdef USE_SHADOW_MAP
#include shadow_mapping.sampling.dir
#endif

void main() {
    // fetch from GBuffer
    vec3 N = fetchNormal(in_texco);
    float depth = texture(in_gDepthTexture, in_texco).r;
    vec3 P = texcoToWorldSpace(in_texco, depth);
    vec4 spec = texture(in_gSpecularTexture, in_texco);
    vec4 diff = texture(in_gDiffuseTexture, in_texco);
    float shininess = spec.a*256.0; // map from [0,1] to [0,256]
    
    vec3 L = normalize(in_lightDirection);
    float nDotL = dot( N, L );
    if(nDotL<=0.0) discard;
    
#ifdef USE_SKY_COLOR
    vec3 lightDiffuse = texture(in_skyColorTexture, N ).rgb;
#else
    vec3 lightDiffuse = in_lightDiffuse;
#endif
    vec3 lightSpecular = in_lightSpecular;

#ifdef USE_SHADOW_MAP
    // find the texture layer
    int shadowLayer = 0;
    for(int i=0; i<NUM_SHADOW_LAYER; ++i) {
        if(depth<in_shadowFar[i]) {
            shadowLayer = i;
            break;
        }
    }
    // compute texture lookup coordinate
    vec4 shadowCoord = dirShadowCoord(shadowLayer, P, in_shadowMatrix[shadowLayer]);
    // compute filtered shadow
    float attenuation = dirShadow${SHADOW_MAP_FILTER}(in_shadowTexture, shadowCoord);
#else
    float attenuation = 1.0;
#endif
    
    // apply ambient and diffuse light
    out_color = vec4(
        diff.rgb*lightDiffuse*(attenuation*nDotL) +
        spec.rgb*lightSpecular*(attenuation*specularFactor(P,L,N)),
        0.0);
    
#ifdef USE_SHADOW_MAP
#ifdef DEBUG_SHADOW_SLICES
    vec3 color[8] = vec3[8](
        vec3(1.0, 0.4, 0.4),
        vec3(0.4, 1.0, 0.4),
        vec3(0.4, 0.4, 1.0),
        vec3(1.0, 1.0, 0.7),
        vec3(1.0, 0.7, 1.0),
        vec3(0.7, 1.0, 1.0),
        vec3(1.0, 1.0, 1.0),
        vec3(0.7, 0.7, 0.7));
    out_color.rgb *= color[shadowLayer];
#endif
#endif
}

--------------------------------------
---- Deferred Shading Fragment-Shader.
---- Can be used for point and spot lights.
---- Accumulate multiple lights using add blending.
--------------------------------------
-- deferred.fs
#extension GL_EXT_gpu_shader4 : enable

out vec4 out_color;

// G-buffer input
uniform sampler2D in_gNorWorldTexture;
uniform sampler2D in_gDiffuseTexture;
uniform sampler2D in_gSpecularTexture;
uniform sampler2D in_gDepthTexture;
// camera input
uniform vec3 in_cameraPosition;
uniform vec2 in_viewport;
uniform mat4 in_inverseViewProjectionMatrix;
// light input
uniform vec3 in_lightPosition;
uniform vec2 in_lightRadius;
#ifdef IS_SPOT_LIGHT
uniform vec3 in_lightDirection;
uniform vec2 in_lightConeAngles;
#endif
uniform vec3 in_lightDiffuse;
uniform vec3 in_lightSpecular;

#ifdef USE_SHADOW_MAP
// shadow input
uniform float in_shadowFar;
uniform float in_shadowNear;
uniform float in_shadowInverseSize;
#ifdef IS_SPOT_LIGHT
  #ifdef USE_SHADOW_SAMPLER
uniform sampler2DShadow in_shadowTexture;
  #else
uniform sampler2D in_shadowTexture;
  #endif
uniform mat4 in_shadowMatrix;
#else // !IS_SPOT_LIGHT
  #ifdef USE_SHADOW_SAMPLER
uniform samplerCubeShadow in_shadowTexture;
  #else
uniform samplerCube in_shadowTexture;
  #endif
uniform mat4 in_shadowMatrix[6];
#endif // !IS_SPOT_LIGHT
#endif // USE_SHADOW_MAP

#include utility.texcoToWorldSpace
#include utility.computeCubeLayer

#include shading.radiusAttenuation
#include shading.fetchNormal
#include shading.specularFactor
#ifdef IS_SPOT_LIGHT
  #include shading.spotConeAttenuation
#endif

#ifdef USE_SHADOW_MAP
  #ifdef IS_SPOT_LIGHT
    #include shadow_mapping.sampling.spot
  #else
    #include shadow_mapping.sampling.point
  #endif
#endif // USE_SHADOW_MAP

void main() {
    vec2 texco = gl_FragCoord.xy/in_viewport;
    // fetch from GBuffer
    vec3 N = fetchNormal(texco);
    float depth = texture(in_gDepthTexture, texco).r;
    vec3 P = texcoToWorldSpace(texco, depth);
    vec4 spec = texture(in_gSpecularTexture, texco);
    vec4 diff = texture(in_gDiffuseTexture, texco);
    float shininess = spec.a*256.0;

    vec3 lightVec = in_lightPosition - P;
    vec3 L = normalize(lightVec);
    float nDotL = dot( N, L );
    
    // calculate attenuation
    float attenuation = radiusAttenuation(
        length(lightVec), in_lightRadius.x, in_lightRadius.y);
#ifdef IS_SPOT_LIGHT
    attenuation *= spotConeAttenuation(L,in_lightDirection,in_lightConeAngles);
#endif
    // discard if facing away
    if(attenuation*nDotL<0.001) discard;

#ifdef USE_SHADOW_MAP
#ifdef IS_SPOT_LIGHT
    attenuation *= spotShadow${SHADOW_MAP_FILTER}(
        in_shadowTexture,
        in_shadowMatrix*vec4(P,1.0),
        lightVec,
        in_shadowNear,
        in_shadowFar);
#else
    float shadowDepth = (
        in_shadowMatrix[computeCubeLayer(lightVec)]*
        vec4(lightVec,1.0)).z;
    attenuation *= pointShadow${SHADOW_MAP_FILTER}(
        in_shadowTexture,
        lightVec,
        shadowDepth,
        in_shadowNear,
        in_shadowFar,
        in_shadowInverseSize);
#endif
#endif // USE_SHADOW_MAP
    
    // apply diffuse and specular light
    out_color = vec4(
        diff.rgb*in_lightDiffuse*(attenuation*nDotL) +
        spec.rgb*in_lightSpecular*(attenuation*specularFactor(P,L,N)),
        0.0);
}

--------------------------------------
--------------------------------------
---- Deferred Point Light Shading. Input mesh can be a cube or sphere.
--------------------------------------
--------------------------------------
-- deferred.point.vs
in vec3 in_pos;

uniform mat4 in_viewProjectionMatrix;
uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;

uniform vec2 in_lightRadius;
uniform vec3 in_lightPosition;

void main() {
    vec3 posWorld = in_lightPosition + in_pos*in_lightRadius.y;
    gl_Position = in_viewProjectionMatrix*vec4(posWorld,1.0);
}

-- deferred.point.fs
// #define IS_SPOT_LIGHT
#include shading.deferred.fs

--------------------------------------
--------------------------------------
---- Deferred Spot Light Shading. Input mesh is a cone.
--------------------------------------
--------------------------------------
-- deferred.spot.vs
in vec3 in_pos;
out vec3 out_intersection;

uniform mat4 in_viewProjectionMatrix;

uniform mat4 in_modelMatrix;
uniform vec3 in_lightPosition;
uniform vec2 in_lightRadius;
uniform vec2 in_lightConeAngles;

void main() {
    // TODO: cone model matrix should include scaling/rotation
    vec3 posWorld = in_pos;
    // scale height to base radius
    posWorld.z *= in_lightRadius.y;
    // find base radius based on cone angle
    // and scale the base with radius
    posWorld.xy *= 2.0*in_lightRadius.y*tan(acos(in_lightConeAngles.y));
    // rotate to light direction
    posWorld = (in_modelMatrix * vec4(posWorld,1.0)).xyz;
    // translate to light position
    posWorld.xyz += in_lightPosition;
    out_intersection = posWorld;
    gl_Position = in_viewProjectionMatrix*vec4(posWorld,1.0);
}

-- deferred.spot.fs
#define IS_SPOT_LIGHT
#include shading.deferred.fs

--------------------------------------
--------------------------------------
---- Direct Lighting
--------------------------------------
--------------------------------------
-- direct.inputs
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
-- direct.shade
struct Shading {
    vec3 diffuse;
    vec3 specular;
};

#include shading.direct.inputs
#include shading.spotConeAttenuation
#include shading.radiusAttenuation
#include shading.specularFactor
#include shadow_mapping.sampling.all
#include utility.computeCubeLayer

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
-- direct.diffuse
#include shading.direct.inputs
#include shading.spotConeAttenuation
#include shading.radiusAttenuation
#include shadow_mapping.sampling.all
#include utility.computeCubeLayer

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
