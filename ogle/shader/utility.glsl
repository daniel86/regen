
-- fs.ortho.header
#ifndef IS_VOLUME
    #define vecTex vec2
    #define ivecTex ivec2
    #define toVecTex(v) v.xy
    #define samplerTex sampler2D
    #define fragCoord() gl_FragCoord.xy
    #define ifragCoord() ivec2(fragCoord())
#else
    #define vecTex vec3
    #define ivecTex ivec3
    #define toVecTex(v) v.xyz
    #define samplerTex sampler3D
    #define fragCoord() vec3(gl_FragCoord.xy,f_layer)
    #define ifragCoord() ivec3(fragCoord())
#endif
in vecTex texco;

-- gs.ortho
layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;

in vec4 texco[3];
in vec4 g_pos[3];
in int g_instanceID[3];
// FIXME: name missmatch
out float f_layer;
out float f_texco;

void main()
{
    gl_Layer = g_instanceID[0];
    f_layer = float(g_instanceID[0]) + 0.5;
    gl_Position = g_pos[0]; EmitVertex();
    gl_Position = g_pos[1]; EmitVertex();
    gl_Position = g_pos[2]; EmitVertex();
    EndPrimitive();
}

-- vs.ortho
in vec3 v_pos;
out vec2 texco;
#ifdef IS_VOLUME
out vec4 g_pos;
out int g_instanceID;
#endif

void main()
{
#ifdef IS_VOLUME
    g_pos = vec4(v_pos.xy, 0.0, 1.0);
    g_instanceID = gl_InstanceID;
#endif
    // TODO: volume texco
    texco = 0.5*(v_pos.xy+vec2(1.0));
    gl_Position = vec4(v_pos.xy, 0.0, 1.0);
}

-- fetchNeighbors
#ifndef fetchNeighbors_INCLUDED
#define fetchNeighbors_INCLUDED
#ifndef IS_VOLUME
vec4[4] fetchNeighbors(samplerTex tex, ivecTex pos) {
    vec4[4] neighbors;
    neighbors[NORTH] = texelFetchOffset(tex, pos, 0, ivec2( 0,  1));
    neighbors[SOUTH] = texelFetchOffset(tex, pos, 0, ivec2( 0, -1));
    neighbors[EAST]  = texelFetchOffset(tex, pos, 0, ivec2( 1,  0));
    neighbors[WEST]  = texelFetchOffset(tex, pos, 0, ivec2(-1,  0));
    return neighbors;
}
#else
vec4[6] fetchNeighbors(samplerTex tex, ivecTex pos) {
    vec4[6] neighbors;
    neighbors[NORTH] = texelFetchOffset(tex, pos, 0, ivec3( 0,  1,  0));
    neighbors[SOUTH] = texelFetchOffset(tex, pos, 0, ivec3( 0, -1,  0));
    neighbors[EAST]  = texelFetchOffset(tex, pos, 0, ivec3( 1,  0,  0));
    neighbors[WEST]  = texelFetchOffset(tex, pos, 0, ivec3(-1,  0,  0));
    neighbors[FRONT] = texelFetchOffset(tex, pos, 0, ivec3( 0,  0,  1));
    neighbors[BACK]  = texelFetchOffset(tex, pos, 0, ivec3( 0,  0, -1));
    return neighbors;
}
#endif
#endif // fetchNeighbors_INCLUDED

