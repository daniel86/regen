
-- gs.header
/*
#ifdef IS_POINT
    layout(points) in;
    #define NUM_IN_VERTICES 1
#elif IS_LINE
    layout(lines) in;
    #define NUM_IN_VERTICES 2
#elif IS_QUAD
    layout(lines) in;
    #define NUM_IN_VERTICES 4
#else
*/
    layout(triangles) in;
    #define NUM_IN_VERTICES 3
//#endif
in vec4 in_pos[3];

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
    #define fragCoord() vec3(gl_FragCoord.xy,in_layer)
    #define ifragCoord() ivec3(fragCoord())
#endif
in vecTex in_texco;

-- gs.ortho
layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;

in vec4 in_texco[3];
in vec4 in_pos[3];
in int in_instanceID[3];
out float out_layer;
out float out_texco;

void main()
{
    gl_Layer = in_instanceID[0];
    out_layer = float(g_instanceID[0]) + 0.5;
    gl_Position = in_pos[0]; EmitVertex();
    gl_Position = in_pos[1]; EmitVertex();
    gl_Position = in_pos[2]; EmitVertex();
    EndPrimitive();
}

-- vs.ortho
in vec3 in_pos;
out vec2 out_texco;
#ifdef IS_VOLUME
out vec4 out_pos;
out int out_instanceID;
#endif

