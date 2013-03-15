
-- vsHeader
in vec3 in_pos;
#ifdef IS_2D_TEXTURE
out vec2 out_texco;
#endif

-- gsHeader
#extension GL_EXT_geometry_shader4 : enable
#extension GL_ARB_gpu_shader5 : enable

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;
#ifdef IS_CUBE_TEXTURE
layout(invocations = 6) in;
#else
layout(invocations = NUM_TEXTURE_LAYERS) in;
#endif

out vec3 out_texco;

-- gsEmit
#ifdef IS_CUBE_TEXTURE
#include utility.computeCubeDirection
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
#ifdef IS_2D_TEXTURE
in vec2 in_texco;
#else
in vec3 in_texco;
#endif

#ifdef IS_ARRAY_TEXTURE
uniform sampler2DArray in_inputTexture;
#elif IS_CUBE_TEXTURE
uniform samplerCube in_inputTexture;
#else // IS_2D_TEXTURE
uniform sampler2D in_inputTexture;
#endif

-- fs
#include sampling.fsHeader
out vec3 output;

void main() {
    vec4 v = texture(in_inputTexture, in_texco);
    output = v.rgb*v.a;
}

-- vs
#include sampling.vsHeader

void main() {
#ifdef IS_2D_TEXTURE
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
#endif
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- gs
#ifndef IS_2D_TEXTURE
#include sampling.gsHeader
#include sampling.gsEmit

void main(void) {
    int layer = gl_InvocationID;
    // select framebuffer layer
    gl_Layer = layer;
    // TODO: allow to skip layers
    emitVertex(gl_PositionIn[0], layer);
    emitVertex(gl_PositionIn[1], layer);
    emitVertex(gl_PositionIn[2], layer);
    EndPrimitive();
}
#endif
