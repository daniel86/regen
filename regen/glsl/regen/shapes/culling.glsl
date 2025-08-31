-- isSphereVisible
#ifndef isSphereVisible_included_
#define2 isSphereVisible_included_
bool isSphereVisible3(vec3 center, float radius, uint frustumOffset) {
    uint idx;
    #for PLANE_I to 6
    idx = frustumOffset + ${PLANE_I};
    if (in_frustumPlanes[idx].w +
        dot(in_frustumPlanes[idx].xyz, center) +
        radius < 0) return false;
    #endfor
    return true;
}
    #if NUM_LAYERS > 1
bool isSphereVisible2(vec3 center, float radius) {
    #for LAYER_I to NUM_LAYERS
    if(isSphereVisible3(center, radius, ${LAYER_I} * 6)) return true;
    #endfor
    return false;
}
    #else // NUM_LAYERS == 1
#define isSphereVisible2(center, radius) isSphereVisible(center, radius, 0)
    #endif // NUM_LAYERS > 1
#endif // isSphereVisible_included_

-- isAABBVisible
#ifndef isAABBVisible_included_
#define2 isAABBVisible_included_
bool isAABBBehindPlane(vec3 aabbMin, vec3 aabbMax, uint i) {
    // Select vertext farthest from the plane in direction of the plane normal.
    // If this point is behind the plane, the AABB must be outside of the frustum.
    vec4 plane = in_frustumPlanes[i];
    vec3 p = vec3(
        plane.x < 0.0 ? aabbMin.x : aabbMax.x,
        plane.y < 0.0 ? aabbMin.y : aabbMax.y,
        plane.z < 0.0 ? aabbMin.z : aabbMax.z);
    // Compute distance to plane
    return (plane.w + dot(plane.xyz, p) < 0.0);
}

bool isAABBVisible3(vec3 aabbMin, vec3 aabbMax, uint frustumOffset) {
    #for PLANE_I to 6
    if (isAABBBehindPlane(aabbMin, aabbMax, frustumOffset + ${PLANE_I})) return false;
    #endfor
    return true;
}
    #if NUM_LAYERS > 1
bool isAABBVisible2(vec3 aabbMin, vec3 aabbMax) {
    #for LAYER_I to NUM_LAYERS
    if(isAABBVisible3(aabbMin, aabbMax, ${LAYER_I} * 6)) return true;
    #endfor
    return false;
}
    #else // NUM_LAYERS == 1
#define isAABBVisible2(aabbMin, aabbMax) isAABBVisible(aabbMin, aabbMax, 0)
    #endif // NUM_LAYERS > 1
#endif // isAABBVisible_included_

-- isOBBVisible
#ifndef isOBBVisible_included_
#define2 isOBBVisible_included_
bool isOBBVisible_i(vec3 center, vec3 halfExtents, mat3 basis, uint i) {
    vec4 plane = in_frustumPlanes[i];
    // Project OBB onto plane normal
    float r =
        halfExtents.x * abs(dot(plane.xyz, basis[0])) +
        halfExtents.y * abs(dot(plane.xyz, basis[1])) +
        halfExtents.z * abs(dot(plane.xyz, basis[2]));
    float s = dot(plane.xyz, center) + plane.w;
    return (s + r < 0.0);
}

bool isOBBVisible4(vec3 center, vec3 halfExtents, mat3 basis, uint frustumOffset) {
    #for PLANE_I to 6
    if (isOBBVisible_i(center, halfExtents, basis, frustumOffset + ${PLANE_I})) return false;
    #endfor
    return true;
}
    #if NUM_LAYERS > 1
bool isOBBVisible3(vec3 center, vec3 halfExtents, mat3 basis) {
    #for LAYER_I to NUM_LAYERS
    if(isOBBVisible4(center, halfExtents, basis, ${LAYER_I} * 6)) return true;
    #endfor
    return false;
}
    #else // NUM_LAYERS == 1
#define isOBBVisible3(center, halfExtents, basis) isOBBVisible4(center, halfExtents, basis, 0)
    #endif // NUM_LAYERS > 1
#endif // isOBBVisible_included_

-- getModelScale
#ifndef getModelScale_included_
#define2 getModelScale_included_
#ifdef HAS_modelMatrix
vec3 getModelScale(uint index) {
    mat4 m = fetch_modelMatrix(index);
    return vec3(m[0][0],m[1][1],m[2][2]);
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
bool isShapeVisible(uint layer, uint index, vec3 pos) {
    float radius = in_shapeRadius;
#ifdef HAS_modelMatrix
    vec3 scale = getModelScale(index);
    radius *= max(max(scale.x, scale.y), scale.z);
    //pos *= scale;
#endif // HAS_modelMatrix
#ifdef HAS_shapeOffset
    return isSphereVisible3(pos + in_shapeOffset, radius, layer * 6);
#else // !HAS_shapeOffset
    return isSphereVisible3(pos, radius, layer * 6);
#endif
}
#endif

///////// SHAPE_TYPE == AABB
#if SHAPE_TYPE == AABB
#include regen.shapes.culling.isAABBVisible
bool isShapeVisible(uint layer, uint index, vec3 pos) {
    vec3 aabbMin = in_shapeAABBMin.xyz;
    vec3 aabbMax = in_shapeAABBMax.xyz;
    #ifdef HAS_modelMatrix
    vec3 scale = getModelScale(index);
    aabbMin *= scale;
    aabbMax *= scale;
    //pos *= scale;
    #endif
    return isAABBVisible3(pos + aabbMin, pos + aabbMax, layer * 6);
}
#endif

///////// SHAPE_TYPE == OBB
#if SHAPE_TYPE == OBB
#ifdef HAS_modelMatrix
#include regen.shapes.culling.isOBBVisible
#else
#include regen.shapes.culling.isAABBVisible
#endif

bool isShapeVisible(uint layer, uint index, vec3 pos) {
    vec3 aabbMin = in_shapeAABBMin;
    vec3 aabbMax = in_shapeAABBMax;
    #ifdef HAS_modelMatrix
    mat4 modelMatrix = in_modelMatrix[index];
    vec3 scale = getModelScale(modelMatrix);
    aabbMin *= scale;
    aabbMax *= scale;
    return isOBBVisible4(
            // center position
            pos + (aabbMax + aabbMin) * 0.5,
            // half extents
            (aabbMax - aabbMin) * 0.5,
            // basis
            mat3(modelMatrix),
            // frustum offset
            layer * 6);
    #else // !HAS_modelMatrix
    return isAABBVisible3(pos + aabbMin, pos + aabbMax, layer * 6);
    #endif
}
#endif
#endif // isShapeVisible_included_
