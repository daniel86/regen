
-- fogIntensity
#ifndef fogIntensity_DEFINED
#define2 fogIntensity_DEFINED
const vec2 in_fogDistance = vec2(0.0,100.0);

float fogIntensity(float d) {
    float x = smoothstep(in_fogDistance.x, in_fogDistance.y, d);
#ifdef USE_EXP_FOG
    return 1.0 - exp( -pow(1.75*x, 2.0) );
#elif defined(USE_EXP2_FOG)
    return 1.0 - exp( -pow(1.5*x, 2.0) * pow(1.5*x, 2.0) );
#elif defined(USE_S_CURVE_FOG)
    return x * x * (3.0 - 2.0 * x); // smoothstep curve, manual
#else
    return x;
#endif
}
#endif

-- applyFogToColor
#ifndef applyFogToColor_DEFINED
#define2 applyFogToColor_DEFINED
#include regen.weather.fog.fogIntensity
#ifdef HAS_lightDirection_Sun
    #include regen.weather.utility.sunIntensity
#endif

float fogIntensity(vec3 posWorld, vec3 eyeDir, float d) {
    float fogFactor = fogIntensity(d) * in_fogDensity;
    #ifdef HAS_fogHeightMin && HAS_fogHeightMax
    // height-based falloff
    fogFactor *=  1.0 - smoothstep(in_fogHeightMin, in_fogHeightMax, posWorld.y);
    #endif
    return fogFactor;
}

const vec3 in_fogColor = vec3(1.0);
const float in_fogDensity = 1.0;
#ifdef HAS_lightDirection_Sun
const vec3 in_lightDiffuse_Sun = vec3(1.0, 0.9, 0.7);
const vec3 in_warmTint = vec3(1.0, 0.8, 0.6);
#endif
#ifdef HAS_skyColorTexture
const vec2 in_farDistance = vec2(150.0, 200.0);
#endif

vec3 applyFogToColor(vec3 sceneColor, float sceneDepth, vec3 posWorld) {
    vec3 eye = posWorld - REGEN_CAM_POS_(in_layer);
    float eyeLength = length(eye);
    vec3 eyeDir = eye / eyeLength;
    // Compute fog factor based on distance to camera, eye direction, and (optionally) height.
    float fogFactor = fogIntensity(posWorld, eyeDir, eyeLength) * in_fogDensity;
    #ifdef HAS_fogColor
    vec3 fogColor = in_fogColor;
    #else
    vec3 fogColor = vec3(1.0);
    #endif

    #ifdef HAS_lightDirection_Sun
    float daytimeBlend = smoothstep(-0.1, 0.1, in_lightDirection_Sun.y);
        #ifdef USE_DIRECTIONAL_SCATTERING
    // Directional Fog Scattering
    float scatteringAmount = pow(max(dot(eyeDir, in_lightDirection_Sun), 0.0), 2.0);
    scatteringAmount = pow(scatteringAmount, 4.0) * (1.0 - fogFactor);
    // cancel scattering if sun is below horizon
    scatteringAmount *= daytimeBlend;
    fogColor = mix(fogColor, in_lightDiffuse_Sun, scatteringAmount);
        #endif

    // Vary Fog Based on Sun Elevation using Day-Twilight-Night-Intensity Mapping (Butterworth-Filter)
    float sunFactor = 1.0 - sunIntensity();
    // mix fog color with warm tint based on sun elevation
    fogColor = mix(in_warmTint, fogColor, sunFactor);
    // darken fog color based on sun elevation
    fogColor *= clamp(sunFactor, 0.0, 1.0);
    #endif

    fogColor = mix(sceneColor, fogColor, fogFactor);

    #ifdef HAS_skyColorTexture
    // blend into sky to avoid hard edges at far plane
    vec3 skyColor = texture(in_skyColorTexture, eyeDir).rgb;
    float farPlaneBlend = smoothstep(in_farDistance.x, in_farDistance.y, eyeLength) * fogFactor;
    fogColor = mix(fogColor, skyColor, farPlaneBlend);
    #endif

    return fogColor;
}
#endif

