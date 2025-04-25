-- isSphereVisible
#ifndef isSphereVisible_included_
#define2 isSphereVisible_included_
bool isSphereVisible(vec3 center, float radius) {
#for PLANE_I to 6
    if (in_frustumPlanes[${PLANE_I}].w + radius <
        dot(in_frustumPlanes[${PLANE_I}].xyz, center)) return false;
#endfor
    return true;
}
#endif // isSphereVisible_included_

-- isAABBVisible
#ifndef isAABBVisible_included_
#define2 isAABBVisible_included_
bool isAABBVisible_i(vec3 aabbMin, vec3 aabbMax, int i) {
    // Select most negative vertex (outside-leaning)
    vec4 plane = in_frustumPlanes[i];
    vec3 p = vec3(
        plane.x > 0.0 ? aabbMin.x : aabbMax.x,
        plane.y > 0.0 ? aabbMin.y : aabbMax.y,
        plane.z > 0.0 ? aabbMin.z : aabbMax.z);
    // Compute distance to plane
    return (dot(plane.xyz, p) + plane.w < 0.0);
}

bool isAABBVisible(vec3 aabbMin, vec3 aabbMax) {
#for PLANE_I to 6
    if (isAABBVisible_i(aabbMin, aabbMax, ${PLANE_I})) return false;
#endfor
    return true;
}
#endif // isAABBVisible_included_

-- isOBBVisible
#ifndef isOBBVisible_included_
#define2 isOBBVisible_included_
bool isOBBVisible_i(vec3 center, vec3 halfExtents, mat3 basis, int i) {
    vec4 plane = in_frustumPlanes[i];
    // Project OBB onto plane normal
    float r =
        halfExtents.x * abs(dot(plane.xyz, basis[0])) +
        halfExtents.y * abs(dot(plane.xyz, basis[1])) +
        halfExtents.z * abs(dot(plane.xyz, basis[2]));
    float s = dot(plane.xyz, center) + plane.w;
    return (s + r < 0.0);
}

bool isOBBVisible(vec3 center, vec3 halfExtents, mat3 basis) {
#for PLANE_I to 6
    if (isOBBVisible_i(center, halfExtents, basis, ${PLANE_I})) return false;
#endfor
    return true;
}
#endif // isOBBVisible_included_

-- getModelScale
#ifndef getModelScale_included_
#define2 getModelScale_included_
#ifdef HAS_modelMatrix
vec3 getModelScale(uint index) {
    return vec3(
        in_modelMatrix[index][0][0],
        in_modelMatrix[index][1][1],
        in_modelMatrix[index][2][2]);
}
vec3 getModelScale(mat4 modelMatrix) {
    return vec3(
        modelMatrix[0][0],
        modelMatrix[1][1],
        modelMatrix[2][2]);
}
#else
#define getModelScale(index) vec3(1.0)
#endif // HAS_modelMatrix
#endif // getModelScale_included_

-- isShapeVisible
#ifndef isShapeVisible_included_
#define2 isShapeVisible_included_
#ifdef HAS_modelMatrix
#include regen.shapes.culling.getModelScale
#endif

///////// SHAPE_TYPE == SPHERE
#if SHAPE_TYPE == SPHERE
#include regen.shapes.culling.isSphereVisible
bool isShapeVisible(uint index, vec3 pos) {
    float radius = in_shapeRadius;
#ifdef HAS_modelMatrix
    vec3 scale = getModelScale(index);
    radius *= max(max(scale.x, scale.y), scale.z);
#endif // HAS_modelMatrix
#ifdef HAS_shapeOffset
    return isSphereVisible(pos + in_shapeOffset, radius);
#else // !HAS_shapeOffset
    return isSphereVisible(pos, radius);
#endif
}
#endif

///////// SHAPE_TYPE == AABB
#if SHAPE_TYPE == AABB
#include regen.shapes.culling.isAABBVisible
bool isShapeVisible(uint index, vec3 pos) {
    vec3 aabbMin = in_shapeAABBMin;
    vec3 aabbMax = in_shapeAABBMax;
    #ifdef HAS_modelMatrix
    vec3 scale = getModelScale(index);
    aabbMin *= scale;
    aabbMax *= scale;
    #endif
    return isAABBVisible(pos + aabbMin, pos + aabbMax);
}
#endif

///////// SHAPE_TYPE == OBB
#if SHAPE_TYPE == OBB
#ifdef HAS_modelMatrix
#include regen.shapes.culling.isOBBVisible
#else
#include regen.shapes.culling.isAABBVisible
#endif

bool isShapeVisible(uint index, vec3 pos) {
    vec3 aabbMin = in_shapeAABBMin;
    vec3 aabbMax = in_shapeAABBMax;
    #ifdef HAS_modelMatrix
    mat4 modelMatrix = in_modelMatrix[index];
    vec3 scale = getModelScale(modelMatrix);
    aabbMin *= scale;
    aabbMax *= scale;
    return isOBBVisible(
            // center position
            pos + (aabbMax + aabbMin) * 0.5,
            // half extents
            (aabbMax - aabbMin) * 0.5,
            // basis
            mat3(modelMatrix));
    #else // !HAS_modelMatrix
    return isAABBVisible(pos + aabbMin, pos + aabbMax);
    #endif
}
#endif
#endif // isShapeVisible_included_
