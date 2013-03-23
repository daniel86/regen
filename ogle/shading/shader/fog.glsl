
-- fogIntensity
float fogIntensity(float d)
{
    float x = smoothstep(in_fogDistance.x, in_fogDistance.y, d);
#ifdef USE_EXP_FOG
    return 1.0 - exp( -pow(1.75*x, 2.0) );
#else
    return x;
#endif
}

--------------------------------------
---- Distance Fog.
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
out vec4 out_color;
in vec2 in_texco;

uniform sampler2D in_gDepthTexture;
#ifdef USE_TBUFFER
uniform sampler2D in_tDepthTexture;
uniform sampler2D in_tColorTexture;
#endif

#ifdef USE_SKY_COLOR
uniform samplerCube in_skyColorTexture;
#else
const vec3 in_fogColor = vec3(1.0);
#endif
const vec2 in_fogDistance = vec2(0.0,100.0);
const float in_fogDensity = 1.0;

uniform vec3 in_cameraPosition;
uniform mat4 in_viewMatrix;
uniform mat4 in_inverseViewProjectionMatrix;

#include fog.fogIntensity
#include utility.texcoToWorldSpace

void main() {
    float d0 = texture(in_gDepthTexture, in_texco).x;
    vec3 eye0 = texcoToWorldSpace(in_texco, d0) - in_cameraPosition;
    float factor0 = fogIntensity(length(eye0));
    
#ifdef USE_SKY_COLOR
    // TODO: use normal for cubemap lookup or reflected eye ?
    vec3 fogColor = texture(in_skyColorTexture, eye0).rgb;
#else
    vec3 fogColor = in_fogColor;
#endif
    
#ifdef USE_TBUFFER
    float d1 = texture(in_tDepthTexture, in_texco).x;
    vec3 eye1 = texcoToWorldSpace(in_texco, d1) - in_cameraPosition;
    
    // use standard fog color from eye to transparent object
    float factor1 = fogIntensity(length(eye1));
    out_color = (factor1*in_fogDensity)*fogColor;
    
    // starting from transparent object to scene depth sample use alpha blended fog color.
    vec4 tcolor = texture(in_tColorTexture, in_texco).x;
    vec3 blended = fogColor*(1.0-tcolor.a) + tcolor.rgb*tcolor.a;
    // substract intensity from eye to p1
    factor0 -= factor1;
    // multiple by alpha value (no fog behind opaque objects)
    factor0 *= (1.0-tcolor.a);
    out_color += (factor0*in_fogDensity) * blended;

#else
    out_color = vec4(fogColor, factor0*in_fogDensity);
#endif
}

-------------------
-------------------

-- volumetric.fs
out vec3 out_color;
#ifdef IS_SPOT_LIGHT
in vec3 in_intersection;
#endif

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

uniform vec2 in_viewport;
uniform vec3 in_cameraPosition;
uniform mat4 in_inverseViewProjectionMatrix;
uniform mat4 in_viewProjectionMatrix;
#ifdef IS_SPOT_LIGHT
uniform mat4 in_modelMatrix;
#endif

uniform float in_fogExposure;
uniform vec2 in_fogRadiusScale;
#ifdef IS_SPOT_LIGHT
uniform vec2 in_fogConeScale;
#endif
uniform vec2 in_fogDistance;

#include shading.radiusAttenuation
#ifdef IS_SPOT_LIGHT
#include shading.spotConeAttenuation
#endif
#include utility.pointVectorDistance
#include utility.texcoToWorldSpace
#include utility.worldSpaceToTexco
#include fog.fogIntensity

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
    // TODO: can be simplified knowing one intersection.
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

#if 0
const float in_occlusionDensity = 1.0;
const float in_occlusionSamples = 20.0;
float volumeOcclusion(vec2 texco)
{
    vec4 ss = in_viewProjectionMatrix*vec4(in_lightPosition,1.0);
    vec2 lightTexco = (ss.xy/ss.w + vec2(1.0))*0.5;
    lightTexco.x = 1.0 - lightTexco.x;

    //vec2 lightTexco = worldSpaceToTexco(vec4(in_lightPosition,1.0)).xy;
    float stepScale = 1.0/(in_occlusionSamples);
    float occlusions = 0.0;
    // ray step size
    vec2 dt = (texco-lightTexco)*stepScale;
    vec2 t = texco;
    float d0 = texture(in_gDepthTexture, texco).r;
    float d1 = texture(in_gDepthTexture, lightTexco).r;
    float dmin = min(d0,d1);
    float dmax = max(d0,d1);
    // shoot screen space ray from texco to lightTexco
    for (int i=2; i<in_occlusionSamples; i++)
    {
        t -= dt;
        float d = texture(in_gDepthTexture, t).r;
        occlusions += float(d<dmin || d>dmax);
    }
    return occlusions*stepScale;
}
#endif

