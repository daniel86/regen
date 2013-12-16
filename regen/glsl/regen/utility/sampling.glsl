
-- defines
#ifndef RENDER_LAYER
#define RENDER_LAYER 1
#endif
#endif
#ifndef RENDER_TARGET
#define RENDER_TARGET 2D
#endif
#include regen.shading.deferred.defines
#include regen.states.camera.input

-- vsHeader
in vec3 in_pos;
#if RENDER_TARGET == 2D
out vec2 out_texco;
#endif

-- gsHeader
#extension GL_EXT_geometry_shader4 : enable

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;

out vec3 out_texco;

-- gsEmit
#if RENDER_TARGET == CUBE
#include regen.math.computeCubeDirection
void emitVertex(vec4 P, int layer)
{
    gl_Position = P;
    out_texco = computeCubeDirection(vec2(P.x, -P.y), layer);
    EmitVertex();
}
#else
void emitVertex(vec4 P, int layer)
{
    gl_Position = P;
    out_texco = vec3(0.5*(gl_Position.xy+vec2(1.0)), layer);
    EmitVertex();
}
#endif

-- fsHeader
#include regen.utility.sampling.defines
#if RENDER_TARGET == 2D
in vec2 in_texco;
#else
in vec3 in_texco;
#endif

#if RENDER_TARGET == 2D_ARRAY
uniform sampler2DArray in_inputTexture;
#elif RENDER_TARGET == CUBE
uniform samplerCube in_inputTexture;
#else // RENDER_TARGET == 2D
uniform sampler2D in_inputTexture;
#endif

--------------------------------------
--------------------------------------
---- Sample an input texture.
---- Supports cube textures, texture arrays and regular 2D textures.
--------------------------------------
--------------------------------------
-- vs
#include regen.utility.sampling.defines
#include regen.utility.sampling.vsHeader

void main() {
#if RENDER_TARGET == 2D
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
#endif
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- gs
#include regen.utility.sampling.defines
#if RENDER_LAYER > 1
#include regen.utility.sampling.gsHeader
#include regen.utility.sampling.gsEmit

void main(void) {
#for LAYER to ${RENDER_LAYER}
#ifndef SKIP_LAYER${LAYER}
    // select framebuffer layer
    gl_Layer = ${LAYER};
    emitVertex(gl_PositionIn[0], ${LAYER});
    emitVertex(gl_PositionIn[1], ${LAYER});
    emitVertex(gl_PositionIn[2], ${LAYER});
    EndPrimitive();
#endif
#endfor
}
#endif

-- fs
#include regen.utility.sampling.fsHeader
out vec4 out_color;
void main()
{
    out_color = texture(in_inputTexture, in_texco);
}

--------------------------------------
--------------------------------------
---- Sample down an input texture.
---- Supports cube textures, texture arrays and regular 2D textures.
--------------------------------------
--------------------------------------
-- downsample.vs
#include regen.utility.sampling.vs
-- downsample.gs
#include regen.utility.sampling.gs
-- downsample.fs
#include regen.utility.sampling.fs
