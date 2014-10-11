
-- layerIntersectionOrDiscard
float layerIntersectionOrDiscard(const vec3 d, const float altitude) {
  vec3  o = vec3(0.0, in_cmn[1] + in_cmn[0], 0.0);
  float r = in_cmn[1] + altitude;
  // for now, ignore if altitude is above cloud layer
  if(o.y > r) discard;
  
  float a = dot(d, d);
  float b = 2 * dot(d, o);
  float c = dot(o, o) - r * r;
  float B = b * b - 4 * a * c;
  if(B < 0) discard;
  B = sqrt(B);
  return (-b + B) * 0.5 / a;
}

--------------------
// Intersection of view ray (d) with a sphere of radius = mean earth
// radius + altitude (altitude). Support is only for rays starting
// below the cloud layer (o must be inside the sphere...).
// (http://wiki.cgsociety.org/index.php/Ray_Sphere_Intersection) and
// (http://www.siggraph.org/education/materials/HyperGraph/raytrace/rtinter1.htm)
-- layerIntersection
bool layerIntersection(const vec3 d, const vec3 o, const float altitude, out float t)
{
  float r = in_cmn[1] + altitude;
  // for now, ignore if altitude is above cloud layer
  if(o.y > r) return false;
  
  float a = dot(d, d);
  float b = 2 * dot(d, o);
  float c = dot(o, o) - r * r;
  float B = b * b - 4 * a * c;
  B = sqrt(B);
  
  float q;
  if(b < 0) q = (-b - B) * 0.5;
  else      q = (-b + B) * 0.5;
  
  float t0 = q / a;
  float t1 = c / q;
  if(t0 > t1) {
    q  = t0;
    t0 = t1;
    t1 = q;
  }
  if(t1 < 0) return false;
  
  t = t0 < 0 ? t1 : t0;
  return true;
}

-- T
float T(sampler2D tex, vec2 uv) {
  return texture(tex, uv * in_scale).r;
}
float T(sampler2D tex, vec3 stu) {
  float m = 2.0 * (1.0 + stu.y);
  vec2 uv = vec2(stu.x / m + 0.5, stu.z / m + 0.5);
  return T(tex,uv);
}

--------------------------------------------------------
--------------------------------------------------------
--------------------------------------------------------
// Depth(osg::Depth::LEQUAL, 1.0, 1.0)
// BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
-- cloud-layer.vs
#include regen.models.mesh.defines

in vec3 in_pos;
#if RENDER_LAYER <= 1
out vec4 out_ray;
#endif

#include regen.states.camera.input

void main() {
    vec4 p = vec4(in_pos.xy, 0.0, 1.0);
    gl_Position = p;
#if RENDER_LAYER <= 1
    out_ray = in_inverseProjectionMatrix * p * in_viewMatrix;
#endif
}

-- cloud-layer.gs
#include regen.states.camera.defines
#include regen.defines.all
#if RENDER_LAYER > 1
#extension GL_EXT_geometry_shader4 : enable
#define2 __MAX_VERTICES__ ${${RENDER_LAYER}*3}

layout(triangles) in;
layout(triangle_strip, max_vertices=${__MAX_VERTICES__}) out;

out vec4 out_ray;
flat out int out_layer;

#include regen.states.camera.input
#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen

#define HANDLE_IO(i)

void emitVertex(vec4 posWorld, int index, int layer) {
  out_ray = __PROJ_INV__(layer) * posWorld * __VIEW__(layer);
  gl_Position = posWorld;
  HANDLE_IO(index);
  EmitVertex();
}

void main() {
#for LAYER to ${RENDER_LAYER}
#ifndef SKIP_LAYER${LAYER}
  // select framebuffer layer
  gl_Layer = ${LAYER};
  out_layer = ${LAYER};
  emitVertex(gl_PositionIn[0], 0, ${LAYER});
  emitVertex(gl_PositionIn[1], 1, ${LAYER});
  emitVertex(gl_PositionIn[2], 2, ${LAYER});
  EndPrimitive();
#endif // SKIP_LAYER
#endfor
}
#endif

-- cloud-layer.fs
out vec4 out_color;
in vec4 in_ray;

uniform sampler2D in_cloudTexture;

uniform vec3 in_sunPosition;

const float in_altitude = 2.0;
const float in_offset = -0.5;
const float in_thickness = 3.0;
const vec2 in_scale = vec2(128.0);

#ifdef USE_SCATTER
const vec3 in_tcolor = vec3(1.0);
const vec3 in_bcolor = vec3(1.0);

const float in_sSteps = 128;
const float in_sRange =  8;
const int in_dSteps = 32;
const float in_dRange = 1.9;
#else
const vec3 in_color = vec3(1.0);
#endif

