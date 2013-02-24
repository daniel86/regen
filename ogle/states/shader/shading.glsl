
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

-- computAmbientOcclusion

#ifndef SIN_45
// 45 degrees = sin(PI / 4)
#define SIN_45 0.707107
#endif

const float in_aoSamplingRadius = 20.0;
const float in_aoBias = 0.05;
const vec2 in_aoAttenuation = vec2(1.0,5.0);

uniform sampler2D in_aoNoiseTexture;

float computeAO__(sampler2D depthTexture, vec2 texco, vec3 pos0, vec3 nor)
{
    vec3 pos1 = texcoToWorldSpace(texco, texture(depthTexture,texco).r);
    vec3 dir = pos1 - pos0;
    float dist = length(dir);
    // calculate occlusion intensity
    float i = max(0.0, dot(normalize(dir), nor) - in_aoBias);
    // distance attenuate intensity
    return i / (in_aoAttenuation.x + (in_aoAttenuation.y * dist));
}
float computAmbientOcclusion(sampler2D depthTexture, vec2 texco,
        vec3 P, vec3 N, float depth, vec2 texelSize)
{
	vec2 kernel[4] = vec2[](
        vec2( 1, 0), vec2(-1, 0),
        vec2( 0, 1), vec2( 0,-1)
    );
    vec2 kernelRadius = (in_aoSamplingRadius * (1.0 - depth)) * texelSize;

    vec2 randomVec = texture(in_aoNoiseTexture, texco).xy;
    randomVec = normalize(randomVec*2.0 - 1.0);
    
    float occlusion = 0.0;
    for (int i=0; i<4; ++i)
    {
        vec2 k = reflect(kernel[i], randomVec)*kernelRadius;
        occlusion += computeAO__(depthTexture, texco + k, P, N);
        occlusion += computeAO__(depthTexture, texco + k*0.5, P, N);

        k = vec2(k.x-k.y, k.x+k.y)*SIN_45;
        occlusion += computeAO__(depthTexture, texco + k*0.75, P, N);
        occlusion += computeAO__(depthTexture, texco + k*0.25, P, N);
    }
    return clamp(occlusion/16.0, 0.0, 1.0);
}

--------------------------------------
---- Ambient Light Shading.
----     Mesh  : Unit Quad
----     Input : GBuffer
----     Target: Color Texture
----     Blend : Add
--------------------------------------

