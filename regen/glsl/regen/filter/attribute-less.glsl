#version 330

uniform vec4 u_pos;

const vec2 pos[] = vec2[4](
    vec2(0.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0)
);

const vec2[] tex = vec2[4](
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0)
);

out vec2 v_texcoord;

void main() {
    vec2 p = pos[gl_VertexID];
    vec2 offset = vec2(u_pos.x * 2.0 - 1.0, u_pos.y * 2.0 - 1.0);
    vec2 scale = vec2(u_pos.z * p.x * 2.0, u_pos.w * p.y * 2.0);
    gl_Position = vec4(offset.x + scale.x,
    offset.y + scale.y,
    0.0, 1.0);
    v_texcoord = tex[gl_VertexID];
}
