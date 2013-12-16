
-- defines
#if RENDER_TARGET == CUBE
#include regen.math.computeCubeDirection
// TODO: compute only once
#define __CUBE_UV__(uv) 
#define __TEXTURE__(_tex_,uv) texture(_tex_,computeCubeDirection(vec2(2,-2)*uv + vec2(-1,1),in_layer))

#elif RENDER_LAYER > 1
#define __TEXTURE__(x,y) texture(x,vec3(y,in_layer))

#else
#define __TEXTURE__(x,y) texture(x,y)

#endif

--------------------------------------
--------------------------------------
---- Ambient Light Shading. Input mesh should be a unit-quad.
--------------------------------------
--------------------------------------
-- ambient.vs
#include regen.post-passes.fullscreen.vs
-- ambient.gs
#include regen.post-passes.fullscreen.gs
-- ambient.fs
#include regen.shading.deferred.defines
out vec4 out_color;
in vec2 in_texco;
#if RENDER_LAYER > 1
flat in int in_layer;
#endif

uniform sampler2D in_gDepthTexture;
uniform sampler2D in_gNorWorldTexture;
uniform sampler2D in_gDiffuseTexture;

uniform vec3 in_lightAmbient;

#include regen.shading.utility.fetchNormal

void main() {
    vec3 N = fetchNormal(in_texco);
    vec4 diff = __TEXTURE__(in_gDiffuseTexture, in_texco);
    out_color.rgb = diff.rgb*in_lightAmbient;
    out_color.a = 0.0;
}

--------------------------------------
--------------------------------------
---- Deferred Directional Light Shading. Input mesh should be a unit-quad.
--------------------------------------
--------------------------------------
-- directional.vs
#include regen.post-passes.fullscreen.vs
-- directional.gs
#include regen.post-passes.fullscreen.gs
-- directional.fs
#extension GL_EXT_gpu_shader4 : enable
#include regen.shading.deferred.defines

out vec4 out_color;
in vec2 in_texco;
#if RENDER_LAYER > 1
flat in int in_layer;
#endif

uniform sampler2D in_gNorWorldTexture;
uniform sampler2D in_gDiffuseTexture;
uniform sampler2D in_gSpecularTexture;
uniform sampler2D in_gDepthTexture;

#include regen.states.camera.input

uniform vec3 in_lightDirection;
uniform vec3 in_lightSpecular;
#ifdef USE_SKY_COLOR
uniform samplerCube in_skyColorTexture;
#else
uniform vec3 in_lightDiffuse;
#endif

#ifdef USE_SHADOW_MAP
uniform sampler2DArrayShadow in_shadowTexture;
uniform vec2 in_shadowInverseSize;
uniform mat4 in_lightMatrix[NUM_SHADOW_LAYER];
uniform float in_lightFar[NUM_SHADOW_LAYER];
#endif

#include regen.states.camera.transformTexcoToWorld
#include regen.shading.utility.fetchNormal
#include regen.shading.utility.specularFactor
#ifdef USE_SHADOW_MAP
#include regen.shading.shadow-mapping.sampling.dir
#endif

void main() {
    // fetch from GBuffer
    vec3 N = fetchNormal(in_texco);
    float depth = __TEXTURE__(in_gDepthTexture, in_texco).r;
    vec3 P = transformTexcoToWorld(in_texco, depth);
    vec4 spec = __TEXTURE__(in_gSpecularTexture, in_texco);
    vec4 diff = __TEXTURE__(in_gDiffuseTexture, in_texco);
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
---- Deferred Shading Fragment-Shader.
---- Can be used for point and spot lights.
---- Accumulate multiple lights using add blending.
--------------------------------------
-- fs
#extension GL_EXT_gpu_shader4 : enable
#include regen.shading.deferred.defines

out vec4 out_color;
#if RENDER_LAYER > 1
flat in int in_layer;
#endif

// G-buffer input
uniform sampler2D in_gNorWorldTexture;
uniform sampler2D in_gDiffuseTexture;
uniform sampler2D in_gSpecularTexture;
uniform sampler2D in_gDepthTexture;
// camera input
#include regen.states.camera.input
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
uniform float in_lightFar;
uniform float in_lightNear;
uniform vec2 in_shadowInverseSize;
#ifdef IS_SPOT_LIGHT
uniform sampler2DShadow in_shadowTexture;
uniform mat4 in_lightMatrix;
#else // !IS_SPOT_LIGHT
uniform samplerCubeShadow in_shadowTexture;
uniform mat4 in_lightMatrix[6];
#endif // !IS_SPOT_LIGHT
#endif // USE_SHADOW_MAP

#include regen.states.camera.transformTexcoToWorld
#include regen.math.computeCubeLayer

#include regen.shading.utility.radiusAttenuation
#include regen.shading.utility.fetchNormal
#include regen.shading.utility.specularFactor
#ifdef IS_SPOT_LIGHT
  #include regen.shading.utility.spotConeAttenuation
#endif

#ifdef USE_SHADOW_MAP
  #ifdef IS_SPOT_LIGHT
    #include regen.shading.shadow-mapping.sampling.spot
  #else
    #include regen.shading.shadow-mapping.sampling.point
  #endif
#endif // USE_SHADOW_MAP

void main() {
    // TODO: not working with cube map render target
    vec2 texco = gl_FragCoord.xy/in_viewport;
      
    // fetch from GBuffer
    vec3 N = fetchNormal(texco);
    float depth = __TEXTURE__(in_gDepthTexture, texco).r;
    vec3 P = transformTexcoToWorld(texco, depth);
    vec4 spec = __TEXTURE__(in_gSpecularTexture, texco);
    vec4 diff = __TEXTURE__(in_gDiffuseTexture, texco);
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

-- gs
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

--------------------------------------
--------------------------------------
---- Deferred Point Light Shading. Input mesh can be a cube or sphere.
--------------------------------------
--------------------------------------
-- point.vs
in vec3 in_pos;

uniform vec2 in_lightRadius;
uniform vec3 in_lightPosition;

#if RENDER_LAYER > 1
void main() {
    vec3 posWorld = in_lightPosition + in_pos*in_lightRadius.y;
    gl_Position = vec4(posWorld,1.0);
}
#else
#include regen.states.camera.input
#include regen.states.camera.transformWorldToScreen

void main() {
    vec3 posWorld = in_lightPosition + in_pos*in_lightRadius.y;
    gl_Position = transformWorldToScreen(posWorld,0);
}
#endif

-- spot.gs
#include regen.shading.deferred.gs
-- point.fs
// #define IS_SPOT_LIGHT
#include regen.shading.deferred.fs

--------------------------------------
--------------------------------------
---- Deferred Spot Light Shading. Input mesh is a cone.
--------------------------------------
--------------------------------------
-- spot.vs
in vec3 in_pos;
out vec3 out_intersection;

uniform mat4 in_modelMatrix;

#if RENDER_LAYER > 1
void main() {
    gl_Position = in_modelMatrix * vec4(in_pos,1.0);
    out_intersection = gl_Position.xyz;
}
#else
#include regen.states.camera.input
#include regen.states.camera.transformWorldToScreen

void main() {
    out_intersection = (in_modelMatrix * vec4(in_pos,1.0)).xyz;
    gl_Position = transformWorldToScreen(out_intersection,0);
}
#endif

-- spot.gs
#include regen.shading.deferred.gs
-- spot.fs
#define IS_SPOT_LIGHT
#include regen.shading.deferred.fs
