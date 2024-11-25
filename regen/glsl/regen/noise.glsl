
-- random
#ifndef REGEN_random_Included
#define2 REGEN_random_Included
// return pseudorandom float with values between 0 and 1.
float random(inout uint seed) {
    seed = (seed * 1103515245u + 12345u);
    return float(seed) / 4294967296.0;
}
// return pseudorandom vec3 with values between -1 and 1.
vec3 random3(inout uint seed) {
    vec3 result;
    seed = (seed * 1103515245u + 12345u); result.x = float(seed);
    seed = (seed * 1103515245u + 12345u); result.y = float(seed);
    seed = (seed * 1103515245u + 12345u); result.z = float(seed);
    return (result / 2147483648.0) - vec3(1,1,1);
}
#endif

-- random2D
#ifndef REGEN_random2D_Included
#define2 REGEN_random2D_Included
#include regen.noise.random2D.a
highp float random (inout vec2 uv) {
    highp float v = random2D(uv);
    uv.x = fract(uv.x + 0.3898);
    uv.y = fract(uv.y + 0.233);
    return v;
}
#endif

-- random2D.a
#ifndef REGEN_random2D_n_Included
#define2 REGEN_random2D_n_Included
float random2D(vec2 uv) {
    return fract(sin(dot(uv, vec2(12.9898,78.233)))*43758.5453123);
}
#endif

-- random2D.b
#ifndef REGEN_random2D_n_Included
#define2 REGEN_random2D_n_Included
highp float random2D(vec2 uv) {
    highp float a = 12.9898;
    highp float b = 78.233;
    highp float c = 43758.5453;
    highp float dt = dot(uv ,vec2(a,b));
    highp float sn = mod(dt,3.14);
    return fract(sin(sn) * c);
}
#endif

-- random2D.c
#ifndef REGEN_random2D_n_Included
#define2 REGEN_random2D_n_Included
const float GOLDEN_RATIO = 1.61803398874989484820459;
float random2D(vec2 xy){
    return fract(tan(distance(xy*GOLDEN_RATIO, xy))*xy.x);
}
#endif

-- variance
#ifndef REGEN_variance_Included
#define2 REGEN_variance_Included
#include regen.noise.random
float variance(float v, inout uint seed) {
    return v*2.0*(random(seed)-0.5);
}
vec2 variance(vec2 v, inout uint seed) {
    return vec2(variance(v.x,seed), variance(v.y,seed));
}
vec3 variance(vec3 v, inout uint seed) {
    return vec3(variance(v.x,seed), variance(v.y,seed), variance(v.z,seed));
}
vec4 variance(vec4 v, inout uint seed) {
    return vec4(variance(v.x,seed), variance(v.y,seed), variance(v.z,seed), variance(v.w,seed));
}
#endif

-- perlin2D
#ifndef REGEN_perlin_Included
#define2 REGEN_perlin_Included
#include regen.noise.random2D
float perlin2D(in float x, in float y, in float wavelength)
{
    float integer_x = x - fract(x);
    float integer_y = y - fract(y);
    float v1 = random2D(vec2(integer_x, integer_y));
    float v2 = random2D(vec2(integer_x+1.0, integer_y));
    float v3 = random2D(vec2(integer_x, integer_y+1.0));
    float v4 = random2D(vec2(integer_x+1.0, integer_y+1.0));

    float fractional_x = x - integer_x;
    float fractional_y = y - integer_y;
    float f1 = smoothstep(0.0, 1.0, fractional_x);
    float f2 = smoothstep(0.0, 1.0, fractional_y);

    return mix(
        mix(v1, v2, f1),
        mix(v3, v4, f1),
        f2);
}
float perlin2D(in vec2 coord, in float wavelength)
{
    return perlin2D(coord.x/wavelength, coord.y/wavelength, wavelength);
}
#endif

-- dots
// @see http://www.science-and-fiction.org/rendering/noise.html
#ifndef REGEN_noise_dots_Included
#define2 REGEN_noise_dots_Included
#include regen.noise.random2D
float dotNoise2D(in float x, in float y, in float fractionalMaxDotSize, in float dDensity)
{
    float integer_x = x - fract(x);
    float integer_y = y - fract(y);
    float fractional_x = x - integer_x;
    float fractional_y = y - integer_y;

    if (random2D(vec2(integer_x+1.0, integer_y +1.0)) > dDensity) {return 0.0;}

    float xoffset = (random2D(vec2(integer_x, integer_y)) -0.5);
    float yoffset = (random2D(vec2(integer_x+1.0, integer_y)) - 0.5);
    float dotSize = 0.5 * fractionalMaxDotSize *
        max(0.25,random2D(vec2(integer_x, integer_y+1.0)));
    vec2 truePos = vec2(
        0.5 + xoffset * (1.0 - 2.0 * dotSize),
        0.5 + yoffset * (1.0 -2.0 * dotSize));
    float distance = length(truePos - vec2(fractional_x, fractional_y));

    return 1.0 - smoothstep(0.3 * dotSize, 1.0* dotSize, distance);
}
float dotNoise2D(in vec2 coord, in float wavelength, in float fractionalMaxDotSize, in float dDensity)
{
    return dotNoise2D(coord.x/wavelength, coord.y/wavelength, fractionalMaxDotSize, dDensity);
}
#endif
