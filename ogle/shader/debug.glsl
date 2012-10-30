
-- debugNormal.vs
#version 150
in vec4 in_Position;
in vec3 in_norWorld;
out vec3 out_nor;
out vec4 out_pos;
uniform mat4 u_viewProjectionMatrix;
void main()
{
    out_pos = vs_Position;
    out_nor = normalize(u_viewProjectionMatrix * vec4(in_norWorld,0.0)).xyz;
    gl_Position = in_Position;
}
-- debugNormal.gs
#version 150
#extension GL_EXT_geometry_shader4 : enable
#ifdef IS_POINT
    #define GS_NUM_VERTICES 1
    layout(points) in;
    layout(line_strip, max_vertices=2) out;
#elif IS_LINE
    #define GS_NUM_VERTICES 2
    layout(lines) in;
    layout(line_strip, max_vertices=4) out;
#elif IS_QUAD
    #define GS_NUM_VERTICES 4
    layout(lines) in;
    layout(line_strip, max_vertices=8) out;
#else
    #define GS_NUM_VERTICES 3
    layout(triangles) in;
    layout(line_strip, max_vertices=6) out;
#endif

in vec4 in_pos[GS_NUM_VERTICES];
in vec3 in_nor[GS_NUM_VERTICES];

void main()
{
    for(int i=0; i< gl_VerticesIn; i++) {
        vec4 posV = in_pos[i];
        vec3 norV = in_nor[i];
        gl_Position = posV;
        EmitVertex();
        gl_Position = posV + vec4(norV,0) * 0.1;
        EmitVertex();
        EndPrimitive();
    }
}
-- debugNormal.fs
#version 150
out vec4 defaultColorOutput;
void main()
{
    defaultColorOutput = vec4(1.0,1.0,0.0,1.0);
}