void main()
{
#ifdef IS_VOLUME
    out_pos = vec4(in_pos.xy, 0.0, 1.0);
    out_instanceID = gl_InstanceID;
#endif
    // TODO: volume texco
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- pixelVelocity.fs
in vec2 in_pos0;
in vec2 in_pos1;

uniform uvec2 in_viewport;
uniform float in_deltaT;
#ifdef USE_DEPTH_TEST
uniform sampler2D in_depthTexture;
#endif

out vec2 velocityOutput;

#ifndef DEPTH_BIAS
    // bias used for scene depth buffer access
    #define DEPTH_BIAS 0.1
#endif

void main() {
#ifdef USE_DEPTH_TEST
    // use the fragment coordinate to find the texture coordinates of
    // this fragment in the scene depth buffer.
    // gl_FragCoord.xy is in window space, divided by the buffer size
    // we get the range [0,1] that can be used for texture access.
    vec2 depthTexco = vec2(gl_FragCoord.x/in_viewport.x, gl_FragCoord.y/in_viewport.y);
    // depth at this pixel obtained in main pass.
    // this is non linear depth in the range [0,1].
    float sceneDepth = texture(in_depthTexture, depthTexco).r;
    // do the depth test against gl_FragCoord.z using a bias.
    // bias is used to avoid flickering
    if( gl_FragCoord.z > sceneDepth+DEPTH_BIAS ) { discard; };
#endif
    velocityOutput = (in_pos0 - in_pos1)/in_deltaT;
}

-- pixelVelocity.vs
out vec2 out_pos0;
out vec2 out_pos1;

#ifdef WORLD_SPACE
in vec4 in_posWorld;
in vec4 in_lastPosWorld;
#elif EYE_SPACE
in vec4 in_posEye;
in vec4 in_lastPosEye;
#else
in vec4 in_Position;
in vec4 in_lastPosition;
#endif

void main() {
#ifdef WORLD_SPACE
    out_pos0 = in_posWorld.xy;
    out_pos1 = in_lastPosWorld.xy;
    gl_Position = in_viewProjectionMatrix * in_posWorld;
#elif EYE_SPACE
    out_pos0 = in_posEye.xy;
    out_pos1 = in_lastPosEye.xy;
    gl_Position = in_projectionMatrix * in_posEye;
#else
    out_pos0 = in_Position.xy;
    out_pos1 = in_lastPosition.xy;
    gl_Position = in_Position;
#endif
}

-- picking.gs
#include utility.gs.header

layout(points, max_vertices=1) out;

out int out_pickObjectID;
out int out_pickInstanceID;
out float out_pickDepth;

in int in_instanceID[3];

uniform vec2 in_mousePosition;
uniform vec2 in_viewport;
uniform int in_pickObjectID;

vec2 deviceToScreenSpace(vec3 vertexDS, vec2 screen){
    return (vertexDS.xy*0.5 + vec2(0.5))*screen;
}
float intersectionDepth(vec3 dev0, vec3 dev1, vec2 mouseDev) {
    float dm0 = distance(mouseDev,dev0.xy);
    float dm1 = distance(mouseDev,dev1.xy);
    return (dev0.z*dm1 + dev1.z*dm0)/(dm0+dm1);
}
bool intersectsLine(vec2 win0, vec2 win1, vec2 winMouse, float epsilon) {
    float a = distance(winMouse,win0);
    float b = distance(winMouse,win1);
    float c = distance(win0,win1);
    float a2 = a*a;
    float b2 = b*b;
    float c2 = c*c;
    float ca = (a2 + c2 - b2)/(2.0*c);
    float cb = (b2 + c2 - a2)/(2.0*c);
    return (ca+cb)<=(c+epsilon) && (b2 - cb*cb)<epsilon;
}
vec2 barycentricCoordinate(vec3 dev0, vec3 dev1, vec3 dev2, vec2 mouseDev) {
   vec2 u = dev2.xy - dev0.xy;
   vec2 v = dev1.xy - dev0.xy;
   vec2 r = mouseDev - dev0.xy;
   float d00 = dot(u, u);
   float d01 = dot(u, v);
   float d02 = dot(u, r);
   float d11 = dot(v, v);
   float d12 = dot(v, r);
   float id = 1.0 / (d00 * d11 - d01 * d01);
   float ut = (d11 * d02 - d01 * d12) * id;
   float vt = (d00 * d12 - d01 * d02) * id;
   return vec2(ut, vt);
}
bool isInsideTriangle(vec2 b) {
   return (
       (b.x >= 0.0) &&
       (b.y >= 0.0) &&
       (b.x + b.y <= 1.0)
   );
}
float intersectionDepth(vec3 dev0, vec3 dev1, vec3 dev2, vec2 mouseDev) {
    float dm0 = distance(mouseDev,dev0.xy);
    float dm1 = distance(mouseDev,dev1.xy);
    float dm2 = distance(mouseDev,dev2.xy);
    float dm12 = dm1+dm2;
    return (dm2/dm12)*((dev0.z*dm1 + dev1.z*dm0)/(dm0+dm1)) +
           (dm1/dm12)*((dev0.z*dm2 + dev2.z*dm0)/(dm0+dm2));
}

void main() {
/*
#ifdef IS_POINT
    vec3 dev0 = gl_in[0].gl_Position.xyz/gl_in[0].gl_Position.w;
    vec2 winPos0 = deviceToScreenSpace(dev0, in_viewport);
    float d = distance(winPos0,in_mousePosition);
    if(d<gl_in[0].gl_PointSize) {
        out_pickObjectID = in_pickObjectID;
        out_pickInstanceID = in_instanceID[0];
        out_pickDepth = dev0.z;
        EmitVertex();
        EndPrimitive();
    }
#elif IS_LINE
    vec3 dev0 = gl_in[0].gl_Position.xyz/gl_in[0].gl_Position.w;
    vec3 dev1 = gl_in[1].gl_Position.xyz/gl_in[1].gl_Position.w;
    vec2 win0 = deviceToScreenSpace(dev0, in_viewport);
    vec2 win1 = deviceToScreenSpace(dev1, in_viewport);
    if(intersectsLine(win0,win1,in_mousePosition,gl_in[0].gl_PointSize)) {"
        vec2 mouseDev = (2.0*(in_mousePosition/in_viewport) - vec2(1.0));
        out_pickObjectID = in_pickObjectID;
        out_pickInstanceID = in_instanceID[0];
        out_pickDepth = intersectionDepth(dev0, dev1, mouseDev);
        EmitVertex();
        EndPrimitive();
    }
// #elif IS_QUAD
#else
*/
    vec3 dev0 = gl_in[0].gl_Position.xyz/gl_in[0].gl_Position.w;
    vec3 dev1 = gl_in[1].gl_Position.xyz/gl_in[1].gl_Position.w;
    vec3 dev2 = gl_in[2].gl_Position.xyz/gl_in[2].gl_Position.w;
    vec2 mouseDev = (2.0*(in_mousePosition/in_viewport) - vec2(1.0));
    mouseDev.y *= -1.0;
    vec2 bc = barycentricCoordinate(dev0, dev1, dev2, mouseDev);
    if(isInsideTriangle(bc)) {
        out_pickObjectID = in_pickObjectID;
        out_pickInstanceID = in_instanceID[0];
        out_pickDepth = intersectionDepth(dev0,dev1,dev2,mouseDev);
        EmitVertex();
        EndPrimitive();
    }
//#endif
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

