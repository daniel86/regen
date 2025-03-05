
-- vs
in vec3 in_pos;
in vec2 in_viewport;
#ifdef HAS_modelMatrix
uniform mat4 in_modelMatrix;
#endif

#ifdef UV_TO_ARRAY_TEXCO || TIME_TO_ARRAY_TEXCO
in vec2 in_texco0;
out vec3 out_texco0;
const float in_arrayLayer=0.0;
#endif

#define HANDLE_IO()

#ifdef TIME_TO_ARRAY_TEXCO
#include regen.states.textures.timeArrayTransfer
#endif

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
    // TODO: better to handle below cases as texco transfer functions.
    //       but currently transfer functions cannot switch the texco type, they use "inout vec2" as input.
    //       also fragment shader currently enforces the texco type based on the texture type,
    //       this restriction should be removed.
#ifdef UV_TO_ARRAY_TEXCO
    out_texco0 = vec3(in_texco0,in_arrayLayer);
#endif
#ifdef TIME_TO_ARRAY_TEXCO
    #define2 _TEX_ID ${TEX_ID${TIME_TO_ARRAY_TEX}}
    #define2 _TEX_DEPTH ${TEX_DEPTH${_TEX_ID}}
    out_texco0 = timeArrayTransfer(in_texco0, ${_TEX_DEPTH}, TIME_TO_ARRAY_FPS);
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
