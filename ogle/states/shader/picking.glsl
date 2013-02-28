-- gs
#version 330

layout(triangles) in;
layout(points, max_vertices=1) out;

out int out_pickObjectID;
out int out_pickInstanceID;
out float out_pickDepth;

/*
// pretend to be fragment shader
in int fs_instanceID[3];

uniform vec2 in_mousePosition;
uniform vec2 in_viewport;
uniform int in_pickObjectID;

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
*/

void main() {
        gl_Position = gl_in[0].gl_Position;
        out_pickObjectID = 1;
        out_pickInstanceID = 2;
        out_pickDepth = 0.5;
        EmitVertex();
        EndPrimitive();
/*
    vec3 dev0 = gl_in[0].gl_Position.xyz/gl_in[0].gl_Position.w;
    vec3 dev1 = gl_in[1].gl_Position.xyz/gl_in[1].gl_Position.w;
    vec3 dev2 = gl_in[2].gl_Position.xyz/gl_in[2].gl_Position.w;
// TODO
    //vec2 mouseDev = (2.0*(in_mousePosition/in_viewport) - vec2(1.0));
    vec2 mouseDev = (2.0*(in_mousePosition/vec2(800,600)) - vec2(1.0));
    mouseDev.y *= -1.0;
    vec2 bc = barycentricCoordinate(dev0, dev1, dev2, mouseDev);
    if(isInsideTriangle(bc)) {
        out_pickObjectID = in_pickObjectID;
        out_pickInstanceID = fs_instanceID[0];
        out_pickDepth = intersectionDepth(dev0,dev1,dev2,mouseDev);
        EmitVertex();
        EndPrimitive();
    }
*/
}

