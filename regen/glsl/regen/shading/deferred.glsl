
-- fetchNormal
vec3 fetchNormal(sampler2D norWorldTex, vec2 texco) {
    vec4 N = texture(norWorldTex, texco);
    if( N.w==0.0 ) discard;
    return N.xyz*2.0 - vec3(1.0);
}
vec3 fetchNormal(sampler2DArray norWorldTex, vec3 texco) {
    vec4 N = texture(norWorldTex, texco);
    if( N.w==0.0 ) discard;
    return N.xyz*2.0 - vec3(1.0);
}
vec3 fetchNormal(samplerCube norWorldTex, vec3 texco) {
    vec4 N = texture(norWorldTex, texco);
    if( N.w==0.0 ) discard;
    return N.xyz*2.0 - vec3(1.0);
}

-- fetchPosition
#include regen.states.camera.transformTexcoToWorld

vec3 fetchPosition(vec2 texco) {
    float depth = __TEXTURE__(in_gDepthTexture, texco).r;
    return transformTexcoToWorld(texco, depth);
}


--------------------------------------
--------------------------------------
---- Ambient Light Shading. Input mesh should be a unit-quad.
--------------------------------------
--------------------------------------
-- ambient.vs
#include regen.filter.sampling.vs
-- ambient.gs
#include regen.filter.sampling.gs
-- ambient.fs
#include regen.states.camera.defines

out vec4 out_color;

uniform sampler2D in_gDepthTexture;
uniform sampler2D in_gNorWorldTexture;
uniform sampler2D in_gDiffuseTexture;

uniform vec3 in_lightAmbient;

#include regen.filter.sampling.computeTexco
#include regen.shading.deferred.fetchNormal

void main() {
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    vecTexco texco = computeTexco(texco_2D);
    
    vec3 N = fetchNormal(in_gNorWorldTexture,texco);
    vec4 diff = texture(in_gDiffuseTexture,texco);
    out_color.rgb = diff.rgb*in_lightAmbient;
    out_color.a = 0.0;
}

--------------------------------------
--------------------------------------
---- Deferred Directional Light Shading. Input mesh should be a unit-quad.
--------------------------------------
--------------------------------------
-- directional.vs
#define IS_DIRECTIONAL_LIGHT
#include regen.filter.sampling.vs
-- directional.gs
#define IS_DIRECTIONAL_LIGHT
#include regen.filter.sampling.gs
-- directional.fs
#define IS_DIRECTIONAL_LIGHT
#extension GL_EXT_gpu_shader4 : enable
#include regen.states.camera.defines

out vec4 out_color;

uniform sampler2D in_gNorWorldTexture;
uniform sampler2D in_gDiffuseTexture;
uniform sampler2D in_gSpecularTexture;
uniform sampler2D in_gDepthTexture;

#include regen.states.camera.input
#include regen.shading.light.input.deferred

#include regen.filter.sampling.computeTexco
#include regen.states.camera.transformTexcoToWorld
#include regen.shading.deferred.fetchNormal
#include regen.shading.light.specularFactor
#ifdef USE_SHADOW_MAP
#include regen.shading.shadow-mapping.sampling.dir
#endif

void main() {
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    vecTexco texco = computeTexco(texco_2D);
    
    // fetch from GBuffer
    vec3 N = fetchNormal(in_gNorWorldTexture,texco);
    float depth = texture(in_gDepthTexture, texco).r;
    vec3 P = transformTexcoToWorld(texco_2D, depth);
    vec4 spec = texture(in_gSpecularTexture, texco);
    vec4 diff = texture(in_gDiffuseTexture, texco);
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
        if(depth<in_lightFar[i]) {
            shadowLayer = i;
            break;
        }
    }
    // compute texture lookup coordinate
    vec4 shadowCoord = dirShadowCoord(shadowLayer, P, in_lightMatrix[shadowLayer]);
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
--------------------------------------
--------------------------------------
--------------------------------------

-- local.gs
#include regen.states.camera.defines
#if RENDER_LAYER > 1
#extension GL_EXT_geometry_shader4 : enable

layout(triangles) in;
// TODO: use ${RENDER_LAYER}*3
layout(triangle_strip, max_vertices=18) out;

flat out int out_layer;

uniform mat4 in_viewProjectionMatrix[${RENDER_LAYER}];

