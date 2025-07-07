-- geom.gs
#include regen.states.camera.defines
#include regen.defines.all
layout(triangles) in;
layout(points, max_vertices=1) out;

// the picker output
out int out_pickObjectID;
out int out_pickInstanceID;
out float out_pickDepth;
in int in_instanceID[3];

// camera input
#include regen.states.camera.input
// mouse ray intersecting the view frustum in view space.
// in ndc the ray starts at (mx,my,0) and ends at (mx,my,-1)
uniform vec3 in_mousePosVS;
uniform vec3 in_mouseDirVS;
uniform vec2 in_mouseTexco;
// mesh id
uniform int in_objectID;
uniform sampler2D in_gDepthTexture;

#define HANDLE_IO(i)

void main()
{
    vec3 a = (REGEN_PROJ_INV_(0) * gl_in[0].gl_Position).xyz;
    vec3 b = (REGEN_PROJ_INV_(0) * gl_in[1].gl_Position).xyz;
    vec3 c = (REGEN_PROJ_INV_(0) * gl_in[2].gl_Position).xyz;
    // Compute barycentric coordinates
    vec3 v0 = in_mousePosVS - a;
    vec3 v1 = b - a;
    vec3 v2 = c - a;
    vec3 s1 = cross(in_mouseDirVS,v2);
    vec3 s2 = cross(v0,v1);
    vec2 uv = vec2( dot(v0,s1), dot(in_mouseDirVS,s2) )/dot(s1,v1);
    // Check if point is in triangle.
    if(uv.x<0.0 || uv.y<0.0 || uv.x+uv.y>1.0) return;
    float t = dot(v2,s2)/dot(s1,v1);
    // Ensure the intersection is in the forward direction
    if (t > 0.0) return;

    // Sample the depth texture at the mouse position, and convert it to from NDC to view space
    float depthBufferValue = texture(in_gDepthTexture, in_mouseTexco).r;
    float z_ndc = depthBufferValue * 2.0 - 1.0;
    float far = REGEN_CAM_FAR_(0);
    float near = REGEN_CAM_NEAR_(0);
    float z_view = (2.0 * near * far) / (far + near - z_ndc * (far - near));

    // Proceed only if the intersection depth is less than or equal to the depth buffer value
    // plus a small epsilon to account for precision issues
    float intersectionDepth = in_mousePosVS.z - t * in_mouseDirVS.z;
    const float epsilon = 0.001;
    if (intersectionDepth > z_view + epsilon) return;

    // Write picking output.
    out_pickObjectID = in_objectID;
    out_pickInstanceID = in_instanceID[0];
    out_pickDepth = intersectionDepth;
    HANDLE_IO(0);
    EmitVertex();
    EndPrimitive();
}

-- gs
#version 150

layout(triangles) in;
layout(points, max_vertices=1) out;

// the picker output
out int out_pickObjectID;
out int out_pickInstanceID;
out float out_pickDepth;
// pretend to be fragment shader for name matching.
flat in int fs_instanceID[3];

// camera input
#include regen.states.camera.input
// mouse ray intersecting the view frustum in view space.
// in ndc the ray starts at (mx,my,0) and ends at (mx,my,-1)
uniform vec3 in_mousePosVS;
uniform vec3 in_mouseDirVS;
uniform vec2 in_mouseTexco;
// mesh id
uniform int in_objectID;

void main()
{
    vec3 a = (REGEN_PROJ_INV_(0) * gl_in[0].gl_Position).xyz;
    vec3 b = (REGEN_PROJ_INV_(0) * gl_in[1].gl_Position).xyz;
    vec3 c = (REGEN_PROJ_INV_(0) * gl_in[2].gl_Position).xyz;
    // Compute barycentric coordinates
    vec3 v0 = in_mousePosVS - a;
    vec3 v1 = b - a;
    vec3 v2 = c - a;
    vec3 s1 = cross(in_mouseDirVS,v2);
    vec3 s2 = cross(v0,v1);
    vec2 uv = vec2( dot(v0,s1), dot(in_mouseDirVS,s2) )/dot(s1,v1);
    // Check if point is in triangle.
    if(uv.x<0.0 || uv.y<0.0 || uv.x+uv.y>1.0) return;
    float t = dot(v2,s2)/dot(s1,v1);
    // TODO: compare to depth texture to avoid processing all meshes here!
    
    // Write picking output.
    out_pickObjectID = in_objectID;
    out_pickInstanceID = fs_instanceID[0];
    out_pickDepth = in_mousePosVS.z + t*in_mouseDirVS.z;
    EmitVertex();
    EndPrimitive();
}