#include regen.sky.utility.belowHorizon
#include regen.sky.clouds.layerIntersection
#include regen.sky.clouds.T
#include regen.states.camera.transformTexcoToWorld

#ifdef USE_SCATTER
float density(in vec3 stu0, in vec3 sun, in float aa0) {
    float iSteps = 1.0 / (in_dSteps - 1);
    float iRange = in_dRange * in_thickness * iSteps;

    vec3 stu1 = stu0 + sun * in_dRange * in_thickness;
    vec3 Dstu = (stu1 - stu0) * iSteps;
    vec3 stu  = stu0;

    float d = 0.0;
    float a1 = in_thickness + in_offset;
    for(int i = 0; i < in_dSteps; ++i) {
        float t = T(in_cloudTexture,stu);
        float a = aa0 + i * iRange;
        if(a > t * in_offset && a < t * a1) d += iSteps;
        stu += Dstu;
    }

    return d;
}

vec2 scatter(in vec3 eye, in vec3 sun) {
    vec3 o0 = vec3(0, in_cmn[1] + in_cmn[0], 0);
    // check if intersects with lower cloud sphere    
    float t0, t1;
    float a1 = in_thickness + in_offset;

    if(!layerIntersection(eye, o0, in_altitude + in_offset, t0) ||
       !layerIntersection(eye, o0, in_altitude + a1, t1)) {
        return vec2(0.0);
    }

    vec3 stu0 = o0 + t0 * eye;
    vec3 stu1 = o0 + t1 * eye;
    float iSteps = 1.0 / (in_sSteps - 1);
    vec3 Dstu = (stu1 - stu0) / (in_sSteps - 1);
    vec3 stu  = stu0;
    float Da = in_thickness * iSteps;

    vec2 sd = vec2(0.0);
    for(int i = 0; i < in_sSteps; ++i) {
        float t = T(in_cloudTexture,stu);
        float a = in_offset + i * Da;
        if(a > t * in_offset && a < t * a1) {
            sd.x += density(stu, sun, a);
            ++sd.y;
        }
        if(sd.y >= in_sRange) break;
        stu += Dstu;
    }
    sd.x /= in_sRange;
    sd.y /= in_sRange;

    return sd;
}
#endif

void main() {
  vec3 eye = normalize(in_ray.xyz);
  if(belowHorizon(eye)) discard;
  
  float t;
  vec3 o = vec3(0, in_cmn[1] + in_cmn[0], 0);
  layerIntersection(eye, o, in_altitude, t);

#ifdef USE_SCATTER
  vec2 sd = scatter(eye, normalize(in_sunPosition));
  sd.y *= (1.0 - pow(t, 0.8) * 12e-3);
  
  out_color = vec4(mix(in_tcolor, in_bcolor, sd.x) * (1 - sd.x), sd.y);
#else
  out_color = vec4(in_color, T(in_cloudTexture, o + t * eye));
#endif
}

--------------------------------------------------------
--------------------------------------------------------
--------------------------------------------------------
// MIN_FILTER=LINEAR, MAG_FILTER=LINEAR
// internal:GL_LUMINANCE16F_ARB, format:GL_LUMINANCE
// clear color: 0.0f, 0.0f, 0.0f, 1.0f
-- pre-noise.vs
in vec3 in_pos;
void main() {
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}
-- pre-noise.fs
out vec4 out_color;

uniform sampler3D in_noise0;
uniform sampler3D in_noise1;
uniform sampler3D in_noise2;
uniform sampler3D in_noise3;

const float in_time = 0.0;
const float in_coverage = 0.2;
const float in_sharpness = 0.5;
const float in_change = 0.1;
const vec2 in_wind = vec2(0.0);

void main() {
   vec2 uv = gl_FragCoord.xy*in_inverseViewport;
   float t = in_time * 3600.0;
   vec2 m = t * in_wind;
   t *= in_change;

   float n = 0;
   n += 1.00000 * texture(in_noise0, vec3(uv     + m * 0.18, t * 0.01)).r;
   n += 0.50000 * texture(in_noise1, vec3(uv     + m * 0.16, t * 0.02)).r;
   n += 0.25000 * texture(in_noise2, vec3(uv     + m * 0.14, t * 0.04)).r;
   n += 0.12500 * texture(in_noise3, vec3(uv     + m * 0.12, t * 0.08)).r;
   n += 0.06750 * texture(in_noise3, vec3(uv * 2 + m * 0.10, t * 0.16)).r;
   n *= 0.76;
   n = n - 1 + in_coverage;
   n /= in_coverage;
   n = max(0.0, n);
   n = pow(n, 1.0 - in_sharpness);

   out_color = vec4(n);
}