#define HANDLE_IO(i)

void emitVertex(vec4 posWorld, int index, int layer) {
  gl_Position = in_viewProjectionMatrix[layer]*posWorld;
  HANDLE_IO(index);
  EmitVertex();
}

void main() {
#for LAYER to ${RENDER_LAYER}
#ifndef SKIP_LAYER${LAYER}
  gl_Layer = ${LAYER};
  out_layer = ${LAYER};
  emitVertex(gl_PositionIn[0], 0, ${LAYER});
  emitVertex(gl_PositionIn[1], 1, ${LAYER});
  emitVertex(gl_PositionIn[2], 2, ${LAYER});
  EndPrimitive();
#endif
#endfor
}
#endif

-- local.fs
#extension GL_EXT_gpu_shader4 : enable
#include regen.states.camera.defines

out vec4 out_color;

// G-buffer input
uniform sampler2D in_gNorWorldTexture;
uniform sampler2D in_gDiffuseTexture;
uniform sampler2D in_gSpecularTexture;
uniform sampler2D in_gDepthTexture;
// camera input
#include regen.states.camera.input
// light input
#include regen.shading.light.input.deferred

#include regen.filter.sampling.computeTexco
#include regen.states.camera.transformTexcoToWorld
#include regen.math.computeCubeLayer

#include regen.shading.light.radiusAttenuation
#include regen.shading.deferred.fetchNormal
#include regen.shading.light.specularFactor
#ifdef IS_SPOT_LIGHT
  #include regen.shading.light.spotConeAttenuation
#endif

#ifdef USE_SHADOW_MAP
  #ifdef IS_SPOT_LIGHT
    #include regen.shading.shadow-mapping.sampling.spot
  #else
    #include regen.shading.shadow-mapping.sampling.point
  #endif
#endif // USE_SHADOW_MAP

#if RENDER_TARGET == CUBE
#include regen.math.computeCubeDirection
#endif

void main() {
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    vecTexco texco = computeTexco(texco_2D);
    
    // fetch from GBuffer
    vec3 N = fetchNormal(in_gNorWorldTexture,texco);
    float depth = texture(in_gDepthTexture, texco).r;
    vec3 P = transformTexcoToWorld(texco_2D, depth);
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
        in_lightMatrix*vec4(P,1.0),
        lightVec,
        in_lightNear,
        in_lightFar);
#else
    float shadowDepth = (
        in_lightMatrix[computeCubeLayer(lightVec)]*
        vec4(lightVec,1.0)).z;
    attenuation *= pointShadow${SHADOW_MAP_FILTER}(
        in_shadowTexture,
        lightVec,
        shadowDepth,
        in_lightNear,
        in_lightFar,
        in_shadowInverseSize.x);
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
-- point.vs
#include regen.states.camera.defines
in vec3 in_pos;

uniform vec2 in_lightRadius;
uniform vec3 in_lightPosition;

#if RENDER_LAYER == 1
#include regen.states.camera.input
#include regen.states.camera.transformWorldToScreen
#endif

void main() {
    vec3 posWorld = in_lightPosition + in_pos*in_lightRadius.y;
#if RENDER_LAYER > 1
    gl_Position = vec4(posWorld,1.0);
#else
    gl_Position = transformWorldToScreen(posWorld,0);
#endif
}

-- point.gs
#include regen.shading.deferred.local.gs
-- point.fs
// #define IS_SPOT_LIGHT
#include regen.shading.deferred.local.fs

--------------------------------------
--------------------------------------
---- Deferred Spot Light Shading. Input mesh is a cone.
--------------------------------------
--------------------------------------
-- spot.vs
#include regen.states.camera.defines

in vec3 in_pos;
out vec3 out_intersection;

uniform mat4 in_modelMatrix;

#if RENDER_LAYER == 1
#include regen.states.camera.input
#include regen.states.camera.transformWorldToScreen
#endif

void main() {
    out_intersection = (in_modelMatrix * vec4(in_pos,1.0)).xyz;
#if RENDER_LAYER > 1
    gl_Position = vec4(out_intersection,1.0);
#else
    gl_Position = transformWorldToScreen(out_intersection,0);
#endif
}
-- spot.gs
#include regen.shading.deferred.local.gs
-- spot.fs
#define IS_SPOT_LIGHT
#include regen.shading.deferred.local.fs
