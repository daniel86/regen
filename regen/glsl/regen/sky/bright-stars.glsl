
--------------------------------
--------------------------------
----- Visualization of bright stars as sprites.
----- Code based on: https://code.google.com/p/osghimmel/
----- Bright star catalog from: http://tdc-www.harvard.edu/catalogs/bsc5.html
--------------------------------
--------------------------------
-- vs
#include regen.models.mesh.defines
#define _35OVER13PI 0.85698815511020565414014334123662

in vec4 in_pos;
in vec4 in_col0;

out float out_k;
out vec3 out_col;

uniform vec3 in_sunPosition;
uniform mat4 in_equToHorMatrix;
uniform float in_q;

const vec3 in_starColor = vec4(0.66, 0.78, 1.00);
const float in_colorRatio = 0.66;
const float in_glareScale = 1.0;
const float in_apparentMagnitude = 7.0;

#ifdef USE_SCINTILLATION
uniform sampler1D in_noiseTexture;
const float in_scintillation = 0.15;
#endif

#ifndef SKIP_SCATTERING
const float in_scattering = 4.0;
#include regen.sky.utility.scatter
#endif

void main(void) {
  vec4 v = in_equToHorMatrix * vec4(in_pos.xyz,0.0);
  gl_Position = v;
  
  out_k = 0.0;

  float delta_m = pow(2.512, in_apparentMagnitude - in_col0.w);
  float i_t = delta_m * _35OVER13PI;
  i_t *= 4e-7 / (in_q * in_q);  // resolution correlated
  i_t = min(1.167, i_t);  // volume of smoothstep (V_T)
  // Day-Twilight-Night-Intensity Mapping (Butterworth-Filter)
  i_t *= 1.0 / sqrt(1 + pow(in_sunPosition.y + 1.14, 32));
  if(i_t < 0.01) return;
  out_col = vec3(i_t);

#ifndef SKIP_SCATTERING
  out_col -= in_scattering * scatter( acos(v.y) );
#endif
#ifdef USE_SCINTILLATION
  float r = mod(int(in_cmn.w) ^ int(in_pos.w), 251);
  out_col -= in_scintillation * texture( in_noiseTexture, r / 256.0 ).r;
#endif
  out_col *= mix(in_col0.rgb, in_starColor, in_colorRatio);
  out_col = max(vec3(0.0), out_col);
  
  float i_g = pow(2.512, in_apparentMagnitude - (in_col0.w + 0.167)) - 1;
  out_k = max(in_q, sqrt(i_g) * 2e-2 * in_glareScale);
}

-- gs
#include regen.models.mesh.defines
#extension GL_EXT_geometry_shader4 : enable
#define2 __MAX_VERTICES__ ${${RENDER_LAYER}*4}

layout (points) in;
layout(triangle_strip, max_vertices=4) out;

in float in_k[ ];
in vec3 in_col[ ];
out vec3 out_col;
out vec3 out_texco;
flat out int out_layer;

uniform float in_q;

const float surfaceHeight = 0.99;

#include regen.states.camera.transformWorldToScreen
#include regen.sky.utility.computeEyeExtinction

void emitVertex(vec3 posWorld, vec2 texco, int layer) {
  gl_Position = transformWorldToScreen(vec4(posWorld,0.0),layer);
  gl_Position.z = gl_Position.w;
  out_texco.xy = texco;
  EmitVertex();
}
void emitBrightStar(int layer) {
    vec3 p = normalize(gl_PositionIn[0].xyz);

    float ext = computeEyeExtinction(p);
    if(ext <= 0.0) return;
    out_col = in_col[0]*smoothstep(0.0, 0.05, ext);
    out_texco.z = in_k[0] / in_q;
    
    vec3 u = cross(p, vec3(0, 0, 1));
    vec3 v = cross(u, p);
    emitVertex(p - normalize(-u -v) * in_k[0], vec2(-1.0, -1.0), layer);
    emitVertex(p - normalize(-u +v) * in_k[0], vec2(-1.0,  1.0), layer);
    emitVertex(p - normalize(+u -v) * in_k[0], vec2( 1.0, -1.0), layer);
    emitVertex(p - normalize(+u +v) * in_k[0], vec2( 1.0,  1.0), layer);
    EndPrimitive();
}

void main() {
  if(in_k[0] > 0) {
#for LAYER to ${RENDER_LAYER}
#ifndef SKIP_LAYER${LAYER}
#if RENDER_LAYER > 1
    gl_Layer = ${LAYER};
    out_layer = ${LAYER};
#endif
    emitBrightStar(${LAYER});
#endif
#endfor
  }
}

-- fs
#include regen.models.mesh.defines

out vec4 out_color;
in vec3 in_texco;
in vec3 in_col;

const float in_scale = 1.0;
const float in_glareIntensity = 1.0;

void main(void) {
  float l = length(in_texco.xy);
  float t = 1 - smoothstep(0.0, 1.0, l * in_texco.z / in_scale);
  float g = 1 - pow(l, in_glareIntensity);
  out_color = vec4((t > g ? t : g) * in_col, 1.0);
}
