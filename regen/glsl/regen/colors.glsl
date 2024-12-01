
-- tonemap
#include regen.colors.tonemap1
#include regen.colors.tonemap3

-- tonemap1
#ifndef REGEN_TONEMAP1
#define REGEN_TONEMAP1
#if TONEMAP_MODE == reinhard
    #include regen.colors.tonemap1.reinhard
#elif TONEMAP_MODE == reinhard2
    #include regen.colors.tonemap1.reinhard2
#elif TONEMAP_MODE == aces
    #include regen.colors.tonemap1.aces
#elif TONEMAP_MODE == uncharted
    #include regen.colors.tonemap1.uncharted
#else
    #include regen.colors.tonemap1.unreal
#endif
#endif

-- tonemap3
#ifndef REGEN_TONEMAP3
#define REGEN_TONEMAP3
#if TONEMAP_MODE == reinhard
    #include regen.colors.tonemap3.reinhard
#elif TONEMAP_MODE == reinhard2
    #include regen.colors.tonemap3.reinhard2
#elif TONEMAP_MODE == aces
    #include regen.colors.tonemap3.aces
#elif TONEMAP_MODE == uncharted
    #include regen.colors.tonemap3.uncharted
#else
    #include regen.colors.tonemap3.unreal
#endif

-- tonemap1.reinhard
#ifndef TONEMAP1_REINHARD1
#define TONEMAP1_REINHARD1
float tonemap(float v) {
    return v / (1.0 + v);
}
#endif

-- tonemap3.reinhard
#ifndef TONEMAP3_REINHARD1
#define TONEMAP3_REINHARD1
vec3 tonemap(vec3 v) {
    return v / (1.0 + v);
}
#endif

-- tonemap1.reinhard2
#ifndef TONEMAP1_REINHARD2
#define TONEMAP1_REINHARD2
const float in_maxWhite = 4.0;
float tonemap(float v) {
    float numerator = v * (1.0f + (v / (in_maxWhite * in_maxWhite)));
    return numerator / (1.0f + v);
}
#endif

-- tonemap3.reinhard2
#ifndef TONEMAP3_REINHARD2
#define TONEMAP3_REINHARD2
const float in_maxWhite = 4.0;
vec3 tonemap(vec3 v) {
    vec3 numerator = v * (1.0f + (v / (in_maxWhite * in_maxWhite)));
    return numerator / (1.0f + v);
}
#endif

-- tonemap1.unreal
#ifndef TONEMAP1_UNREAL
#define TONEMAP1_UNREAL
float tonemap(float x) {
    return x / (x + 0.155) * 1.019;
}
#endif

-- tonemap3.unreal
#ifndef TONEMAP3_UNREAL
#define TONEMAP3_UNREAL
vec3 tonemap(vec3 x) {
    return x / (x + 0.155) * 1.019;
}
#endif

-- tonemap1.aces
#ifndef TONEMAP1_ACES
#define TONEMAP1_ACES
float tonemap(float x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}
#endif

-- tonemap3.aces
#ifndef TONEMAP3_ACES
#define TONEMAP3_ACES
vec3 tonemap(vec3 x) {
    const vec3 a = vec3(2.51);
    const vec3 b = vec3(0.03);
    const vec3 c = vec3(2.43);
    const vec3 d = vec3(0.59);
    const vec3 e = vec3(0.14);
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}
#endif

-- tonemap1.uncharted
#ifndef TONEMAP1_UNCHARTED
#define TONEMAP1_UNCHARTED
float uncharted2(float x) {
  float A = 0.15;
  float B = 0.50;
  float C = 0.10;
  float D = 0.20;
  float E = 0.02;
  float F = 0.30;
  float W = 11.2;
  return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float tonemap(float color) {
  const float W = 11.2;
  const float exposureBias = 2.0;
  float curr = uncharted2(in_exposure * color);
  float whiteScale = 1.0 / uncharted2(W);
  return curr * whiteScale;
}
#endif

-- tonemap3.uncharted
#ifndef TONEMAP3_UNCHARTED
#define TONEMAP3_UNCHARTED
vec3 uncharted2(vec3 x) {
  vec3 A = vec3(0.15);
  vec3 B = vec3(0.50);
  vec3 C = vec3(0.10);
  vec3 D = vec3(0.20);
  vec3 E = vec3(0.02);
  vec3 F = vec3(0.30);
  vec3 W = vec3(11.2);
  return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 tonemap(vec3 color) {
  const vec3 W = vec3(11.2);
  const float exposureBias = 2.0;
  vec3 curr = uncharted2(in_exposure * color);
  vec3 whiteScale = 1.0 / uncharted2(W);
  return curr * whiteScale;
}
#endif
