
-- fetchNormal
vec4 fetchNormal(sampler2D norWorldTex, vec2 texco) {
    vec4 N = texture(norWorldTex, texco);
    return vec4(N.xyz*2.0 - vec3(1.0), N.w);
}
vec4 fetchNormal(sampler2DArray norWorldTex, vec3 texco) {
    vec4 N = texture(norWorldTex, texco);
    return vec4(N.xyz*2.0 - vec3(1.0), N.w);
}
vec4 fetchNormal(samplerCube norWorldTex, vec3 texco) {
    vec4 N = texture(norWorldTex, texco);
    return vec4(N.xyz*2.0 - vec3(1.0), N.w);
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

void main() {
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    vecTexco texco = computeTexco(texco_2D);

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
    vec4 col1 = texture(in_gEmissionTexture,texco);
#ifdef HAS_gEmissionBlurred
    vec4 col2 = texture(in_gEmissionBlurred,texco);
    out_color.rgb = col1.rgb + col2.rgb;
#else
    out_color.rgb = col1.rgb;
#endif
    // need to provide an alpha value for blending
    //out_color.a = (out_color.r+out_color.g+out_color.b)/3.0;
    out_color.a = col1.a;
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
    vec4 N = fetchNormal(in_gNorWorldTexture,texco);
    float depth = texture(in_gDepthTexture, texco).r;
    vec3 P = transformTexcoToWorld(texco_2D, depth, in_layer);
    vec4 spec = texture(in_gSpecularTexture, texco);
    vec4 diff = texture(in_gDiffuseTexture, texco);
#ifdef USE_AMBIENT_LIGHT
    vec3 lightColor = in_lightAmbient * diff.rgb;
#else
    vec3 lightColor = vec3(0.0);
#endif
    vec3 L = normalize(in_lightDirection.xyz);
    float nDotL = dot( N.xyz, L );
    // modulate light intensity by the vertical component of the light direction
    // this makes the light less strong when coming from the side and cancels it
    // when coming from below the ground.
    nDotL *= L.y;

#ifdef USE_AMBIENT_LIGHT
    if(nDotL>0.0) {
#else
    if(nDotL<=0.0) discard;
#endif

#ifdef USE_SKY_COLOR
    vec3 lightDiffuse = texture(in_skyColorTexture, N ).rgb;
#else
    vec3 lightDiffuse = in_lightDiffuse;
#endif
    vec3 lightSpecular = in_lightSpecular;

#ifdef USE_SHADOW_MAP
    // find the texture layer
    int shadowLayer = ${NUM_SHADOW_LAYER};
    #for S_LAYER to ${NUM_SHADOW_LAYER}
    shadowLayer = min(shadowLayer, ${NUM_SHADOW_LAYER} -
            int(depth<in_lightProjParams[${S_LAYER}].y)*
            (${NUM_SHADOW_LAYER} - ${S_LAYER}));
    #endfor
    // compute texture lookup coordinate
    vec4 shadowCoord = dirShadowCoord(shadowLayer, P, in_lightMatrix[shadowLayer]);
    // compute filtered shadow
    // Note: we multiply by N.w which contains the SSAO occlusion factor.
    float attenuation = dirShadow${SHADOW_MAP_FILTER}(in_shadowTexture, shadowCoord) * N.w;
#ifdef USE_SHADOW_COLOR
    vec4 shadowColor = shadow2DArray(in_shadowColorTexture,shadowCoord);
    attenuation += (1.0-shadow)*(1.0-shadowColor.a);
    diff.rgb += mix(diff.rgb, shadowColor.rgb, shadowColor.a);
#endif
#else
    float attenuation = N.w;
#endif

    // Note: shininess stored in specular buffer in the range [0,1].
    //       We map it back to [0,256] to get the shininess value.
    float sf = specularFactor(P,L,N.xyz);
    float shininess = spec.a*256.0; // map from [0,1] to [0,256]
    // multiple diffuse and specular material color with light color
    diff.rgb *= in_lightDiffuse;
    spec.rgb *= in_lightSpecular;
    // add diffuse and specular light (ambient is already added)
#if SHADING_MODEL == PHONG
    lightColor.rgb += attenuation *
            phong(diff.rgb, spec.rgb, nDotL, sf, shininess);
#elif SHADING_MODEL == TOON
    lightColor.rgb += attenuation *
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
    lightColor.rgb *= color[shadowLayer];
#endif
#endif
#ifdef USE_AMBIENT_LIGHT
    }
#endif

    // set output color
    out_color = vec4(lightColor, 1.0);
}

--------------------------------------
--------------------------------------
--------------------------------------
--------------------------------------

-- local.gs
// pass-through geometry shader if needed (e.g. for layer selection)
#include regen.models.mesh.gs

-- local.fs
#include regen.shading.light.defines
#include regen.states.camera.defines

out vec4 out_color;
in vec4 in_posEye;
#ifdef HAS_INSTANCES
flat in int in_instanceID;
#endif

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
#ifdef IS_POINT_LIGHT
    #if POINT_LIGHT_TYPE == CUBE
        #include regen.math.computeDepth
    #endif
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
    #else // IS_POINT_LIGHT
        #if POINT_LIGHT_TYPE == CUBE
            #include regen.shading.shadow-mapping.sampling.point.cube
        #endif
        #if POINT_LIGHT_TYPE == SPHERE
            #include regen.shading.shadow-mapping.sampling.point.sphere
        #endif
        #if POINT_LIGHT_TYPE == PARABOLIC
            #include regen.shading.shadow-mapping.sampling.point.parabolic
        #endif
    #endif
#endif // USE_SHADOW_MAP

#if RENDER_TARGET == CUBE
    #include regen.math.computeCubeDirection
#endif

void main() {
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    vecTexco texco = computeTexco(texco_2D);
    
    // fetch from GBuffer
    vec4 N = fetchNormal(in_gNorWorldTexture,texco);
    vec3 P = transformTexcoToWorld(texco_2D,
            texture(in_gDepthTexture, texco).r,
            in_layer);
    vec4 spec = texture(in_gSpecularTexture, texco);
    vec4 diff = texture(in_gDiffuseTexture, texco);
    vec3 lightVec = in_lightPosition.xyz - P;
    float lightDist = length(lightVec);
    vec3 L = lightVec / lightDist;
#ifdef USE_AMBIENT_LIGHT
    vec3 lightColor = in_lightAmbient * diff.rgb;
#else
    vec3 lightColor = vec3(0.0);
#endif
    
    // calculate attenuation.
    // Note: we multiply by N.w which contains the SSAO occlusion factor.
    float attenuation = radiusAttenuation(lightDist, in_lightRadius.x, in_lightRadius.y) * N.w;
#ifdef IS_SPOT_LIGHT
    attenuation *= spotConeAttenuation(L,in_lightDirection.xyz,in_lightConeAngles);
#endif
    float nDotL = dot( N.xyz, L );

#ifdef USE_AMBIENT_LIGHT
    if(attenuation*nDotL > 0.0) {
#else
    // discard if facing away
    if(attenuation*nDotL <= 0.0) discard;
#endif
#ifdef HAS_headlightMask
    vec4 lightUV = in_viewProjectionMatrix_Light * vec4(P,1.0);
    lightUV.xy = lightUV.xy*0.5/lightUV.w + 0.5;
    #ifdef HAS_lightUVScale
    lightUV.xy *= in_lightUVScale;
    #endif
#endif

#ifdef HAS_headlightMask
    // apply the mask
    attenuation *= texture(in_headlightMask, lightUV.xy).r;
#endif
#ifdef USE_SHADOW_MAP
    float lightNear = in_lightProjParams.x;
    float lightFar = in_lightProjParams.y;
    #ifdef IS_SPOT_LIGHT
    /*************************************/
    /***** SPOT SHADOW MAPPING *****/
    /*************************************/
    vec4 shadowTexco = in_lightMatrix*vec4(P,1.0);
    float shadow = spotShadow${SHADOW_MAP_FILTER}(
        in_shadowTexture,
        shadowTexco,
        lightVec,
        lightNear,
        lightFar);
        #ifdef USE_SHADOW_COLOR
    vec4 shadowColor = textureProj(in_shadowColorTexture,shadowTexco);
        #endif
    #else // IS_POINT_LIGHT
        #if POINT_LIGHT_TYPE == PARABOLIC
    /*************************************/
    /***** PARABOLIC SHADOW MAPPING ******/
    /*************************************/
    int parabolicLayer = int(dot(L, in_lightDirection.xyz) > 0.0);
    vec4 shadowCoord = parabolicShadowCoord(
            parabolicLayer,
            P,
            in_lightMatrix[parabolicLayer],
            lightNear, lightFar);
    float shadow = parabolicShadow${SHADOW_MAP_FILTER}(in_shadowTexture, shadowCoord);
            #if NUM_SHADOW_LAYER == 1
    shadow *= float(1 - parabolicLayer);
            #endif
            #ifdef USE_SHADOW_COLOR
    vec4 shadowColor = shadow2DArray(in_shadowColorTexture, shadowCoord);
            #endif
        #endif // POINT_LIGHT_TYPE == PARABOLIC
        #if POINT_LIGHT_TYPE == CUBE
    /*************************************/
    /******** CUBE SHADOW MAPPING ********/
    /*************************************/
    vec3 absLightVec = abs(lightVec);
    float shadowDepth = computeDepth(
        max(absLightVec .x, max(absLightVec .y, absLightVec .z)),
        lightNear, lightFar);
    float shadow = pointShadow${SHADOW_MAP_FILTER}(
        in_shadowTexture,
        L,
        shadowDepth,
        lightNear,
        lightFar,
        in_shadowInverseSize.x);
            #ifdef USE_SHADOW_COLOR
    vec4 shadowColor = shadowCube(in_shadowColorTexture,vec4(-lightVec,shadowDepth));
            #endif
        #endif // POINT_LIGHT_TYPE == CUBE
    #endif // IS_POINT_LIGHT
    /*************************************/
    /*************************************/
    #ifdef USE_SHADOW_COLOR
    // Here the idea is that we actually reduce the shadow in case we have a shadow color.
    // Then we rather add the shadow color to the material color weighted by the alpha of the shadow color.
    // NOTE: this can create artifacts of colored shadows "shining through" objects and being projected on
    //       on objects behind. Not sure what would be the best way to avoid this...
    diff.rgb += mix(diff.rgb, shadowColor.rgb, shadowColor.a);
    shadow += (1.0-shadow)*(1.0-shadowColor.a);
    #endif
    attenuation *= shadow;
#endif // USE_SHADOW_MAP

    // Note: shininess stored in specular buffer in the range [0,1].
    //       We map it back to [0,256] to get the shininess value.
    float sf = specularFactor(P,L,N.xyz);
    float shininess = spec.a*256.0; // map from [0,1] to [0,256]
    //shininess = 1.0;
    // multiple diffuse and specular material color with light color
    diff.rgb *= in_lightDiffuse;
    spec.rgb *= in_lightSpecular;
    // add diffuse and specular light (ambient is already added)
#if SHADING_MODEL == PHONG
    lightColor.rgb += attenuation *
            phong(diff.rgb, spec.rgb, nDotL, sf, shininess);
#elif SHADING_MODEL == TOON
    lightColor.rgb += attenuation *
            toon(diff.rgb, spec.rgb, nDotL, sf, shininess);
#endif
#ifdef USE_AMBIENT_LIGHT
    }
#endif

    // set output color
    out_color = vec4(lightColor, 1.0);
}

--------------------------------------
--------------------------------------
---- Deferred Point Light Shading. Input mesh can be a cube or sphere.
--------------------------------------
--------------------------------------
-- point.vs
#define IS_POINT_LIGHT
#include regen.states.camera.defines
#include regen.defines.all

in vec3 in_pos;
#ifdef HAS_INSTANCES
flat out int out_instanceID;
#endif
#ifdef VS_LAYER_SELECTION
flat out int out_layer;
#endif

uniform vec2 in_lightRadius;
uniform vec4 in_lightPosition;

#include regen.layered.VS_SelectLayer
#include regen.states.camera.input
#include regen.states.camera.transformWorldToScreen

void main() {
    vec3 posWorld = in_lightPosition.xyz + in_pos*in_lightRadius.y;
    gl_Position = transformWorldToScreen(vec4(posWorld,1.0),0);
#ifdef HAS_INSTANCES
    out_instanceID = gl_InstanceID + gl_BaseInstance;
#endif // HAS_INSTANCES
    VS_SelectLayer(regen_RenderLayer());
}

-- point.gs
#define IS_POINT_LIGHT
#include regen.shading.deferred.local.gs
-- point.fs
#define IS_POINT_LIGHT
#include regen.shading.deferred.local.fs

-- point.parabolic.vs
#define POINT_LIGHT_TYPE PARABOLIC
#include regen.shading.deferred.point.vs
-- point.parabolic.gs
#define POINT_LIGHT_TYPE PARABOLIC
#include regen.shading.deferred.point.gs
-- point.parabolic.fs
#define POINT_LIGHT_TYPE PARABOLIC
#include regen.shading.deferred.point.fs

-- point.cube.vs
#define POINT_LIGHT_TYPE CUBE
#include regen.shading.deferred.point.vs
-- point.cube.gs
#define POINT_LIGHT_TYPE CUBE
#include regen.shading.deferred.point.gs
-- point.cube.fs
#define POINT_LIGHT_TYPE CUBE
#include regen.shading.deferred.point.fs

--------------------------------------
--------------------------------------
---- Deferred Spot Light Shading. Input mesh is a cone.
--------------------------------------
--------------------------------------
-- spot.vs
#include regen.states.camera.defines
#include regen.defines.all

in vec3 in_pos;
out vec3 out_intersection;
#ifdef HAS_INSTANCES
flat out int out_instanceID;
#endif
#ifdef VS_LAYER_SELECTION
flat out int out_layer;
#endif

uniform mat4 in_lightConeMatrix;

#include regen.layered.VS_SelectLayer
#include regen.states.camera.input
#include regen.states.camera.transformWorldToScreen

void main() {
    out_intersection = (in_lightConeMatrix * vec4(in_pos,1.0)).xyz;
#ifdef HAS_INSTANCES
    out_instanceID = gl_InstanceID + gl_BaseInstance;
#endif // HAS_INSTANCES
    VS_SelectLayer(regen_RenderLayer());
    gl_Position = transformWorldToScreen(vec4(out_intersection,1.0),0);
}

-- spot.gs
#include regen.shading.deferred.local.gs
-- spot.fs
#define IS_SPOT_LIGHT
#include regen.shading.deferred.local.fs