--------------------------------------
--------------------------------------
---- Computes fog by distance to camera.
--------------------------------------
--------------------------------------
-- distance.schema

-- distance.vs
#include regen.filter.sampling.vs
-- distance.gs
#include regen.filter.sampling.gs
-- distance.fs
#include regen.states.camera.defines

out vec4 out_color;

uniform sampler2D in_gDepthTexture;
uniform sampler2D in_gColorTexture;

#define USE_DIRECTIONAL_SCATTERING

#include regen.filter.sampling.computeTexco
#include regen.states.camera.input
#include regen.states.camera.transformTexcoToWorld
#include regen.weather.fog.applyFogToColor

void main() {
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    vecTexco sceneUV = computeTexco(texco_2D);

    vec3 sceneColor = texture(in_gColorTexture, sceneUV).rgb;
    float sceneDepth = texture(in_gDepthTexture, sceneUV).x;
    vec3 posWorld = transformTexcoToWorld(texco_2D, sceneDepth, in_layer);

    vec3 foggedColor = applyFogToColor(sceneColor, sceneDepth, posWorld);
    out_color = vec4(foggedColor, 1.0);
}

--------------------------------------
---- Draw a fog volume. Can be used for spot and point lights.
--------------------------------------
-- volumetric.fs
#include regen.states.camera.defines

out vec3 out_color;
#ifdef IS_SPOT_LIGHT
in vec3 in_intersection;
#endif
in vec4 in_posEye;

// G-buffer input
uniform sampler2D in_gDepthTexture;
// light input
#include regen.shading.light.input.deferred
#ifdef USE_SHADOW_MAP
const float in_shadowSampleStep = 0.025;
const float in_shadowSampleThreshold = 0.075;
#endif // USE_SHADOW_MAP
// camera input
#include regen.states.camera.input
// fog input
uniform float in_fogExposure;
uniform vec2 in_fogRadiusScale;
#ifdef IS_SPOT_LIGHT
uniform vec2 in_fogConeScale;
#endif
uniform vec2 in_fogDistance;

#include regen.filter.sampling.computeTexco
#include regen.math.pointVectorDistance
#include regen.states.camera.transformTexcoToWorld

#include regen.states.clipping.isClipped

#include regen.shading.light.radiusAttenuation
#ifdef IS_SPOT_LIGHT
    #include regen.shading.light.spotConeAttenuation
#endif

#ifdef USE_SHADOW_MAP
    #ifdef IS_SPOT_LIGHT
        #include regen.shading.shadow-mapping.sampling.spot
    #else // IS_POINT_LIGHT
        #if POINT_LIGHT_TYPE == CUBE
            #include regen.shading.shadow-mapping.sampling.point.cube
            #include regen.math.computeCubeLayer
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

#include regen.weather.fog.fogIntensity

#ifdef IS_SPOT_LIGHT
void solvableQuadratic(
    float a, float b, float c,
    out float t0, out float t1)
{
    // Note: discriminant should always be >=0.0 because we are
    // using the cone mesh as input.
    float discriminant = b*b - 4.0*a*c;
    // numerical receipes 5.6 (this method ensures numerical accuracy is preserved)
    float t = -0.5 * (b + sign(b)*sqrt(discriminant));
    t0 = t / a;
    t1 = c / t;
}
vec2 computeConeIntersections(
    vec3 pos, vec3 ray,
    vec3 conePos, vec3 coneDir,
    float cosAngle)
{
    // TODO: cone intersection could be simplified knowing one intersection, i guess
    vec2 t = vec2(0.0);
    vec3 dp = pos-conePos;
    float a = dot(coneDir,ray);
    float b = dot(coneDir,dp);
    float phi = cosAngle*cosAngle;
    solvableQuadratic(
         a*a - phi*dot(ray,ray),
        (a*b - phi*dot(ray,dp))*2.0,
         b*b - phi*dot(dp,dp),
         t.x,t.y);
    // t.x is backface intersection and t.y frontface
    t = vec2( min(t.x,t.y), max(t.x,t.y) );
    // compute intersection points
    vec3 x0 = pos + t.x*ray;
    vec3 x1 = pos + t.y*ray;
    // near intersects reflected cone ?
    float reflected0 = float(dot(coneDir, x0-conePos)<0.0);
    // far intersects reflected cone ?
    float reflected1 = float(dot(coneDir, x1-conePos)<0.0);
    t = (1.0-reflected0-reflected1)*t +
        vec2(reflected0*t.y, reflected0 + reflected1*t.x);
    return t;
}
#endif

