
-- matrixInverse
#ifndef __matrixInverse_included__
#define __matrixInverse_included__
mat3 matrixInverse(in mat3 inMatrix){  
    float det = dot(cross(inMatrix[0], inMatrix[1]), inMatrix[2]);
    mat3 T = transpose(inMatrix);
    return mat3(
	cross(T[1], T[2]),
        cross(T[2], T[0]),
        cross(T[0], T[1])) / det;
}
#endif

-- linstep
#ifndef __linstep_included__
#define __linstep_included__
float linstep(float low, float high, float v) {
    return clamp((v-low)/(high-low), 0.0, 1.0);
}
#endif

-- isPointBetween
#ifndef __isPointBetween_included__
#define __isPointBetween_included__
bool isPointBetween(vec2 x, vec2 start, vec2 p0, vec2 p1)
{
    // point to start
    vec2 a = x-start;
    // start to first edge point
    vec2 b = start-p0;
    // second edge to first edge
    vec2 c = p1-p0;
    // find vector lengths
    float lambda = (b.y*a.x - b.x*a.y) / (c.y*a.x - c.x*a.y);
    float psi = (lambda*c.x - b.x)/a.x;
    return 0.0<=lambda && lambda<=1.0 && psi>=0.0;
}
#endif

-- isPointInCone
#ifndef __isPointInCone_included__
#define __isPointInCone_included__
bool isPointInCone(vec3 x, vec3 apex, vec3 direction, float halfConeAngleCos)
{
    vec3 pointDirection = normalize(apex-x);
    float pointAngleCos = dot(pointDirection,-direction);
    return pointAngleCos > halfConeAngleCos;
}
#endif

-- pointLineDistance
#ifndef __pointLineDistance_included__
#define __pointLineDistance_included__
float pointLineDistance(vec3 p, vec3 pl, vec3 dirl)
{
    return length( cross(dirl, p-pl) ) / length(dirl);
}
float pointLineDistance(vec2 p, vec2 pl, vec2 dirl)
{
    return pointLineDistance(vec3(p,0.0),vec3(pl,0.0),vec3(dirl,0.0));
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
    vec3 v0 = vec3(1)     - vec3(lessThanEqual(absDir,vec3(magnitude)));
    vec3 v1 = vec3(1,2,3) - 0.5*sign(dir);
    return int(dot(v0,v1));
}
#endif

-- computeClosestCubeLayer
#ifndef __computeClosestCubeLayer__included__
#define2 __computeClosestCubeLayer__included__
// Calculate cube map layers a sprite could affect.
// Should be done because a sprite can intersect with 3 cube faces at the same time.
int[3] computeClosestCubeLayer(vec3 p)
{
    return int[](
        1 - int(sign(p.x)*0.5 + 0.5), //0 or 1
        3 - int(sign(p.y)*0.5 + 0.5), //2 or 3
        5 - int(sign(p.z)*0.5 + 0.5)  //4 or 5
    );
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
#include regen.math.computeCubeAxis
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
