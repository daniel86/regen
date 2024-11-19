
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
    return transformTexcoToWorld(texco, depth, in_layer);
}

-- phong
#ifndef REGEN_phong_Included_
#define2 REGEN_phong_Included_
vec3 phong(vec3 diff, vec3 spec, float nDotL, float sf, float shininess) {
    return diff*nDotL + spec*pow(sf, shininess);
}
#endif

-- toon
#ifndef REGEN_toon_Included_
#define2 REGEN_toon_Included_

const float in_toonThreshold0 = 0.05;
const float in_toonThreshold1 = 0.3;
const float in_toonThreshold2 = 0.6;
const float in_toonThreshold3 = 0.9;

vec3 toon(vec3 diff, vec3 spec, float nDotL, float sf, float shininess) {
    float df = nDotL;
    if (df < in_toonThreshold0) df = 0.0;
    else if (df < in_toonThreshold1) df = in_toonThreshold1;
#ifdef HAS_toonThreshold2
    else if (df < in_toonThreshold2) df = in_toonThreshold2;
#endif
#ifdef HAS_toonThreshold3
    else if (df < in_toonThreshold3) df = in_toonThreshold3;
#endif
    else df = 1.0;

    return df*diff +
        step(0.5, pow(sf, shininess))*spec;
}
#endif

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
---- Material Emission Light. Input mesh should be a unit-quad.
--------------------------------------
--------------------------------------
-- emission.vs
#include regen.filter.sampling.vs
-- emission.gs
#include regen.filter.sampling.gs
-- emission.fs
#include regen.states.camera.defines

out vec4 out_color;

uniform sampler2D in_gEmissionTexture;
#ifdef HAS_gEmissionBlurred
uniform sampler2D in_gEmissionBlurred;
#endif

#include regen.filter.sampling.computeTexco

