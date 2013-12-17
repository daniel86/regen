
-- vs
#include regen.states.camera.defines

in vec3 in_pos;
#if RENDER_TARGET == 2D
out vec2 out_texco;
#endif

void main() {
#if RENDER_TARGET == 2D
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
#endif
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- gs
#include regen.states.camera.defines
#if RENDER_LAYER > 1
#extension GL_EXT_geometry_shader4 : enable

layout(triangles) in;
// TODO: use ${RENDER_LAYER}*3
layout(triangle_strip, max_vertices=18) out;

out vec3 out_texco;
flat out int out_layer;

#if RENDER_TARGET == CUBE
#include regen.math.computeCubeDirection
#endif

#define HANDLE_IO(i)

void emitVertex(vec4 posWorld, int index, int layer) {

  gl_Position = posWorld;
#if RENDER_TARGET == CUBE
  out_texco = computeCubeDirection(vec2(posWorld.x, -posWorld.y), layer);
#else
  out_texco = vec3(0.5*(gl_Position.xy+vec2(1.0)), layer);
#endif
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

-- fs-texco
#if RENDER_LAYER == 1
in vec2 in_texco;
#else
in vec3 in_texco;
#endif

-- fs
#include regen.states.camera.defines
#include regen.filter.sampling.fs-texco

#if RENDER_TARGET == 2D_ARRAY
uniform sampler2DArray in_inputTexture;
#elif RENDER_TARGET == CUBE
uniform samplerCube in_inputTexture;
#else // RENDER_TARGET == 2D
uniform sampler2D in_inputTexture;
#endif

out vec4 out_color;
void main()
{
    out_color = texture(in_inputTexture, in_texco);
}
