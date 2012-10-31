-- gs
#if GS_INPUT_PRIMITIVE == points
#define GS_NUM_VERTICES 1
#elif GS_INPUT_PRIMITIVE == lines || GS_INPUT_PRIMITIVE == lines_adjacency
#define GS_NUM_VERTICES 2
#else
#define GS_NUM_VERTICES 3
#endif

layout(GS_INPUT_PRIMITIVE) in;
layout(points, max_vertices=1) out;

out int out_pickObjectID;
out int out_pickInstanceID;
out float out_pickDepth;

// pretend to be fragment shader
in int fs_instanceID[GS_NUM_VERTICES];

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
    vec3 dev0 = gl_in[0].gl_Position.xyz/gl_in[0].gl_Position.w;
#if GS_NUM_VERTICES == 1
    vec2 winPos0 = deviceToScreenSpace(dev0, in_viewport);
    float d = distance(winPos0,in_mousePosition);
    if(d<gl_in[0].gl_PointSize) {
        out_pickObjectID = in_pickObjectID;
        out_pickInstanceID = fs_instanceID[0];
        out_pickDepth = dev0.z;
        EmitVertex();
        EndPrimitive();
    }
#else
    vec3 dev1 = gl_in[1].gl_Position.xyz/gl_in[1].gl_Position.w;
#endif

#if GS_NUM_VERTICES == 2
    vec2 win0 = deviceToScreenSpace(dev0, in_viewport);
    vec2 win1 = deviceToScreenSpace(dev1, in_viewport);
    if(intersectsLine(win0,win1,in_mousePosition,gl_in[0].gl_PointSize)) {
        vec2 mouseDev = (2.0*(in_mousePosition/in_viewport) - vec2(1.0));
        out_pickObjectID = in_pickObjectID;
        out_pickInstanceID = fs_instanceID[0];
        out_pickDepth = intersectionDepth(dev0, dev1, mouseDev);
        EmitVertex();
        EndPrimitive();
    }
#endif
#if GS_NUM_VERTICES == 3
    vec3 dev2 = gl_in[2].gl_Position.xyz/gl_in[2].gl_Position.w;
    vec2 mouseDev = (2.0*(in_mousePosition/in_viewport) - vec2(1.0));
    mouseDev.y *= -1.0;
    vec2 bc = barycentricCoordinate(dev0, dev1, dev2, mouseDev);
    if(isInsideTriangle(bc)) {
        out_pickObjectID = in_pickObjectID;
        out_pickInstanceID = fs_instanceID[0];
        out_pickDepth = intersectionDepth(dev0,dev1,dev2,mouseDev);
        EmitVertex();
        EndPrimitive();
    }
#endif
}

