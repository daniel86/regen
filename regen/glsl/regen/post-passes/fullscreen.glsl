
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
