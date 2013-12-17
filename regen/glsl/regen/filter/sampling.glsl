
-- computeTexco
#ifndef __computeTexco_Included__
#define2 __computeTexco_Included__
#if RENDER_LAYER > 1
flat in int in_layer;
#endif
#if RENDER_TARGET == CUBE
#include regen.math.computeCubeDirection
vec3 computeTexco(vec2 texco_2D) {
  return computeCubeDirection(vec2(2,-2)*texco_2D + vec2(-1,1),in_layer);
}
#define vecTexco vec3
#elif RENDER_LAYER > 1
vec3 computeTexco(vec2 texco_2D) {
  return vec3(texco_2D,in_layer);
}
#define vecTexco vec3
#else
#define computeTexco(texco_2D) texco_2D
#define vecTexco vec2
#endif
#endif

-- vs
in vec3 in_pos;

void main() {
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- gs
// TODO: redundant, nothing special done here
#include regen.states.camera.defines
#if RENDER_LAYER > 1
#extension GL_EXT_geometry_shader4 : enable
#define2 __MAX_VERTICES__ ${${RENDER_LAYER}*3}

layout(triangles) in;
layout(triangle_strip, max_vertices=${__MAX_VERTICES__}) out;

flat out int out_layer;

#define HANDLE_IO(i)

void emitVertex(vec4 pos, int index, int layer) {

  gl_Position = pos;
  HANDLE_IO(index);
  EmitVertex();
}

void main() {
#for LAYER to ${RENDER_LAYER}
#ifndef SKIP_LAYER${LAYER}
  gl_Layer = ${LAYER};
  out_layer = ${LAYER};
  emitVertex(gl_PositionIn[0], 0, ${LAYER});
  emitVertex(gl_PositionIn[1], 1, ${LAYER});
  emitVertex(gl_PositionIn[2], 2, ${LAYER});
  EndPrimitive();
#endif
#endfor
}
#endif

-- fs
#include regen.states.camera.defines

out vec4 out_color;

uniform vec2 in_inverseViewport;
#if RENDER_TARGET == 2D_ARRAY
uniform sampler2DArray in_inputTexture;
#elif RENDER_TARGET == CUBE
uniform samplerCube in_inputTexture;
#else // RENDER_TARGET == 2D
uniform sampler2D in_inputTexture;
#endif
#include regen.filter.sampling.computeTexco

void main()
{
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    out_color = texture(in_inputTexture, computeTexco(texco_2D));
}
