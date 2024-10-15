
-- matrixInverse
#ifndef REGEN_matrixInverse_included_
#define REGEN_matrixInverse_included_
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
#ifndef REGEN_linstep_included_
#define REGEN_linstep_included_
float linstep(float low, float high, float v) {
    return clamp((v-low)/(high-low), 0.0, 1.0);
}
#endif

-- isPointBetween
#ifndef REGEN_isPointBetween_included_
#define REGEN_isPointBetween_included_
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

-- random
#ifndef REGEN_random_Included
#define2 REGEN_random_Included
// return pseudorandom float with values between 0 and 1.
float random(inout uint seed) {
    seed = (seed * 1103515245u + 12345u);
    return float(seed) / 4294967296.0;
}
// return pseudorandom vec3 with values between -1 and 1.
vec3 random3(inout uint seed) {
    vec3 result;
    seed = (seed * 1103515245u + 12345u); result.x = float(seed);
    seed = (seed * 1103515245u + 12345u); result.y = float(seed);
    seed = (seed * 1103515245u + 12345u); result.z = float(seed);
    return (result / 2147483648.0) - vec3(1,1,1);
}
#endif

-- variance
#ifndef REGEN_variance_Included
#define2 REGEN_variance_Included
#include regen.math.random
float variance(float v, inout uint seed) {
    return v*2.0*(random(seed)-0.5);
}
vec2 variance(vec2 v, inout uint seed) {
    return vec2(
        variance(v.x,seed),
        variance(v.y,seed));
}
vec3 variance(vec3 v, inout uint seed) {
    return vec3(
        variance(v.x,seed),
        variance(v.y,seed),
        variance(v.z,seed));
}
vec4 variance(vec4 v, inout uint seed) {
    return vec4(
        variance(v.x,seed),
        variance(v.y,seed),
        variance(v.z,seed),
        variance(v.w,seed));
}
#endif

-- isPointInCone
#ifndef REGEN_isPointInCone_included_
#define REGEN_isPointInCone_included_
bool isPointInCone(vec3 x, vec3 apex, vec3 direction, float halfConeAngleCos)
{
    vec3 pointDirection = normalize(apex-x);
    float pointAngleCos = dot(pointDirection,-direction);
    return pointAngleCos > halfConeAngleCos;
}
#endif

-- pointLineDistance
#ifndef REGEN_pointLineDistance_included_
#define REGEN_pointLineDistance_included_
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
#ifndef REGEN_pointVectorDistance_included_
#define REGEN_pointVectorDistance_included_
float pointVectorDistance(vec3 dir, vec3 p)
{
    return dot(p,dir)/dot(dir,dir);
}
#endif


-- computeCubeAxis
#ifndef REGEN_computeCubeAxis_included_
#define2 REGEN_computeCubeAxis_included_
int computeCubeAxis(vec3 dir)
{
    vec3 a = abs(dir);
    float magnitude = max(a.x, max(a.y, a.z));
    // X->0, Y->1, Z->2
    return min(2, (1-int(a.y<magnitude)) + (1-int(a.z<magnitude))*2);
}
#endif

-- computeCubeLayer
#ifndef REGEN_computeCubeLayer_included_
#define2 REGEN_computeCubeLayer_included_
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
#ifndef REGEN_computeClosestCubeLayer_included_
#define2 REGEN_computeClosestCubeLayer_included_
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
#ifndef REGEN_computeCubeDirection_included_
#define2 REGEN_computeCubeDirection_included_
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
#ifndef REGEN_computeCubeOffset_included_
#define2 REGEN_computeCubeOffset_included_
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

-- computeSpritePoints2
#ifndef REGEN_computeSpritePoints_included_
#define2 REGEN_computeSpritePoints_included_
vec3[4] computeSpritePoints(vec3 point, vec2 size, vec3 zAxis, vec3 yAxis)
{
  vec3 xAxis = normalize(cross(zAxis,yAxis));
  vec3 x = xAxis*0.5*size.x;
  vec3 y = yAxis*0.5*size.y;
  return vec3[](
      point - x - y,
      point - x + y,
      point + x - y,
      point + x + y
  );
}
vec3[4] computeSpritePoints(vec3 point, vec2 size, vec3 yAxis)
{
  return computeSpritePoints(point,size,normalize(-point),yAxis);
}
#endif

-- computeSpritePoints
#ifndef REGEN_computeSpritePoints_included_
#define2 REGEN_computeSpritePoints_included_
vec3[4] computeSpritePoints(vec3 point, vec2 size, vec3 zAxis, vec3 upVector)
{
  vec3 xAxis = normalize(cross(zAxis, upVector));
  vec3 yAxis = normalize(cross(xAxis, zAxis));
  vec3 x = xAxis*0.5*size.x;
  vec3 y = yAxis*0.5*size.y;
  return vec3[](
      point - x - y,
      point - x + y,
      point + x - y,
      point + x + y
  );
}
vec3[4] computeSpritePoints(vec3 point, vec2 size, vec3 upVector)
{
  return computeSpritePoints(point,size,normalize(-point),upVector);
}
#endif
