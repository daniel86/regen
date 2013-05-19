--------------------------------
--------------------------------
----- Used to update a cube map texture.
----- Bottom face is not updated.
--------------------------------
--------------------------------
-- vs
in vec3 in_pos;
out vec2 out_pos;

void main(void) {
    out_pos = vec2(in_pos.x, -in_pos.y);
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- gs
#extension GL_EXT_geometry_shader4 : enable

layout(triangles) in;
layout(triangle_strip, max_vertices=15) out;

out vec3 out_pos;
in vec2 in_pos[3];

vec3 getCubePoint(vec2 p, int i)
{
    vec3 cubePoints[5] = vec3[](
        vec3( 1.0, p.y,-p.x), // +X
        vec3(-1.0, p.y, p.x), // -X
        vec3( p.x, 1.0,-p.y), // +Y
        vec3( p.x, p.y, 1.0), // +Z
        vec3(-p.x, p.y,-1.0)  // -Z
    );
    return cubePoints[i];
}

void main(void) {
    for(int i=0; i<5; ++i) {
        // select framebuffer layer
        gl_Layer = i + int(i>2);
        
        out_pos = getCubePoint(in_pos[0],i);
        gl_Position = gl_PositionIn[0];
        EmitVertex();
        out_pos = getCubePoint(in_pos[1],i);
        gl_Position = gl_PositionIn[1];
        EmitVertex();
        out_pos = getCubePoint(in_pos[2],i);
        gl_Position = gl_PositionIn[2];
        EmitVertex();
        EndPrimitive();
    }
}
