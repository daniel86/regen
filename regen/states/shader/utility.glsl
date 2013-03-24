
-- linstep
#ifndef __linstep_included__
#define __linstep_included__
float linstep(float low, float high, float v) {
    return clamp((v-low)/(high-low), 0.0, 1.0);
}
#endif

-- texcoToWorldSpace
#ifndef __texcoToWorldSpace_included__
#define __texcoToWorldSpace_included__
vec3 texcoToWorldSpace(vec2 texco, float depth) {
    vec3 pos0 = vec3(texco.xy, depth)*2.0 - vec3(1.0);
    vec4 D = in_inverseViewProjectionMatrix*vec4(pos0,1.0);
    return D.xyz/D.w;
}
#endif

-- texcoToViewSpace
#ifndef __texcoToViewSpace_included__
#define __texcoToViewSpace_included__
vec3 texcoToViewSpace(vec2 texco, float depth) {
    vec3 pos0 = vec3(texco.xy, depth)*2.0 - vec3(1.0);
    vec4 D = in_inverseProjectionMatrix*vec4(pos0,1.0);
    return D.xyz/D.w;
}
#endif

-- worldSpaceToTexco
#ifndef __worldSpaceToTexco_included__
#define __worldSpaceToTexco_included__
vec3 worldSpaceToTexco(vec4 ws)
{
    vec4 ss = in_viewProjectionMatrix*ws;
    return (ss.xyz/ss.w + vec3(1.0))*0.5;
}
#endif

-- eyeSpaceToTexco
#ifndef __eyeSpaceToTexco_included__
#define __eyeSpaceToTexco_included__
vec3 eyeSpaceToTexco(vec4 es)
{
    vec4 ss = in_projectionMatrix*ws;
    return (ss.xyz/ss.w + vec3(1.0))*0.5;
}
#endif

-- linearizeDepth
#ifndef __linearizeDepth_included__
#define __linearizeDepth_included__
//float linearizeDepth(float d, float n, float f)
//{
//    float z_n = 2.0*d - 1.0;
//    return 2.0*n*f/(f+n-z_n*(f-n));
//}
float linearizeDepth(float expDepth, float n, float f)
{
    return (2.0*n)/(f+n - expDepth*(f-n));
}
#endif

-- pointVectorDistance
#ifndef __pointVectorDistance_included__
#define __pointVectorDistance_included__
float pointVectorDistance(vec3 dir, vec3 p)
{
    return dot(p,dir)/dot(dir,dir);
}
#endif

-- computeCubeAxis
#ifndef __computeCubeAxis__included__
#define2 __computeCubeAxis__included__
int computeCubeAxis(vec3 dir)
{
    vec3 a = abs(dir);
    float magnitude = max(a.x, max(a.y, a.z));
    // X->0, Y->1, Z->2
    return min(2, (1-int(a.y<magnitude)) + (1-int(a.z<magnitude))*2);
}
#endif

-- computeCubeLayer
#ifndef __computeCubeLayer__included__
#define2 __computeCubeLayer__included__
int computeCubeLayer(vec3 dir)
{
    vec3 absDir = abs(dir);
    float magnitude = max(absDir.x, max(absDir.y, absDir.z));
    return
        (1-int(absDir.x<magnitude))*int(1.0 - (sign(dir.x)+1.0)/2.0) +
        (1-int(absDir.y<magnitude))*int(3.0 - (sign(dir.y)+1.0)/2.0) +
        (1-int(absDir.z<magnitude))*int(5.0 - (sign(dir.z)+1.0)/2.0);
}
#endif

-- computeCubeDirection
#ifndef __computeCubeDirection__included__
#define2 __computeCubeDirection__included__
vec3 computeCubeDirection(vec2 uv, int layer)
{
    vec3 cubePoints[6] = vec3[](
        vec3(  1.0, uv.y,-uv.x), // +X
        vec3( -1.0, uv.y, uv.x), // -X
        vec3( uv.x,  1.0,-uv.y), // +Y
        vec3( uv.x, -1.0, uv.y), // -Y
        vec3( uv.x, uv.y,  1.0), // +Z
        vec3(-uv.x, uv.y, -1.0)  // -Z
    );
    return cubePoints[layer];
}
#endif

-- computeCubeOffset
#ifndef __computeCubeOffset__included__
#define2 __computeCubeOffset__included__
#include utility.computeCubeAxis
void computeCubeOffset(vec3 dir, float x, out vec3 dx, out vec3 dy)
{
    vec3 offsetsX[3] = vec3[](
        vec3(0.0,  x,0.0), // X
        vec3(  x,0.0,0.0), // Y
        vec3(  x,0.0,0.0)  // Z
    );
    vec3 offsetsY[3] = vec3[](
        vec3(0.0,0.0,  x), // X
        vec3(0.0,0.0,  x), // Y
        vec3(0.0,  x,0.0)  // Z
    );
    int axis = computeCubeAxis(dir);
    dx = offsetsX[axis];
    dy = offsetsY[axis];
}
#endif

