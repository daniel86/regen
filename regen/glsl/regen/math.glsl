
-- mat4.scale
#ifndef REGEN_mat4_scale_included_
#define2 REGEN_mat4_scale_included_
void scale(inout mat4 m, vec3 scale) {
    m[0] *= scale.x;
    m[1] *= scale.y;
    m[2] *= scale.z;
}
#endif

-- mat3.inverse
#ifndef REGEN_mat3_inverse_included_
#define2 REGEN_mat3_inverse_included_
mat3 matrixInverse(in mat3 inMatrix){
    float det = dot(cross(inMatrix[0], inMatrix[1]), inMatrix[2]);
    mat3 T = transpose(inMatrix);
    return mat3(
	cross(T[1], T[2]),
        cross(T[2], T[0]),
        cross(T[0], T[1])) / det;
}
#endif

-- quaternion
#include regen.math.quaternion.0
#include regen.math.quaternion.1
#include regen.math.quaternion.inverse
#include regen.math.quaternion.rotate
#include regen.math.quaternion.multiply

-- quaternion.0
#ifndef REGEN_quaternion_create0_included_
#define2 REGEN_quaternion_create0_included_
vec4 quaternion(vec3 axis, float angle) {
	float halfAngle = angle * 0.5;
    return vec4(
        axis * sin(halfAngle), // x, y, z
        cos(halfAngle)         // w
    );
}
#endif

-- quaternion.1
#ifndef REGEN_quaternion_create1_included_
#define2 REGEN_quaternion_create1_included_
vec4 quaternion(float pitch, float yaw, float roll) {
    float fSinPitch = sin(pitch * 0.5f);
    float fCosPitch = cos(pitch * 0.5f);
    float fSinYaw = sin(yaw * 0.5f);
    float fCosYaw = cos(yaw * 0.5f);
    float fSinRoll = sin(roll * 0.5f);
    float fCosRoll = cos(roll * 0.5f);
    return vec4(
        fSinRoll * fCosPitch * fCosYaw - fCosRoll * fSinPitch * fSinYaw,  // x
        fCosRoll * fSinPitch * fCosYaw + fSinRoll * fCosPitch * fSinYaw,  // y
        fCosRoll * fCosPitch * fSinYaw - fSinRoll * fSinPitch * fCosYaw,  // z
        fCosRoll * fCosPitch * fCosYaw + fSinRoll * fSinPitch * fSinYaw   // w
    );
}
#endif

-- quaternion.rotate
#ifndef REGEN_quaternion_rotate_included_
#define2 REGEN_quaternion_rotate_included_
#include regen.math.quaternion.multiply
vec3 q_rotate(vec4 q, vec3 v) {
    vec4 q2 = vec4( v.xyz, 0.0);
    vec4 q1 = vec4(-q.xyz, q.w);
    return q_multiply(q_multiply(q1, q2), q).xyz;
}
#endif

-- quaternion.multiply
#ifndef REGEN_quaternion_multiply_included_
#define2 REGEN_quaternion_multiply_included_
vec4 q_multiply(vec4 qA, vec4 qB) {
    vec4 q;
    q.x = qA.w * qB.x + qA.x * qB.w + qA.y * qB.z - qA.z * qB.y;
    q.y = qA.w * qB.y + qA.y * qB.w + qA.z * qB.x - qA.x * qB.z;
    q.z = qA.w * qB.z + qA.z * qB.w + qA.x * qB.y - qA.y * qB.x;
    q.w = qA.w * qB.w - qA.x * qB.x - qA.y * qB.y - qA.z * qB.z;
    return q;
}
#endif

-- quaternion.inverse
#ifndef REGEN_quaternion_inverse_included_
#define2 REGEN_quaternion_inverse_included_
vec4 q_inverse(vec4 quaternion) {
    float lengthSqr = quaternion.x * quaternion.x + quaternion.y * quaternion.y + quaternion.z * quaternion.z + quaternion.w * quaternion.w;
	if(lengthSqr < 0.001) {
	    return vec4(0, 0, 0, 1.0f);
	}
	quaternion.x = -quaternion.x / lengthSqr;
	quaternion.y = -quaternion.y / lengthSqr;
	quaternion.z = -quaternion.z / lengthSqr;
	quaternion.w = quaternion.w / lengthSqr;
	return quaternion;
}
#endif

