
-- vs
#undef HAS_LIGHT
#undef HAS_MATERIAL

in vec3 in_pos;
in vec2 in_viewport;

out vec4 out_color;
uniform vec4 in_backgroundColor;
uniform vec4 in_foregroundColor;
uniform bool in_drawBackground;

#define HANDLE_IO()

void main() {
    vec2 pos = 2.0*in_pos.xy;
#ifndef USE_NORMALIZED_COORDINATES
    pos.x -= in_viewport.x;
    pos.y += in_viewport.y;
    pos /= in_viewport;
#endif
    
    if(in_drawBackground && gl_VertexID<4) {
        out_color = in_backgroundColor;
    }
    else {
        out_color = in_foregroundColor;
    }

    gl_Position = vec4(pos, 0.0, 1.0);

    HANDLE_IO(gl_VertexID);
}

-- fs
#include textures.defines
#undef HAS_LIGHT
#undef HAS_MATERIAL

#include textures.input

#include textures.mapToFragment

in vec4 in_color;
uniform vec4 in_backgroundColor;
uniform vec4 in_foregroundColor;

out vec4 output;

void main() {
    float A = 0.0;
    vec3 N = vec3(0.0);
    output = in_color;
    if(in_color == in_foregroundColor) {
        textureMappingFragment(gl_FragCoord.xyz,N,output,A);
    }
}