void main() {
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    vecTexco texco = computeTexco(texco_2D);
    vec3 col1 = texture(in_gEmissionTexture,texco).rgb;
#ifdef HAS_gEmissionBlurred
    vec3 col2 = texture(in_gEmissionBlurred,texco).rgb;
    out_color.rgb = col1 + col2;
#else
    out_color.rgb = col1;
#endif
    // need to provide an alpha value for blending
    out_color.a = (out_color.r+out_color.g+out_color.b)/3.0;
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
#include regen.shading.light.defines
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
#if SHADING_MODEL == PHONG
    #include regen.shading.deferred.phong
#elif SHADING_MODEL == TOON
    #include regen.shading.deferred.toon
#endif
#ifdef USE_SHADOW_MAP
    #include regen.shading.shadow-mapping.sampling.dir
#endif

void main() {
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    vecTexco texco = computeTexco(texco_2D);
    
    // fetch from GBuffer
    vec3 N = fetchNormal(in_gNorWorldTexture,texco);
    float depth = texture(in_gDepthTexture, texco).r;
    vec3 P = transformTexcoToWorld(texco_2D, depth, in_layer);
    vec4 spec = texture(in_gSpecularTexture, texco);
    vec4 diff = texture(in_gDiffuseTexture, texco);

    vec3 L = normalize(in_lightDirection);
    float nDotL = dot( N, L );
    if(nDotL<=0.0) discard;
    out_color = vec4(0.0);
    
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
#ifdef USE_SHADOW_COLOR
    vec4 shadowColor = shadow2DArray(in_shadowColorTexture,shadowCoord);
    attenuation += (1.0-shadow)*(1.0-shadowColor.a);
    diff.rgb += (1.0-shadow)*shadowColor.rgb;
#endif
#else
    float attenuation = 1.0;
#endif

    // Note: shininess stored in specular buffer in the range [0,1].
    //       We map it back to [0,256] to get the shininess value.
    float sf = specularFactor(P,L,N);
    float shininess = spec.a*256.0; // map from [0,1] to [0,256]
    // multiple diffuse and specular material color with light color
    diff.rgb *= in_lightDiffuse;
    spec.rgb *= in_lightSpecular;
    // add diffuse and specular light (ambient is already added)
#if SHADING_MODEL == PHONG
    out_color.rgb += attenuation *
            phong(diff.rgb, spec.rgb, nDotL, sf, shininess);
#elif SHADING_MODEL == TOON
    out_color.rgb += attenuation *
            toon(diff.rgb, spec.rgb, nDotL, sf, shininess);
#endif
    
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
#define2 REGEN_MAX_VERTICES_ ${${RENDER_LAYER}*3}

layout(triangles) in;
layout(triangle_strip, max_vertices=${REGEN_MAX_VERTICES_}) out;

flat out int out_layer;
out vec4 out_posEye;

#include regen.states.camera.input
#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen

#define HANDLE_IO(i)

void emitVertex(vec4 posWorld, int index, int layer) {
    out_posEye = transformWorldToEye(posWorld,layer);
    gl_Position = transformEyeToScreen(out_posEye,layer);
    HANDLE_IO(index);
    EmitVertex();
}

void main() {
#for LAYER to ${RENDER_LAYER}
#ifndef SKIP_LAYER${LAYER}
    gl_Layer = ${LAYER};
    out_layer = ${LAYER};
    emitVertex(gl_in[0].gl_Position, 0, ${LAYER});
    emitVertex(gl_in[1].gl_Position, 1, ${LAYER});
    emitVertex(gl_in[2].gl_Position, 2, ${LAYER});
    EndPrimitive();
#endif
#endfor
}
#endif

-- local.fs
#include regen.shading.light.defines
#include regen.states.camera.defines

out vec4 out_color;
in vec4 in_posEye;

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
#ifdef USE_SHADOW_MAP
#include regen.math.computeDepth
#endif

#include regen.shading.light.radiusAttenuation
#include regen.shading.deferred.fetchNormal
#include regen.shading.light.specularFactor
#ifdef IS_SPOT_LIGHT
  #include regen.shading.light.spotConeAttenuation
#endif
#if SHADING_MODEL == PHONG
    #include regen.shading.deferred.phong
#elif SHADING_MODEL == TOON
    #include regen.shading.deferred.toon
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
    vec3 P = transformTexcoToWorld(texco_2D,
            texture(in_gDepthTexture, texco).r,
            in_layer);
    vec4 spec = texture(in_gSpecularTexture, texco);
    vec4 diff = texture(in_gDiffuseTexture, texco);
    vec3 lightVec = in_lightPosition - P;
    vec3 L = normalize(lightVec);
    
    // calculate attenuation
    float attenuation = radiusAttenuation(
        length(lightVec), in_lightRadius.x, in_lightRadius.y);
#ifdef IS_SPOT_LIGHT
    attenuation *= spotConeAttenuation(L,in_lightDirection,in_lightConeAngles);
#endif
    float nDotL = dot( N, L );
    // discard if facing away
    if(attenuation*nDotL<0.0) discard;
    out_color = vec4(0.0);

#ifdef USE_SHADOW_MAP
    #ifdef IS_SPOT_LIGHT
    vec4 shadowTexco = in_lightMatrix*vec4(P,1.0);
    float shadow = spotShadow${SHADOW_MAP_FILTER}(
        in_shadowTexture,
        shadowTexco,
        lightVec,
        in_lightNear,
        in_lightFar);
        #ifdef USE_SHADOW_COLOR
    vec4 shadowColor = textureProj(in_shadowColorTexture,shadowTexco);
        #endif
    #else // IS_POINT_LIGHT
    vec3 absLightVec = abs(lightVec);
    float shadowDepth = computeDepth(
        max(absLightVec .x, max(absLightVec .y, absLightVec .z)),
        in_lightNear, in_lightFar);
    float shadow = pointShadow${SHADOW_MAP_FILTER}(
        in_shadowTexture,
        L,
        shadowDepth,
        in_lightNear,
        in_lightFar,
        in_shadowInverseSize.x);
        #ifdef USE_SHADOW_COLOR
    vec4 shadowColor = shadowCube(in_shadowColorTexture,vec4(-lightVec,shadowDepth));
        #endif
    #endif // IS_POINT_LIGHT
    #ifdef USE_SHADOW_COLOR
    shadow += (1.0-shadow)*(1.0-shadowColor.a);
    diff.rgb += (1.0-shadow)*shadowColor.rgb;
    #endif
    attenuation *= shadow;
#endif // USE_SHADOW_MAP

    // Note: shininess stored in specular buffer in the range [0,1].
    //       We map it back to [0,256] to get the shininess value.
    float sf = specularFactor(P,L,N);
    float shininess = spec.a*256.0; // map from [0,1] to [0,256]
    //shininess = 1.0;
    // multiple diffuse and specular material color with light color
    diff.rgb *= in_lightDiffuse;
    spec.rgb *= in_lightSpecular;
    // add diffuse and specular light (ambient is already added)
#if SHADING_MODEL == PHONG
    out_color.rgb += attenuation *
            phong(diff.rgb, spec.rgb, nDotL, sf, shininess);
#elif SHADING_MODEL == TOON
    out_color.rgb += attenuation *
            toon(diff.rgb, spec.rgb, nDotL, sf, shininess);
#endif
}

--------------------------------------
--------------------------------------
---- Deferred Point Light Shading. Input mesh can be a cube or sphere.
--------------------------------------
--------------------------------------
-- point.vs
#define IS_POINT_LIGHT
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
    gl_Position = transformWorldToScreen(vec4(posWorld,1.0),0);
#endif
}

-- point.gs
#define IS_POINT_LIGHT
#include regen.shading.deferred.local.gs
-- point.fs
#define IS_POINT_LIGHT
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
    gl_Position = transformWorldToScreen(vec4(out_intersection,1.0),0);
#endif
}
-- spot.gs
#include regen.shading.deferred.local.gs
-- spot.fs
#define IS_SPOT_LIGHT
#include regen.shading.deferred.local.fs