-- quaternion.matrix
#ifndef REGEN_quaternion_matrix_included_
#define2 REGEN_quaternion_matrix_included_
mat4 q_matrix(vec4 q) {
    float xx = q.x * q.x;
    float xy = q.x * q.y;
    float xz = q.x * q.z;
    float xw = q.w * q.x;
    float yy = q.y * q.y;
    float yz = q.y * q.z;
    float yw = q.w * q.y;
    float zz = q.z * q.z;
    float zw = q.w * q.z;

    mat4 result = mat4(1.0);
	result[0][0] = -2.0 * (yy + zz) + 1.0;
	result[0][1] =  2.0 * (xy - zw);
	result[0][2] =  2.0 * (xz + yw);
	result[1][0] =  2.0 * (xy + zw);
	result[1][1] = -2.0 * (xx + zz) + 1.0;
	result[1][2] =  2.0 * (yz - xw);
	result[2][0] =  2.0 * (xz - yw);
	result[2][1] =  2.0 * (yz + xw);
	result[2][2] = -2.0 * (xx + yy) + 1.0;
    return result;
}
#endif

-- rotateXZ
#ifndef REGEN_rotateXZ_included_
#define2 REGEN_rotateXZ_included_
vec3 rotateXZ(vec3 v, float angle) {
    float s = sin(angle);
    float c = cos(angle);
    return vec3(v.x * c - v.z * s, v.y, v.x * s + v.z * c);
}
#endif

-- rotateXY
#ifndef REGEN_rotateXY_included_
#define2 REGEN_rotateXY_included_
vec3 rotateXY(vec3 v, float angle) {
    float s = sin(angle);
    float c = cos(angle);
    return vec3(v.x * c - v.y * s, v.x * s + v.y * c, v.z);
}
#endif

-- rotateYZ
#ifndef REGEN_rotateYZ_included_
#define2 REGEN_rotateYZ_included_
vec3 rotateYZ(vec3 v, float angle) {
    float s = sin(angle);
    float c = cos(angle);
    return vec3(v.x, v.y * c - v.z * s, v.y * s + v.z * c);
}
#endif

-- linstep
#ifndef REGEN_linstep_included_
#define2 REGEN_linstep_included_
float linstep(float low, float high, float v) {
    return clamp((v-low)/(high-low), 0.0, 1.0);
}
#endif

-- isPointBetween
#ifndef REGEN_isPointBetween_included_
#define2 REGEN_isPointBetween_included_
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
#ifndef REGEN_isPointInCone_included_
#define2 REGEN_isPointInCone_included_
bool isPointInCone(vec3 x, vec3 apex, vec3 direction, float halfConeAngleCos)
{
    vec3 pointDirection = normalize(apex-x);
    float pointAngleCos = dot(pointDirection,-direction);
    return pointAngleCos > halfConeAngleCos;
}
#endif

-- pointLineDistance
#ifndef REGEN_pointLineDistance_included_
#define2 REGEN_pointLineDistance_included_
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
#define2 REGEN_pointVectorDistance_included_
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
    vec3 absDir = abs(normalize(dir));
    float magnitude = max(absDir.x, max(absDir.y, absDir.z));
    //vec3 isMax = step(magnitude, vec3(absDir));
    vec3 isMax = vec3(1.0) - vec3(lessThan(absDir,vec3(magnitude)));
    vec3 layer = vec3(0.5,2.5,4.5) - 0.5*sign(dir);
    return int(dot(isMax,layer));
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

-- computeDepth
#ifndef REGEN_computeDepth_included_
#define2 REGEN_computeDepth_included_
float computeDepth(float value, float near, float far) {
    float norm_z_comp = (far + near) /
        (far - near) - (2 * far * near) / (far - near) / value;
    return (norm_z_comp + 1.0) * 0.5;
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
void computeSpritePoints(vec3 point, vec2 size, vec3 zAxis, vec3 upVector, out vec3[4] spritePoints)
{
    vec3 xAxis = normalize(cross(zAxis, upVector));
    vec3 yAxis = normalize(cross(xAxis, zAxis));
    vec3 x = xAxis*0.5*size.x;
    vec3 y = yAxis*0.5*size.y;
    spritePoints[0] = point - x - y;
    spritePoints[1] = point - x + y;
    spritePoints[2] = point + x - y;
    spritePoints[3] = point + x + y;
}
vec3[4] computeSpritePoints(vec3 point, vec2 size, vec3 zAxis, vec3 upVector)
{
    vec3 spritePoints[4];
    computeSpritePoints(point, size, zAxis, upVector, spritePoints);
    return spritePoints;
}
vec3[4] computeSpritePoints(vec3 point, vec2 size, vec3 upVector)
{
    // billboard sprite
    return computeSpritePoints(point,size,normalize(-point),upVector);
}
#endif
