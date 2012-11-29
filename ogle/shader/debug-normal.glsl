/**
 * Supposed to be used in combination with transform feedback
 * on gl_Position and norWorld.
 **/

-- vs
#version 150

out vec3 out_nor;
out vec4 out_pos;

in vec4 in_Position;
in vec3 in_norWorld;

uniform mat4 u_viewProjectionMatrix;

void main()
{
    out_pos = vs_Position;
    out_nor = normalize(u_viewProjectionMatrix * vec4(in_norWorld,0.0)).xyz;
    gl_Position = in_Position;
}

-- gs
#include geometry-shader.defines
#version 330
#extension GL_EXT_geometry_shader4 : enable

layout(GS_INPUT_PRIMITIVE) in;
layout(line_strip,
#if GS_NUM_VERTICES==1
       max_vertices=2
#elif GS_NUM_VERTICES==2
       max_vertices=4
#else
       max_vertices=8
#endif
) out;

in vec4 in_pos[GS_NUM_VERTICES];
in vec3 in_nor[GS_NUM_VERTICES];

void main()
{
    for(int i=0; i< gl_VerticesIn; i++) {
        gl_Position = in_pos[i];
        EmitVertex();
        gl_Position = in_pos[i] + vec4(in_nor[i],0) * 0.1;
        EmitVertex();
        EndPrimitive();
    }
}

-- fs
#version 330

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_specular;
layout(location = 2) out vec4 out_norWorld;
layout(location = 3) out vec3 out_posWorld;

void main()
{
    out_color = vec4(1.0,1.0,0.0,1.0);
    out_specular = vec4(0.0);
    out_norWorld = vec4(0.0);
    out_posWorld = vec3(0.0);
}

