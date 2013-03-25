
--------------------------------
--------------------------------
----- Maps font textures to geometry.
----- Font is saved as texture array and can indexed by the characters.
--------------------------------
--------------------------------
-- vs
in vec3 in_pos;
in vec2 in_viewport;

#define HANDLE_IO()

void main() {
    vec2 pos = 2.0*in_pos.xy;
#ifndef USE_NORMALIZED_COORDINATES
    pos.x -= in_viewport.x;
    pos.y += in_viewport.y;
    pos /= in_viewport;
#endif
    // TODO: allow transformation

    gl_Position = vec4(pos, 0.0, 1.0);
    HANDLE_IO(gl_VertexID);
}

-- fs
#include textures.defines
#include textures.input
#include textures.mapToFragmentUnshaded

uniform vec4 in_textColor;
out vec4 out_color;

void main() {
    out_color = in_textColor;
    textureMappingFragmentUnshaded(gl_FragCoord.xyz,out_color);    
}

