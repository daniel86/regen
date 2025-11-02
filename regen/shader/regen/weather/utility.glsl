
-- belowHorizon
#ifndef __belowHorizon_Included_
#define2 __belowHorizon_Included_
float tAtm(vec3 ray) {
  float r = in_cmn.y + in_cmn.x;
  vec3 x = vec3(0.0, r, 0.0);
  vec3 v = normalize(ray);
  float mu = dot(x, v) / r;
  return r * mu - sqrt(r * r * (mu * mu - 1.0) + in_cmn.y * in_cmn.y);
}
bool belowHorizon(vec3 ray) {
  return ray.y<0.0 || tAtm(ray)<0.0;
}
#endif

-- scatter
#ifndef __scatter_Included_
#define2 __scatter_Included_
const vec3 in_lambda = vec3(0.52, 1.22, 2.98);

float optical(const float theta) {
  float sin_theta = sin(theta);
  float s = -sin(asin((in_cmn.y + in_cmn.x) / in_cmn.z * sin_theta) - theta);
  s *= in_cmn.z;
  s /= sin_theta;
  s /= in_cmn.z - in_cmn.x;
  return s;
}
// theta is the angle between ray and zenith ~ probably acos(ray.z)
vec3 scatter(const float theta) {
  return in_lambda * optical(theta);
}
#endif

-- noise2
#ifndef __noise2_INCLUDED
#define2 __noise2_INCLUDED
float noise2(const sampler2D perm, const vec2 st, const float fade) {
  const float o  = 1.0 / %SIZE%;
  vec2 i = o * floor(st);
  vec2 f = fract(st);
  
  vec2 AA = texture2D(perm, i).xy               * 256.0 - 1.0;
  vec2 BA = texture2D(perm, i + vec2( o, 0)).xy * 256.0 - 1.0;
  vec2 AB = texture2D(perm, i + vec2( 0, o)).xy * 256.0 - 1.0;
  vec2 BB = texture2D(perm, i + vec2( o, o)).xy * 256.0 - 1.0;
  
  float dAA = dot(AA, f              );
  float dBA = dot(BA, f - vec2( 1, 0));
  float dAB = dot(AB, f - vec2( 0, 1));
  float dBB = dot(BB, f - vec2( 1, 1));
  vec2 t = mix(vec2(dAA, dAB), vec2(dBA, dBB), fade(f.x));
  return mix(t.x, t.y, fade(f.y));
}
#endif