void main()
{
    vec2 texco = gl_FragCoord.xy/in_viewport;
    vec3 vertexPos = texcoToWorldSpace(texco, texture(in_gDepthTexture, texco).x);
    vec3 vertexRay = vertexPos-in_cameraPosition;
    // fog volume scales light radius
    vec2 lightRadius = in_lightRadius*in_fogRadiusScale;
    // compute point in the volume with maximum light intensity
#ifdef IS_SPOT_LIGHT
    // compute a ray. all intersections must be in range [0,1]*ray
    vec3 ray1 = in_intersection - in_cameraPosition;
    float toggle = float(dot(ray1,ray1) > dot(vertexRay,vertexRay));
    vec3 ray = toggle*vertexRay + (1.0-toggle)*ray1;
    // compute intersection points
    vec2 t = computeConeIntersections(
        in_cameraPosition, ray,
        in_lightPosition,
        normalize(in_lightDirection),
        in_lightConeAngles.y);
    // clamp to ray length
    vec3 x = in_cameraPosition +
        0.5*(clamp(t.x,0.0,1.0)+clamp(t.y,0.0,1.0))*ray;
#else
    float d = pointVectorDistance(vertexRay, in_lightPosition - in_cameraPosition);
    vec3 x = in_cameraPosition + clamp(d, 0.0, 1.0)*vertexRay;
#endif
    // compute fog exposure by distance to camera
    float dCam = length(x-in_cameraPosition)/length(vertexRay);
    // compute fog exposure by distance to camera
    float exposure = in_fogExposure * (1.0 - fogIntensity(dCam));
#ifdef IS_SPOT_LIGHT
    // approximate spot falloff.
    exposure *= spotConeAttenuation(
        normalize(in_lightPosition - x),
        in_lightDirection,
        in_lightConeAngles*in_fogConeScale);
    // compute distance attenuation.
    float a0 = radiusAttenuation(min(
        distance(in_lightPosition, in_cameraPosition + t.x*ray),
        distance(in_lightPosition, in_cameraPosition + t.y*ray)),
        lightRadius.x, lightRadius.y);
#else
    // compute distance attenuation.
    // vertexRay and the light position.
    float a0 = radiusAttenuation(
        distance(in_lightPosition, x),
        lightRadius.x, lightRadius.y);
#endif
#if 0 
    // calculate volume occlusion
    exposure *= 1.0-volumeOcclusion(texco);
#endif

#ifdef USE_TBUFFER
    vec3 alphaPos = texcoToWorldSpace(texco, texture(in_tDepthTexture, texco).x);
    float dLightAlpha = distance(alphaPos, in_lightPosition);
    float a1 = radiusAttenuation(dLightAlpha, lightRadius.x, lightRadius.y));
    vec4 tcolor = texture(in_tColorTexture, texco).x;
#if 0
    // XXX: use this ? radius must be found in GS anyway
    float dz = sqrt(pow(in_radius,2) - pow(dnl,2));
    float blendFactor = smoothstep(dLightNearest - dz, dLightNearest + dz, distance(in_cameraPosition,alphaPos));
#else
    // x=1 -> transparent object in front else x=0
    // when transparent object is in front then at least 50% of the volume
    // is blended with the transparent color
    float x = float(dCamNearest>distance(in_cameraPosition,alphaPos));
    // occlusion=1 -> the other 50% are also occluded.
    float occlusion = x - (2.0*x - 1.0)*a1/a0;
    // linear blend between unoccluded volume and transparency occluded
    // volume.
    float blendFactor = x*0.5 + occlusion*0.5;
    // apply unoccluded fog
    out_color  = (1.0-blendFactor) * in_lightDiffuse;
    // apply transparency occluded fog using alpha blending between fog
    // and transparency color. Also scale result by alpha inverse.
    out_color += (blendFactor*(1.0-tcolor.a)) *
        (in_lightColor*(1.0-tcolor.a) + tcolor.rgb*tcolor.a);
    // scale by attenuation and exposure factor
    out_color *= exposure * a0;
#endif // 0

#else
    out_color = (exposure * a0) * in_lightDiffuse;
#endif // USE_TBUFFER
}

--------------------------------------
---- Volumetric Fog for point lights.
--------------------------------------

-- volumetric.point.vs
// #undef IS_SPOT_LIGHT
#include shading.deferred.point.vs

-- volumetric.point.fs
// #undef IS_SPOT_LIGHT
#include fog.volumetric.fs

--------------------------------------
---- Volumetric Fog for spot lights.
--------------------------------------

-- volumetric.spot.vs
#define IS_SPOT_LIGHT
#include shading.deferred.spot.vs

-- volumetric.spot.fs
#define IS_SPOT_LIGHT
#include fog.volumetric.fs

