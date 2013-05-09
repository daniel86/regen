-- gs
#version 150

layout(triangles) in;
layout(points, max_vertices=1) out;

// the picker output
out int out_pickObjectID;
out int out_pickInstanceID;
out float out_pickDepth;
// pretend to be fragment shader for name matching.
in int fs_instanceID[3];

// camera input
uniform mat4 in_inverseProjectionMatrix;
uniform mat4 in_projectionMatrix;
// mouse ray intersecting the view frustum in view space.
// in ndc the ray starts at (mx,my,0) and ends at (mx,my,-1)
uniform vec3 in_mousePosVS;
uniform vec3 in_mouseDirVS;
uniform vec2 in_mouseTexco;
// mesh id
uniform int in_pickObjectID;
// scene depth
// TODO: drop in_depthTexture input ?
uniform sampler2D in_depthTexture;

void main()
{
    vec3 a = (in_inverseProjectionMatrix * gl_in[0].gl_Position).xyz;
    vec3 b = (in_inverseProjectionMatrix * gl_in[1].gl_Position).xyz;
    vec3 c = (in_inverseProjectionMatrix * gl_in[2].gl_Position).xyz;
    // Compute barycentric coordinates
    vec3 v0 = in_mousePosVS - a;
    vec3 v1 = b - a;
    vec3 v2 = c - a;
    vec3 s1 = cross(in_mouseDirVS,v2);
    vec3 s2 = cross(v0,v1);
    vec2 uv = vec2( dot(v0,s1), dot(in_mouseDirVS,s2) )/dot(s1,v1);
    // Check if point is in triangle.
    if(uv.x<0.0 || uv.y<0.0 || uv.x+uv.y>1.0) return;
    // Find intersection depth
    float t = dot(v2,s2)/dot(s1,v1);
    float depth = in_mousePosVS.z + t*in_mouseDirVS.z;
    
    // Write picking output.
    out_pickObjectID = in_pickObjectID;
    out_pickInstanceID = fs_instanceID[0];
    out_pickDepth = depth;
    EmitVertex();
    EndPrimitive();
}
