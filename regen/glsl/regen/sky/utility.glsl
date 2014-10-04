
-- quad-layer.vs
in vec3 in_pos;

void main() {
  gl_Position = vec4(in_pos.xy,1.0,1.0);
}

-- belowHorizon
#ifndef __belowHorizon_Included_
#define2 __belowHorizon_Included_
float tAtm(vec3 ray) {
  float r = in_cmn.y + in_cmn.x;
  vec3 x = vec3(0.0, 0.0, r);
  vec3 v = normalize(ray);
  float mu = dot(x, v) / r;
  return r * mu - sqrt(r * r * (mu * mu - 1.0) + in_cmn.y * in_cmn.y);
}
bool belowHorizon(vec3 ray) {
  return (ray.z>0.0) && (tAtm(ray)>0.0);
}
#endif

-- scatter
#ifndef __scatter_Included_
#define2 __scatter_Included_
const vec3 in_lambda = vec3(0.52, 1.22, 2.98);

float optical(const float theta) {
  float sin_theta = sin(theta);
  float s = -sin(asin((in_cmn[1] + in_cmn[0]) / in_cmn[2] * sin_theta) - theta);
  s *= in_cmn[2];
  s /= sin_theta;
  s /= in_cmn[2] - in_cmn[0];
  return s;
}
// theta is the angle between ray and zenith ~ probably acos(ray.z)
vec3 scatter(const float theta) {
  return in_lambda * optical(theta);
}
#endif

-- noise2
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

-- pseudo_rand
float pseudo_rand(const vec2 i, const int seed) {
  int i1 = int(i.x + i.y * 1733);
  i1 = (i1 << 7) ^ i1 + seed;  // seed
  int i2 = int(i.y + i.x * 1103);
  i2 = (i2 << 7) ^ i2 + seed;  // seed
  i1 ^= i2;
  return 1.0 - float((i1 * (i1 * i1 * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0;
}

-- dither
#include regen.sky.utility.pseudo_rand
vec4 dither(float multiplier, int seed) {
  float r = pseudo_rand(gl_FragCoord.xy, seed);
  uvec4 v = uint(r * 3571) * uvec4(67, 89, 23, 71);
  // A ditheringMultiplier of 1 will add frame to frame coherent noise for each pixel of about +-1.
  // The average brightness of the rendering will roughly remain unchanged.
  return (vec4(v % uvec4(853)) - 241 - multiplier * 1.41) * 0.00001 * multiplier;
}

-- hband
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

-- fakeSun
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
  return vec4(coeffs.rgb * s, coeffs.a);\n"
}

-- fade
float fade(const float t) {
  return t * t * t * (t * (t * 6 - 15) + 10);
}
