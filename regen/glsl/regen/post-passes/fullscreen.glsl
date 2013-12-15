
--------------------------------------
---- Vertex shader for fullscreen rendering pass.
--------------------------------------
-- vs
in vec3 in_pos;
out vec2 out_texco;
#define HANDLE_IO(i)
void main() {
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
    HANDLE_IO(gl_VertexID);
}

-- gs
#if RENDER_LAYER > 1
#extension GL_EXT_geometry_shader4 : enable

layout(triangles) in;
// TODO: use ${RENDER_LAYER}*3
layout(triangle_strip, max_vertices=18) out;

flat out int out_layer;

#define HANDLE_IO(i)

void emitVertex(vec4 posWorld, int index, int layer) {
  gl_Position = posWorld;
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