-- deferred.ambient.vs
in vec3 in_pos;
out vec2 out_texco;
void main() {
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- deferred.ambient.fs
#extension GL_EXT_gpu_shader4 : enable

out vec4 output;
in vec2 in_texco;

uniform sampler2D in_gDepthTexture;
uniform sampler2D in_gNorWorldTexture;
uniform sampler2D in_gDiffuseTexture;

uniform float in_near;
uniform float in_far;
uniform mat4 in_inverseViewProjectionMatrix;

uniform vec3 in_lightAmbient;

#include shading.fetchNormal

#ifdef USE_AO
#include utility.texcoToWorldSpace
#include utility.linearizeDepth
#include shading.computAmbientOcclusion
#endif

void main() {
    vec3 N = fetchNormal(in_texco);
    vec4 diff = texture(in_gDiffuseTexture, in_texco);
    output.rgb = diff.rgb*in_lightAmbient;
    output.a = 0.0;
    
#ifdef USE_AO
    float depth = texture(in_gDepthTexture, in_texco).r;
    vec3 P = texcoToWorldSpace(in_texco, depth);
    depth = linearizeDepth(depth, in_near, in_far);
    // TODO: texel size uniform
    vec2 texelSize = 1.0/vec2(800.0,600.0);
    
    float occlusion = computAmbientOcclusion(
        in_gDepthTexture, in_texco, P, N, depth, texelSize);
    output.rgb *= 1.0-occlusion;
#endif
}

--------------------------------------
---- Directional Light Shading.
---- Shader is invoked once for each light.
----     Mesh  : Unit Quad
----     Input : GBuffer
----     Target: Color Texture
----     Blend : Add
--------------------------------------

-- deferred.directional.vs
in vec3 in_pos;
out vec2 out_texco;
void main() {
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- deferred.directional.fs
#extension GL_EXT_gpu_shader4 : enable

out vec4 output;
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
uniform mat4 in_shadowMatrices[NUM_SHADOW_LAYER];
uniform float in_shadowFar[NUM_SHADOW_LAYER];
#endif

#include utility.texcoToWorldSpace
#include shading.fetchNormal
#include shading.specularFactor
#ifdef USE_SHADOW_MAP
#include shadow_mapping.sampling.dir
#endif

#ifdef USE_SHADOW_MAP
int getShadowLayer(float depth) {
    // in_shadowFar is in camera homogeneous coordinates
    for(int i=0; i<NUM_SHADOW_LAYER; ++i) {
        if(depth<in_shadowFar[i]) return i;
    }
    return 0;
}
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
    // TODO: use irradiance environment map
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
    vec4 shadowCoord = dirShadowCoord(shadowLayer, P, in_shadowMatrices[shadowLayer]);
    // compute filtered shadow
    float attenuation = dirShadow${SHADOW_MAP_FILTER}(in_shadowTexture, shadowCoord);
#else
    float attenuation = 1.0;
#endif
    
    // apply ambient and diffuse light
    output = vec4(
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
    output.rgb *= color[shadowLayer];
#endif
#endif
}

--------------------------------------
--------------------------------------

-- light_sprite.fs
#extension GL_EXT_gpu_shader4 : enable

out vec4 output;

uniform sampler2D in_gNorWorldTexture;
uniform sampler2D in_gDiffuseTexture;
uniform sampler2D in_gSpecularTexture;
uniform sampler2D in_gDepthTexture;

uniform vec3 in_cameraPosition;
uniform vec2 in_viewport;
uniform mat4 in_inverseViewProjectionMatrix;

uniform vec3 in_lightPosition;
uniform vec2 in_lightRadius;
#ifdef IS_SPOT_LIGHT
uniform vec3 in_lightDirection;
uniform vec2 in_lightConeAngles;
#endif
uniform vec3 in_lightDiffuse;
uniform vec3 in_lightSpecular;

#ifdef USE_SHADOW_MAP
uniform float in_shadowFar;
uniform float in_shadowNear;
#endif // USE_SHADOW_MAP

#ifdef IS_SPOT_LIGHT
#ifdef USE_SHADOW_MAP
  #ifdef USE_SHADOW_SAMPLER
uniform sampler2DShadow in_shadowTexture;
  #else
uniform sampler2D in_shadowTexture;
  #endif
uniform mat4 in_shadowMatrix;
#endif // USE_SHADOW_MAP

#else // !IS_SPOT_LIGHT
#ifdef USE_SHADOW_MAP
  #ifdef USE_SHADOW_SAMPLER
uniform samplerCubeShadow in_shadowTexture;
  #else
uniform samplerCube in_shadowTexture;
  #endif
#endif // USE_SHADOW_MAP
#endif

#ifdef IS_SPOT_LIGHT
#include shading.spotConeAttenuation
#endif
#include shading.radiusAttenuation
#include shading.fetchNormal
#include shading.specularFactor
#include utility.texcoToWorldSpace

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

    // discard if facing away
    vec3 lightVec = in_lightPosition - P;
    vec3 L = normalize(lightVec);
    float nDotL = dot( N, L );
    if(nDotL<=0.0) discard;
    
    // calculate attenuation
    float attenuation = radiusAttenuation(
        length(lightVec), in_lightRadius.x, in_lightRadius.y);
#ifdef IS_SPOT_LIGHT
    attenuation *= spotConeAttenuation(L,in_lightDirection,in_lightConeAngles);
#endif
    if(attenuation<0.01) discard;

#ifdef USE_SHADOW_MAP
#ifdef IS_SPOT_LIGHT
    attenuation *= spotShadow${SHADOW_MAP_FILTER}(
        in_shadowTexture,
        in_shadowMatrix*vec4(P,1.0),
        lightVec,
        in_shadowNear,
        in_shadowFar);
#else
    attenuation *= pointShadow${SHADOW_MAP_FILTER}(
        in_shadowTexture,
        lightVec,
        in_shadowNear,
        in_shadowFar);
#endif
#endif // USE_SHADOW_MAP
    
    // apply diffuse and specular light
    output = vec4(
        diff.rgb*in_lightDiffuse*(attenuation*nDotL) +
        spec.rgb*in_lightSpecular*(attenuation*specularFactor(P,L,N)),
        0.0);
}

--------------------------------------
---- Point Light Shading.
---- Shader is invoked once for each light. Points are extruded to sprites in the GS.
----     Mesh  : Points
----     Input : GBuffer
----     Target: Color Texture
----     Blend : Add
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
#include shading.light_sprite.fs

--------------------------------------
---- Spot Light Shading.
---- Shader is invoked once for each light.
----     Mesh  : Two Sided Cone
----     Input : GBuffer
----     Target: Color Texture
----     Blend : Add
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
    // currently model matrix contains only rotation.
    // scalig/translation could be included aswell....
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
#include shading.light_sprite.fs

--------------------------------------
---- Combine Shaded Deferred Shaded Scene with TBuffer/AOBuffer.
----     Mesh  : Unit Quad
----     Input : TBuffer/AOBuffer
----     Target: Color Texture
----     Blend : Src
--------------------------------------

-- combine.vs
in vec3 in_pos;
out vec2 out_texco;
void main() {
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- combine.fs
out vec3 output;
in vec2 in_texco;

uniform sampler2D in_gColorTexture;
#ifdef USE_TRANSPARENCY
uniform sampler2D in_tColorTexture;
#endif
#ifdef USE_AMBIENT_OCCLUSION
uniform sampler2D in_aoTexture;
#endif
#ifdef USE_TRANSPARENCY_AMBIENT_OCCLUSION
uniform sampler2D in_taoTexture;
#endif

void main()
{
    output = texture(in_gColorTexture, in_texco).rgb;
#ifdef USE_AMBIENT_OCCLUSION
    {
        output *= 1.0-texture(in_aoTexture, in_texco).x;
    }
#endif
#ifdef USE_TRANSPARENCY
    {
        vec4 transp = texture(in_tColorTexture, in_texco);
#ifdef USE_TRANSPARENCY_AMBIENT_OCCLUSION
        transp.rgb *= 1.0-texture(in_taoTexture, in_texco).x;
#endif
        output = output*(1.0-transp.a) + transp.rgb*transp.a;
#endif
    }
}


-------------------------------
---- Direct Lighting
-------------------------------

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
uniform vec3 in_lightSpotDirection${__ID};
#ifdef LIGHT_HAS_SM${__ID}
uniform mat4 in_shadowMatrix${__ID};
  #ifndef __TEX_shadowTexture${__ID}__
  #ifdef USE_SHADOW_SAMPLER${__ID}
uniform sampler2DShadow in_shadowTexture;
  #else // USE_SHADOW_SAMPLER${__ID}
uniform sampler2D in_shadowTexture;
  #endif // USE_SHADOW_SAMPLER${__ID}
  #endif
#endif // LIGHT_HAS_SM${__ID}
#endif // LIGHT_TYPE${__ID} == SPOT

#if LIGHT_TYPE${__ID} == POINT
// point light
uniform vec3 in_lightPosition${__ID};
#ifdef LIGHT_HAS_SM${__ID}
uniform float in_shadowFar${__ID};
uniform float in_shadowNear${__ID};
  #ifndef __TEX_shadowTexture${__ID}__
  #ifdef USE_SHADOW_SAMPLER${__ID}
uniform samplerCubeShadow in_shadowTexture${__ID};
  #else // USE_SHADOW_SAMPLER${__ID}
uniform samplerCube in_shadowTexture${__ID};
  #endif // USE_SHADOW_SAMPLER${__ID}
  #endif
#endif // LIGHT_HAS_SM${__ID}
#endif // LIGHT_TYPE${__ID} == POINT

#if LIGHT_TYPE${__ID} == DIRECTIONAL
// directional light
uniform vec3 in_lightDirection${__ID};
#ifdef LIGHT_HAS_SM${__ID}
uniform float in_shadowFar${__ID}[NUM_SHADOW_MAP_SLICES];
uniform mat4 in_shadowMatrices${__ID}[NUM_SHADOW_MAP_SLICES];
  #ifndef __TEX_shadowTexture${__ID}__
  #ifdef USE_SHADOW_SAMPLER${__ID}
uniform sampler2DArrayShadow in_shadowTexture${__ID};
  #else // USE_SHADOW_SAMPLER${__ID}
uniform sampler2DArray in_shadowTexture${__ID};
  #endif // USE_SHADOW_SAMPLER${__ID}
  #endif
#endif
#endif

#ifdef LIGHT_HAS_SM${__ID}
  #ifndef __TEX_shadowTexture${__ID}__
#define __TEX_shadowTexture${__ID}__
  #endif
#endif

#endfor
#endif

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
            in_lightSpotDirection${__ID}, in_lightConeAngles${__ID});
#endif
#ifdef LIGHT_HAS_SM${__ID}
        // calculate shadow attenuation
  #if LIGHT_TYPE${__ID} == DIRECTIONAL
        // find the texture layer
        int shadowLayer = 0;
        for(int i=0; i<NUM_SHADOW_LAYER; ++i) {
            if(depth<in_shadowFar${__ID}[i]) {
                shadowLayer = i;
                break;
            }
        }
        // compute texture lookup coordinate
        vec4 shadowCoord = dirShadowCoord(shadowLayer, P,
            in_shadowMatrices${__ID}[shadowLayer]);
        // compute filtered shadow
        attenuation *= dirShadow${SHADOW_MAP_FILTER}(in_shadowTexture, shadowCoord);
  #endif
  #if LIGHT_TYPE${__ID} == POINT
        attenuation *= pointShadow${LIGHT_SM_FILTER${__ID}}(
            in_shadowTexture${__ID},
            lightVec,
            in_shadowNear${__ID},
            in_shadowFar${__ID});
  #endif
  #if LIGHT_TYPE${__ID} == SPOT
        attenuation *= spotShadow${LIGHT_SM_FILTER${__ID}}(
            in_shadowTexture${__ID},
            in_shadowMatrix${__ID}*vec4(P,1.0),
            lightVec,
            in_shadowNear${__ID},
            in_shadowFar${__ID});
  #endif
#endif
        // calculate diffuse light
        s.diffuse += in_lightDiffuse${__ID} * (attenuation * nDotL);
        s.specular += in_lightSpecular${__ID} * (attenuation * specularFactor(P,L,N));
    }

