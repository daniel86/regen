
-- vs
#include regen.models.sky-box.vs_include
#if RENDER_LAYER == 1
void main() {
    emitVertex(in_pos.xyz, gl_VertexID, 0);
}
#else
#define HANDLE_IO(i)
void main() {
    gl_Position = vec4(in_pos,0.0);
    HANDLE_IO(gl_VertexID);
}
#endif
-- tcs
#include regen.models.sky-box.tcs
-- tes
#include regen.models.sky-box.tes
-- gs
#if RENDER_LAYER > 1
#include regen.models.sky-box.gs_include
void main() {
    #for LAYER to ${RENDER_LAYER}
        #ifndef SKIP_LAYER${LAYER}
    gl_Layer = ${LAYER};
    out_layer = ${LAYER};
    emitStarVertex(gl_in[0].gl_Position.xyz, 0, ${LAYER}); EmitVertex();
    emitStarVertex(gl_in[1].gl_Position.xyz, 1, ${LAYER}); EmitVertex();
    emitStarVertex(gl_in[2].gl_Position.xyz, 2, ${LAYER}); EmitVertex();
    EndPrimitive();
        #endif // SKIP_LAYER
    #endfor
}
#endif
-- fs
out vec4 out_color;
void main(void) {
    out_color = vec4(0.0, 0.0, 0.0, 1.0);
}
