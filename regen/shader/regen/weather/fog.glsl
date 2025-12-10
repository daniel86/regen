
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
vec2 computeConeIntersections(vec3 pos, vec3 ray, vec3 conePos, vec3 coneDir, float cosAngle) {
    // NOTE: We know one intersection point along the ray already, either entering or exiting
    // depending on whether we are outside or inside the cone.
    // But we still need to solve the quadratic to get the other intersection point.

    vec3 dp = pos-conePos;
    float a = dot(coneDir,ray);
    float b = dot(coneDir,dp);
    float phi = cosAngle*cosAngle;

    float aa = a*a - phi*dot(ray,ray);
    float ab = 2.0*(a*b - phi*dot(ray,dp));
    float bb = b*b - phi*dot(dp,dp);

    // quadratic: stable NR formula
    float disc = ab*ab - 4.0 * aa * bb;
    disc = max(disc, 0.0); // clamp numeric noise
    float s = sqrt(disc);
    float q = -0.5 * (ab + sign(ab) * s);
    float t0 = q / aa;
    float t1 = bb / q;

    // sort
    float nearT = min(t0, t1);
    float farT = max(t0, t1);

    // discard intersections on reflected/back cone side
    float reflected0 = float(dot(coneDir, dp + nearT*ray) < 0.0);
    float reflected1 = float(dot(coneDir, dp + farT*ray) < 0.0);

    // combine safely
    return (1.0 - reflected0 - reflected1) * vec2(nearT, farT)
             + vec2(reflected0 * farT, reflected0 + reflected1 * nearT);
}
#endif

#ifdef USE_SHADOW_MAP
float volumeShadow(vec3 start, vec3 stop, float step) {
    vec3 p = start;
    float shadow = 0.0, shadowDepth;
    float lightNear = in_lightProjParams.x;
    float lightFar = in_lightProjParams.y;
    // ray through the light volume
    vec3 stepRay = stop-start;
    float lengthStep = max(step, in_shadowSampleThreshold / max(length(stepRay), 1e-6));
    int steps = int(clamp(ceil(1.0 / lengthStep), 1.0, 256.0)); // enforce upper bound
    stepRay *= lengthStep;

    // step through the volume
    for (int i = 0; i < steps; ++i) {
        vec3 lightVec = in_lightPosition.xyz - p;
        vec3 samplePos = p + stepRay * 0.5;
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
                in_shadowTexture, in_lightMatrix*vec4(samplePos,1.0),
                lightVec, lightNear, lightFar);
    #endif
        p += stepRay;
    }

    return shadow / float(steps);
}
#endif

void main() {
    // fog volume scales light radius
    vec2 lightRadius = in_lightRadius*in_fogRadiusScale;
    float lightRadiusX = lightRadius.x*lightRadius.x;
    float lightRadiusY = lightRadius.y*lightRadius.y;
    vec3 lightPos = in_lightPosition.xyz;
#ifdef IS_SPOT_LIGHT
    vec3 lightDir = normalize(in_lightDirection.xyz);
#endif

    vec3 cameraPos = in_cameraPosition.xyz;
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    vecTexco texco = computeTexco(texco_2D);

    vec3 vertexPos = transformTexcoToWorld(texco_2D,
        texture(in_gDepthTexture, texco).x, in_layer);
    vec3 vertexRay = vertexPos - cameraPos;

    // compute point `x` in the volume with maximum light intensity
#ifdef IS_SPOT_LIGHT
    // Compute ray through volume. Note that we need to cover two cases here:
    // 1) the vertex is inside the cone volume, in which case we use the
    //    intersection point as input to compute the other intersection point.
    // 2) the vertex is outside the cone volume, in which case we use the
    //    ray from the camera to the vertex to compute both intersection points.
    vec3 ray1 = in_intersection - cameraPos;
    float toggle = float(dot(ray1,ray1) > dot(vertexRay,vertexRay));
    vec3 ray = toggle*vertexRay + (1.0-toggle)*ray1;

    // compute intersection points
    vec2 t = computeConeIntersections(
        cameraPos, ray,
        lightPos, lightDir,
        in_lightConeAngles.y);
    t.x = clamp(t.x,0.0,1.0);
    t.y = clamp(t.y,0.0,1.0);
    vec3 x = cameraPos + 0.5*(t.x+t.y)*ray;
#else // IS_POINT_LIGHT
    float d = clamp(pointVectorDistance(vertexRay, lightPos - cameraPos), 0.0, 1.0);
    vec3 x = cameraPos + d*vertexRay;
#endif

    vec3 lx = lightPos - x;
    // compute fog exposure by distance to camera
    float dCam = length(x - cameraPos)/length(vertexRay);
    // compute fog falloff/attenuation by distance
    float exposure = in_fogExposure * (1.0 - fogIntensity(dCam));

#ifdef IS_SPOT_LIGHT
    // approximate spot falloff.
    exposure *= spotConeAttenuation(
        normalize(lx), lightDir,
        in_lightConeAngles*in_fogConeScale);
    vec3 start = cameraPos + t.x*ray;
    vec3 stop  = cameraPos + t.y*ray;
    #ifdef HAS_clipPlane
    bool clip0 = isClipped(cameraPos);
    bool clip1 = isClipped(start);
    bool clip2 = isClipped(stop);
    if(clip0==clip1 && clip1==clip2) discard;
    #endif
    // compute distance attenuation.
    vec3 lx0 = lightPos - start;
    vec3 lx1 = lightPos - stop;
    float a0 = radiusAttenuation(
        min(dot(lx0,lx0), dot(lx1,lx1)),
        lightRadiusX, lightRadiusY);
#else // IS_POINT_LIGHT
    #ifdef HAS_clipPlane
    if(isClipped(cameraPos) == isClipped(lightPos)) discard;
    #endif
    // compute distance attenuation.
    float lightDistanceSq = dot(lx,lx);
    float a0 = radiusAttenuation(lightDistanceSq, lightRadiusX, lightRadiusY);
    #ifdef USE_SHADOW_MAP
    float omega = sqrt(lightRadiusY - lightDistanceSq);
    vec3 start = cameraPos + clamp(d+omega, 0.0, 1.0)*vertexRay;
    vec3 stop  = cameraPos + clamp(d-omega, 0.0, 1.0)*vertexRay;
    #endif
#endif

#ifdef USE_SHADOW_MAP
    // sample shadow map along ray through volume
    exposure *= volumeShadow(start, stop, in_shadowSampleStep);
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