#ifdef USE_SHADOW_MAP
float volumeShadow(vec3 start, vec3 stop, float _step)
{
    vec3 p = start, lightVec;
    float shadow = 0.0, shadowDepth;
    float lightNear = in_lightProjParams.x;
    float lightFar = in_lightProjParams.y;
    // ray through the light volume
    vec3 stepRay = stop-start;
    // scale factor for the ray (clamp to minimum to avoid tight samples)
    float step = max(_step, in_shadowSampleThreshold/length(stepRay));
    stepRay *= step;
    // step through the volume
    for(float i=step; i<1.0; i+=step) {
        lightVec = in_lightPosition.xyz - p;
    #ifdef IS_POINT_LIGHT
        /*************************************/
        /***** PARABOLIC SHADOW MAPPING ******/
        /*************************************/
        #if POINT_LIGHT_TYPE == PARABOLIC
        int parabolicLayer = int(dot(lightVec, in_lightDirection[0].xyz) > 0.0);
        vec4 shadowCoord = parabolicShadowCoord(
                parabolicLayer,
                P,
                in_lightMatrix[parabolicLayer],
                lightNear, lightFar);
        float shadow = parabolicShadowSingle(in_shadowTexture, shadowCoord);
            #if NUM_SHADOW_LAYER == 1
        shadow *= float(1 - parabolicLayer);
            #endif
        #endif
        /*************************************/
        /******** CUBE SHADOW MAPPING ********/
        /*************************************/
        #if POINT_LIGHT_TYPE == CUBE
        shadowDepth = (vec4(lightVec,1.0)*in_lightMatrix[computeCubeLayer(lightVec)]).z;
        shadow += pointShadowSingle(
            in_shadowTexture,
            lightVec,
            shadowDepth,
            lightNear,
            lightFar,
            in_shadowInverseSize.x);
        #endif
    #endif
    #ifdef IS_SPOT_LIGHT
        shadow += spotShadowSingle(
                in_shadowTexture, in_lightMatrix*vec4(p,1.0),
                lightVec, lightNear, lightFar);
    #endif
        p += stepRay;
    }
    return shadow*step;
}
#endif

