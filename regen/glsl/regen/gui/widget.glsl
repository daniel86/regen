
-- vs
in vec3 in_pos;
in vec2 in_viewport;
#ifdef HAS_modelMatrix
uniform mat4 in_modelMatrix;
#endif

#define HANDLE_IO()

void main() {
    vec2 pos = 2.0*in_pos.xy;
#ifndef USE_NORMALIZED_COORDINATES
    pos.x -= in_viewport.x;
    pos.y += in_viewport.y;
#endif
#ifdef HAS_modelMatrix
    pos.x += in_modelMatrix[3].x;
    pos.y -= in_modelMatrix[3].y;
#endif
#ifndef USE_NORMALIZED_COORDINATES
    pos /= in_viewport;
#endif
#ifdef INVERT_Y
    pos.y -= 2.0;
#endif
#ifdef INVERT_X
    pos.x += 2.0;
#endif

    gl_Position = vec4(pos, 0.0, 1.0);

    HANDLE_IO(gl_VertexID);
}

-- fs
#include regen.states.material.input

#include regen.states.textures.defines
#include regen.states.textures.input
#include regen.states.textures.mapToFragmentUnshaded

out vec4 out_color;

void main() {
    out_color = vec4(1.0);
    textureMappingFragmentUnshaded(gl_FragCoord.xyz,out_color);
#ifdef HAS_MATERIAL
    out_color.a *= in_matAlpha;
#endif
}
