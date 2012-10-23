
-- debugNormal.vs
#version 150
in vec4 vs_Position;
in vec3 vs_nor;
out vec3 gs_nor;
out vec4 gs_pos;
uniform mat4 u_viewProjectionMatrix;
void main()
{
    gs_pos = vs_Position;
    gs_nor = normalize(u_viewProjectionMatrix * vec4(vs_nor,0.0)).xyz;
    gl_Position = vs_Position;
}
-- debugNormal.gs
#version 150
#extension GL_EXT_geometry_shader4 : enable
#ifdef IS_POINT
    layout(points) in;
    layout(line_strip, max_vertices=2) out;
    in vec4 gs_pos[1];
    in vec3 gs_nor[1];
#elif IS_LINE
    layout(lines) in;
    layout(line_strip, max_vertices=4) out;
    in vec4 gs_pos[2];
    in vec3 gs_nor[2];
#elif IS_QUAD
    layout(lines) in;
    layout(line_strip, max_vertices=8) out;
    in vec4 gs_pos[4];
    in vec3 gs_nor[4];
#else
    layout(triangles) in;
    layout(line_strip, max_vertices=6) out;
    in vec4 gs_pos[3];
    in vec3 gs_nor[3];
#endif
void main()
{
    for(int i=0; i< gl_VerticesIn; i++) {
        vec4 posV = gs_pos[i];
        vec3 norV = gs_nor[i];
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

