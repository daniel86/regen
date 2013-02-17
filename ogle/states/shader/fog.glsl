
-- fogIntensity
float fogIntensity(float d)
{
    // TODO: none linear modes
    return smoothstep(in_fogStart, in_fogEnd, d);
}

--------------------------------------
---- Distance Fog.
---- Fades to Sky color or Constant color.
---- Transparency can be handled when a depth texture for
---- transparent objects is provided.
----     Mesh  : Unit Quad
----     Input : Scene Depth/TBuffer
----     Target: Color Texture
----     Blend : Add
--------------------------------------

-- distance.vs
in vec3 in_pos;
out vec2 out_texco;
void main()
{
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- distance.fs
out vec3 output;
in vec2 in_texco;

uniform sampler2D in_gDepthTexture;
#ifdef USE_TBUFFER
uniform sampler2D in_tDepthTexture;
uniform sampler2D in_tColorTexture;
#endif

#ifdef USE_SKY_COLOR
uniform samplerCube in_skyColor;
#else
const vec3 in_fogColor = vec3(1.0);
#endif
const float in_fogStart = 0.0;
const float in_fogEnd = 100.0;
const float in_fogDensity = 1.0;

void main() {
    float d0 = texture(in_gDepthTexture, in_texco).x;
    vec3 eye0 = texcoToWorldSpace(in_texco, d0) - in_cameraPosition;
    float factor0 = fogIntensity(length(eye));
    
#ifdef USE_SKY_COLOR
    vec3 fogColor = texture(in_skyColorTexture, eye0);
#else
    vec3 fogColor = in_fogColor;
#endif
    
#ifdef USE_TBUFFER
    float d1 = texture(in_tDepthTexture, in_texco).x;
    vec3 eye1 = texcoToWorldSpace(in_texco, d1) - in_cameraPosition;
    
    // use standard fog color from eye to transparent object
    float factor1 = fogIntensity(length(eye1));
    output = (factor1*in_fogDensity)*fogColor;
    
    // starting from transparent object to scene depth sample use alpha blended fog color.
    vec4 tcolor = texture(in_tColorTexture, in_texco).x;
    vec3 blended = fogColor*(1.0-tcolor.a) + tcolor.rgb*tcolor.a;
    // substract intensity from eye to p1
    factor0 -= factor1;
    // multiple by alpha value (no fog behind opaque objects)
    factor0 *= (1.0-tcolor.a);
    output += (factor0*in_fogDensity) * blended;

#else
    output = (factor0*in_fogDensity)*fogColor;
#endif
}

-------------------
-------------------

-- volumetric.fs
out vec3 output;
in vec2 in_texco;

uniform sampler2D in_gDepthTexture;
#ifdef USE_TBUFFER
uniform sampler2D in_tDepthTexture;
uniform sampler2D in_tColorTexture;
#endif

uniform vec3 in_lightPosition;
#ifdef IS_SPOT_LIGHT
uniform vec3 in_lightDirection;
uniform vec2 in_lightConeAngles;
#endif
uniform vec2 in_lightRadius;
uniform vec3 in_lightDiffuse;

uniform vec3 in_cameraPosition;

const float in_fogExposure=1.0;

#include shading.radiusAttenuation

#include fog.pointVectorDistance
#include fog.fogIntensity

void main()
{
    float depth0 = texture(in_gDepthTexture, in_texco).x;
    vec3 posWorld0 = texcoToWorldSpace(in_texco, depth0);
#ifdef USE_TBUFFER
    float depth1 = texture(in_tDepthTexture, in_texco).x;
    vec3 posWorld1 = texcoToWorldSpace(in_texco, depth1);
#endif
    
    // vector from camera to vertex
    vec3 vertexRay = posWorld0 - in_cameraPosition;
    // vector from camera to light
    vec3 lightRay = in_lightPosition - in_cameraPosition;
    // find point on vertex ray nearest to the light
    float dCamNearest = clamp( pointVectorDistance(vertexRay,lightRay), 0.0, 1.0);
    vec3 nearestPoint = in_cameraPosition + dCamNearest*vertexRay;
    float exposure = in_fogExposure * (1.0 - fogIntensity(dCamNearest));

#ifdef IS_SPOT_LIGHT
    {
        // TODO: intersection test ?
        //if(gl_FragCoord.z < depth) discard;
        // Set cone density based on vertexRay entering angle.
        float intersectionAngle = dot(in_lightDirection, vertexRay);
        // TODO: play with other functions
        exposure *= 0.5 + 0.5*(1.0-intersectionAngle);
    }
#endif
    
    float znl = distance(nearestPoint, in_lightPosition);
    float a0 = radiusAttenuation(znl,
        in_lightRadius.x, in_lightRadius.y);
#ifdef USE_TBUFFER
    float a1 = radiusAttenuation(
        distance(posWorld1, in_lightPosition),
        in_lightRadius.x, in_lightRadius.y));
#endif

#ifdef USE_TBUFFER
    vec4 tcolor = texture(in_tColorTexture, in_texco).x;
#if 0
    // XXX: use this ? radius must be found in GS anyway
    float dz = sqrt(pow(in_radius,2) - pow(dnl,2));
    float blendFactor = smoothstep(znl - dz, znl + dz, distance(in_cameraPosition,posWorld1));
#else
    // x=1 -> transparent object in front else x=0
    // when transparent object is in front then at least 50% of the volume
    // is blended with the transparent color
    float x = float(dCamNearest>distance(in_cameraPosition,posWorld1));
    // occlusion=1 -> the other 50% are also occluded.
    float occlusion = x - (2.0*x - 1.0)*a1/a0;
    // linear blend between unoccluded volume and transparency occluded
    // volume.
    float blendFactor = x*0.5 + occlusion*0.5;
#endif
    // apply unoccluded fog
    output  = (1.0-blendFactor) * in_lightDiffuse;
    // apply transparency occluded fog using alpha blending between fog
    // and transparency color. Also scale result by alpha inverse.
    output += (blendFactor*(1.0-tcolor.a)) *
        (in_lightColor*(1.0-tcolor.a) + tcolor.rgb*tcolor.a);
    // scale by attenuation and exposure factor
    output *= exposure * a0;

#else
    output = (exposure * a0) * in_lightColor;
#endif
}

--------------------------------------
---- Volumetric Fog for point lights.
----     Mesh  : Points
----     Input : Scene Depth/TBuffer
----     Target: Color Texture
----     Blend : Add
--------------------------------------

-- volumetric.point.vs
// #undef IS_SPOT_LIGHT
#include shading.light-sprite.vs

-- volumetric.point.gs
// #undef IS_SPOT_LIGHT
#include shading.light-sprite.gs

-- volumetric.point.fs
// #undef IS_SPOT_LIGHT
#include fog.volumetric.fs

--------------------------------------
---- Volumetric Fog for spot lights.
--------------------------------------
---- TODO: better use intersection equation then cone mesh ?

-- volumetric.spot.vs
#define IS_SPOT_LIGHT
#include shading.light-sprite.vs

-- volumetric.spot.gs
#define IS_SPOT_LIGHT
#include shading.light-sprite.gs

-- volumetric.spot.fs
#define IS_SPOT_LIGHT
#include fog.volumetric.fs