-- pseudo_rand
#ifndef __pseudo_rand_INCLUDED
#define2 __pseudo_rand_INCLUDED
float pseudo_rand(const vec2 i, const int seed) {
  int i1 = int(i.x + i.y * 1733);
  i1 = (i1 << 7) ^ i1 + seed;  // seed
  int i2 = int(i.y + i.x * 1103);
  i2 = (i2 << 7) ^ i2 + seed;  // seed
  i1 ^= i2;
  return 1.0 - float((i1 * (i1 * i1 * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0;
}
#endif

-- dither
#ifndef __dither_INCLUDED
#define2 __dither_INCLUDED
#include regen.weather.utility.pseudo_rand
vec4 dither(float multiplier, int seed) {
  float r = pseudo_rand(gl_FragCoord.xy, seed);
  uvec4 v = uint(r * 3571) * uvec4(67, 89, 23, 71);
  // A ditheringMultiplier of 1 will add frame to frame coherent noise for each pixel of about +-1.
  // The average brightness of the rendering will roughly remain unchanged.
  return (vec4(v % uvec4(853)) - 241 - multiplier * 1.41) * 0.00001 * multiplier;
}
#endif

-- hband
#ifndef __hband_INCLUDED
#define2 __hband_INCLUDED
vec4 hband(const float z,
    const float scale,
    const float width,
    const float offset,
    const vec4 color,
    const vec4 background,
    vec4 fc /* fragment color */)
{
  fc = mix(fc, background, step(z, offset));
  float b = abs(z - offset) / scale;
  b = smoothstep(width, 1.0, b);
  return blend_normal(color, fc, b);
}
#endif

-- fakeSun
#ifndef __fakeSun_INCLUDED
#define2 __fakeSun_INCLUDED
vec4 fakeSun(const vec3 eye,
    const vec3 sun,
    const vec4 coeffs,
    const float scale,
    const float alpha)
{
  vec3 fix = normalize(eye.xyz);
  float s = scale * 2.0 / length(normalize(sun) - eye);
  s *= alpha * 0.1 + 0.2;            // Reduce suns' size on low alpha.
  s *= clamp(eye.z + 0.1, 0.0, 0.1); // Disappear in lower hemisphere.
  s  = clamp(clamp(s, 0.0, 2.0) - (1.0 - alpha) * 2.0, 0.0, 2.0);
  return vec4(coeffs.rgb * s, coeffs.a);
}
#endif

-- sunIntensity
#ifndef __sunIntensity_INCLUDED
#define2 __sunIntensity_INCLUDED
float sunIntensity() {
    // Day-Twilight-Night-Intensity Mapping (Butterworth-Filter)
    return 1.0 / sqrt(1 + pow(in_lightDirection_Sun.y + 1.14, 32));
}
#endif

-- fade
#ifndef __fade_float_INCLUDED
#define2 __fade_float_INCLUDED
float fade(const float t) {
  return t * t * t * (t * (t * 6 - 15) + 10);
}
#endif


-- computeHorizonExtinction
#ifndef __computeHorizonExtinction_vec3_vec3_float_INCLUDED
#define2 __computeHorizonExtinction_vec3_vec3_float_INCLUDED
float computeHorizonExtinction(vec3 position, vec3 dir, float radius)
{
    float u = dot(dir, -position);
    if(u<0.0){
        return 1.0;
    }
    vec3 near = position + u*dir;
    if(length(near) < radius){
        return 0.0;
    }
    else{
        vec3 v2 = normalize(near)*radius - position;
        float diff = acos(dot(normalize(v2), dir));
        return smoothstep(0.0, 1.0, pow(diff*2.0, 3.0));
    }
}
#endif

-- computeEyeExtinction
#ifndef __computeEyeExtinction_vec3_INCLUDED
#define2 __computeEyeExtinction_vec3_INCLUDED
#include regen.weather.utility.computeHorizonExtinction
float computeEyeExtinction(vec3 eyedir) {
    vec3 eyePosition = vec3(0.0, in_surfaceHeight, 0.0);
    return computeHorizonExtinction(eyePosition, eyedir, in_surfaceHeight-0.15);
}
#endif

-- computeAtmosphericDepth
#ifndef __computeAtmosphericDepth_vec3_vec3__INCLUDED
#define2 __computeAtmosphericDepth_vec3_vec3__INCLUDED
float computeAtmosphericDepth(vec3 position, vec3 dir) {
    float a = dot(dir, dir);
    float b = 2.0*dot(dir, position);
    float c = dot(position, position)-1.0;
    float det = b*b-4.0*a*c;
    float detSqrt = sqrt(det);
    float q = (-b - detSqrt)/2.0;
    float t1 = c/q;
    return t1;
}
#endif

-- computeEyeDepth
#ifndef computeEyeDepth_vec3__INCLUDED
#define2 computeEyeDepth_vec3__INCLUDED
#include regen.weather.utility.computeAtmosphericDepth
float computeEyeDepth(vec3 eyedir) {
    vec3 eyePosition = vec3(0.0, in_surfaceHeight, 0.0);
    return computeAtmosphericDepth(eyePosition, eyedir);
}
#endif

-- phase
#ifndef __phase_float_float__INCLUDED
#define2 __phase_float_float__INCLUDED
float phase(float alpha, float g) {
    float gg = g*g;
    float a = 3.0*(1.0-gg);
    float b = 2.0*(2.0+gg);
    float c = 1.0+alpha*alpha;
    float d = pow(1.0+gg-2.0*g*alpha, 1.5);
    return (a/b)*(c/d);
}
#endif

-- absorb
#ifndef __absorb_float_vec3_float__INCLUDED
#define2 __absorb_float_vec3_float__INCLUDED
vec3 absorb(float dist, vec3 color, float factor)
{
    return color-color*pow(in_skyAbsorption, vec3(factor/dist));
}
#endif