void main()
{
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    vecTexco texco = computeTexco(texco_2D);
    
    vec3 vertexPos = transformTexcoToWorld(texco_2D, texture(in_gDepthTexture, texco).x, in_layer);
    vec3 vertexRay = vertexPos-in_cameraPosition.xyz;
    // fog volume scales light radius
    vec2 lightRadius = in_lightRadius*in_fogRadiusScale;
    // compute point in the volume with maximum light intensity
#ifdef IS_SPOT_LIGHT
    // compute a ray. all intersections must be in range [0,1]*ray
    vec3 ray1 = in_intersection - in_cameraPosition.xyz;
    float toggle = float(dot(ray1,ray1) > dot(vertexRay,vertexRay));
    vec3 ray = toggle*vertexRay + (1.0-toggle)*ray1;
    // compute intersection points
    vec2 t = computeConeIntersections(
        in_cameraPosition.xyz, ray,
        in_lightPosition.xyz,
        normalize(in_lightDirection.xyz),
        in_lightConeAngles.y);
    t.x = clamp(t.x,0.0,1.0);
    t.y = clamp(t.y,0.0,1.0);
    // clamp to ray length
    vec3 x = in_cameraPosition.xyz + 0.5*(t.x+t.y)*ray;
#else
    float d = clamp(pointVectorDistance(
        vertexRay, in_lightPosition.xyz - in_cameraPosition.xyz), 0.0, 1.0);
    vec3 x = in_cameraPosition.xyz + d*vertexRay;
#endif
    // compute fog exposure by distance to camera
    float dCam = length(x-in_cameraPosition.xyz)/length(vertexRay);
    // compute fog exposure by distance to camera
    float exposure = in_fogExposure * (1.0 - fogIntensity(dCam));
#ifdef IS_SPOT_LIGHT
    // approximate spot falloff.
    exposure *= spotConeAttenuation(
        normalize(in_lightPosition.xyz - x),
        in_lightDirection.xyz,
        in_lightConeAngles*in_fogConeScale);
    vec3 start = in_cameraPosition.xyz + t.x*ray;
    vec3 stop = in_cameraPosition.xyz + t.y*ray;
#ifdef HAS_clipPlane
    bool clip0 = isClipped(in_cameraPosition.xyz);
    bool clip1 = isClipped(start);
    bool clip2 = isClipped(stop);
    if(clip0==clip1 && clip1==clip2) discard;
#endif
    // compute distance attenuation.
    float a0 = radiusAttenuation(min(
        distance(in_lightPosition.xyz, start),
        distance(in_lightPosition.xyz, stop)),
        lightRadius.x, lightRadius.y);
#else
#ifdef HAS_clipPlane
    bool clip0 = isClipped(in_cameraPosition.xyz);
    bool clip1 = isClipped(in_lightPosition.xyz);
    if(clip0==clip1) discard;
#endif
    // compute distance attenuation.
    // vertexRay and the light position.
    float lightDistance = distance(in_lightPosition.xyz, x);
    float a0 = radiusAttenuation(lightDistance, lightRadius.x, lightRadius.y);
#endif

#ifdef USE_SHADOW_MAP
    // sample shadow map along ray through volume
#ifdef IS_POINT_LIGHT
    float omega = sqrt(lightRadius.y*lightRadius.y - lightDistance*lightDistance);
    vec3 start = in_cameraPosition.xyz + clamp(d+omega, 0.0, 1.0)*vertexRay;
    vec3 stop = in_cameraPosition.xyz + clamp(d-omega, 0.0, 1.0)*vertexRay;
#endif
    exposure *= volumeShadow(start,stop,in_shadowSampleStep);
#endif
    out_color = (exposure * a0) * in_lightDiffuse;
}

--------------------------------------
--------------------------------------
---- Volumetric Fog for point lights.
--------------------------------------
--------------------------------------
-- volumetric.point.vs
#define IS_POINT_LIGHT
#include regen.shading.deferred.point.vs
-- volumetric.point.gs
#include regen.shading.deferred.point.gs
-- volumetric.point.fs
#define IS_POINT_LIGHT
#include regen.weather.fog.volumetric.fs

-- volumetric.parabolic.vs
#define POINT_LIGHT_TYPE PARABOLIC
#include regen.weather.fog.volumetric.point.vs
-- volumetric.parabolic.gs
#define POINT_LIGHT_TYPE PARABOLIC
#include regen.weather.fog.volumetric.point.gs
-- volumetric.parabolic.fs
#define POINT_LIGHT_TYPE PARABOLIC
#include regen.weather.fog.volumetric.point.fs

-- volumetric.cube.vs
#define POINT_LIGHT_TYPE CUBE
#include regen.weather.fog.volumetric.point.vs
-- volumetric.cube.gs
#define POINT_LIGHT_TYPE CUBE
#include regen.weather.fog.volumetric.point.gs
-- volumetric.cube.fs
#define POINT_LIGHT_TYPE CUBE
#include regen.weather.fog.volumetric.point.fs

--------------------------------------
--------------------------------------
---- Volumetric Fog for spot lights.
--------------------------------------
--------------------------------------
-- volumetric.spot.vs
#define IS_SPOT_LIGHT
#include regen.shading.deferred.spot.vs
-- volumetric.spot.gs
#include regen.shading.deferred.spot.gs
-- volumetric.spot.fs
#define IS_SPOT_LIGHT
#include regen.weather.fog.volumetric.fs