#endfor

  return s;
}

-- direct.diffuse
#include shading.direct.inputs
#include shading.spotConeAttenuation
#include shading.radiusAttenuation
#include shadow_mapping.sampling.all

vec3 getDiffuseLight(vec3 P, float depth)
{
    vec3 diff = vec3(0.0);
    float attenuation;

#for INDEX to NUM_LIGHTS
#define2 __ID ${LIGHT${INDEX}_ID}
    {
    // LIGHT ${INDEX}
#ifdef LIGHT_IS_ATTENUATED${__ID}
    attenuation = radiusAttenuation(
        length(P - in_lightPosition${__ID}),
        in_lightRadius${__ID}.x, in_lightRadius${__ID}.y);
#else
    attenuation = 1.0;
#endif

#if LIGHT_TYPE${__ID} != DIRECTIONAL
        vec3 lightVec = in_lightPosition${__ID} - P;
#endif

#if LIGHT_TYPE${__ID} == SPOT
        attenuation *= spotConeAttenuation(
            normalize(lightVec),
            in_lightSpotDirection${__ID},
            in_lightConeAngles${__ID});
#endif

#ifdef IS_SHADOW_RECEIVER
#ifdef LIGHT_HAS_SM${__ID}
  #if LIGHT_TYPE${__ID} == DIRECTIONAL
        // find the texture layer
        int shadowLayer = 0;
        for(int i=0; i<NUM_SHADOW_LAYER; ++i) {
            if(depth<in_shadowFar${__ID}[i]) {
                shadowLayer = i;
                break;
            }
        }
        // compute texture lookup coordinate
        vec4 shadowCoord = dirShadowCoord(shadowLayer, P,
            in_shadowMatrices${__ID}[shadowLayer]);
        // compute filtered shadow
        attenuation *= dirShadow${SHADOW_MAP_FILTER}(in_shadowTexture, shadowCoord);
  #endif
  #if LIGHT_TYPE${__ID} == POINT
        attenuation *= pointShadow${LIGHT_SM_FILTER${__ID}}(
                in_shadowTexture${__ID},
                lightVec,
                in_shadowNear${__ID},
                in_shadowFar${__ID});
  #endif
  #if LIGHT_TYPE${__ID} == SPOT
        attenuation *= spotShadow${LIGHT_SM_FILTER${__ID}}(
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


