--------------
----- Computes a bounding box over position and/or model matrix data.
----- Buffers:
----- - [read] in_pos: position data as float[3] array
----- - [write] bounding box buffer (in_bboxMin, ...)
-----
----- While this is a trivial problem, it actually poses some challenges in compute.
----- One is how we can parallelize the computation of min/max of floating point values
----- given that there are no atomics for floating point values in OpenGL.
----- So we need to convert to integer bit representation and back. However,
----- converted negative values would not preserve ordering, so we rather
----- convert to positive values and use a flag to indicate if the value was negative.
----- Then we compute the min/max of the positive and negative values separately.
----- Another problem is with padding restrictions of SSBOs and using vec3 position data.
----- The solution taken here is to "cast" the vec3[] to a float[] buffer, which is tightly packed
----- and still complies std430 for SSBOs.
--------------
-- computeBBox
#ifndef computeBBox_included
#define2 computeBBox_included
// Bounding box SSBO.
buffer vec4 in_bboxMin;
buffer vec4 in_bboxMax;
buffer ivec4 in_bboxPositiveMin;
buffer ivec4 in_bboxPositiveMax;
buffer ivec4 in_bboxNegativeMin;
buffer ivec4 in_bboxNegativeMax;
buffer ivec4 in_bboxPositiveFlags;
buffer ivec4 in_bboxNegativeFlags;
// Shared memory to compute min/max per workgroup.
shared ivec3 l_positiveMin;
shared ivec3 l_positiveMax;
shared ivec3 l_negativeMin;
shared ivec3 l_negativeMax;
shared ivec3 l_positiveFlags;
shared ivec3 l_negativeFlags;
#define atomicMinVec3(mem, val) \
    atomicMin(mem.x, val.x); \
    atomicMin(mem.y, val.y); \
    atomicMin(mem.z, val.z)
#define atomicMaxVec3(mem, val) \
    atomicMax(mem.x, val.x); \
    atomicMax(mem.y, val.y); \
    atomicMax(mem.z, val.z)
// compute bounding box from min/max values of positive and negative ranges
float getBBoxMax(int neg, int pos, int hasPos) {
    return (hasPos == 1) ? intBitsToFloat(pos) : -intBitsToFloat(neg);
}
float getBBoxMin(int neg, int pos, int hasNeg) {
    return (hasNeg == 1) ? -intBitsToFloat(neg) : intBitsToFloat(pos);
}
void computeBBox(uint gid, vec3 pos, uint numElements) {
    uint lid = gl_LocalInvocationID.x;

    // initialize global memory
    if (gid == 0) {
        in_bboxPositiveMin = ivec4(floatBitsToInt(FLT_MAX));
        in_bboxPositiveMax = ivec4(floatBitsToInt(0.0));
        in_bboxNegativeMin = ivec4(floatBitsToInt(FLT_MAX));
        in_bboxNegativeMax = ivec4(floatBitsToInt(0.0));
        in_bboxPositiveFlags = ivec4(0);
        in_bboxNegativeFlags = ivec4(0);
    }

    // initialize shared memory
    if (lid == 0) {
        l_positiveMin = ivec3(floatBitsToInt(FLT_MAX));
        l_positiveMax = ivec3(floatBitsToInt(0.0));
        l_negativeMin = ivec3(floatBitsToInt(FLT_MAX));
        l_negativeMax = ivec3(floatBitsToInt(0.0));
        l_positiveFlags = ivec3(0);
        l_negativeFlags = ivec3(0);
    }
    memoryBarrierShared();

    // update shared memory, i.e. each group computes its own min/max.
    // use atomic operations on shared memory to avoid race conditions.
    if (pos.x < 0.0) {
        atomicMin(l_negativeMin.x, floatBitsToInt(-pos.x));
        atomicMax(l_negativeMax.x, floatBitsToInt(-pos.x));
        atomicMax(l_negativeFlags.x, 1);
    } else {
        atomicMin(l_positiveMin.x, floatBitsToInt(pos.x));
        atomicMax(l_positiveMax.x, floatBitsToInt(pos.x));
        atomicMax(l_positiveFlags.x, 1);
    }
    if (pos.y < 0.0) {
        atomicMin(l_negativeMin.y, floatBitsToInt(-pos.y));
        atomicMax(l_negativeMax.y, floatBitsToInt(-pos.y));
        atomicMax(l_negativeFlags.y, 1);
    } else {
        atomicMin(l_positiveMin.y, floatBitsToInt(pos.y));
        atomicMax(l_positiveMax.y, floatBitsToInt(pos.y));
        atomicMax(l_positiveFlags.y, 1);
    }
    if (pos.z < 0.0) {
        atomicMin(l_negativeMin.z, floatBitsToInt(-pos.z));
        atomicMax(l_negativeMax.z, floatBitsToInt(-pos.z));
        atomicMax(l_negativeFlags.z, 1);
    } else {
        atomicMin(l_positiveMin.z, floatBitsToInt(pos.z));
        atomicMax(l_positiveMax.z, floatBitsToInt(pos.z));
        atomicMax(l_positiveFlags.z, 1);
    }
    memoryBarrierShared();

    // First thread in each workgroup writes result to global memory
    if (lid == 0) {
        atomicMinVec3(in_bboxPositiveMin, l_positiveMin);
        atomicMaxVec3(in_bboxPositiveMax, l_positiveMax);
        atomicMinVec3(in_bboxNegativeMin, l_negativeMin);
        atomicMaxVec3(in_bboxNegativeMax, l_negativeMax);
        atomicMaxVec3(in_bboxPositiveFlags, l_positiveFlags);
        atomicMaxVec3(in_bboxNegativeFlags, l_negativeFlags);
    }
    barrier();

    // Finally write bounding box as float values based on pos/neg boundaries in global memory.
    // This is done in the first thread in global workgroup.
    if (gid == 0) {
        in_bboxMax.x = getBBoxMax(in_bboxNegativeMin.x, in_bboxPositiveMax.x, in_bboxPositiveFlags.x);
        in_bboxMin.x = getBBoxMin(in_bboxNegativeMax.x, in_bboxPositiveMin.x, in_bboxNegativeFlags.x);
        in_bboxMax.y = getBBoxMax(in_bboxNegativeMin.y, in_bboxPositiveMax.y, in_bboxPositiveFlags.y);
        in_bboxMin.y = getBBoxMin(in_bboxNegativeMax.y, in_bboxPositiveMin.y, in_bboxNegativeFlags.y);
        in_bboxMax.z = getBBoxMax(in_bboxNegativeMin.z, in_bboxPositiveMax.z, in_bboxPositiveFlags.z);
        in_bboxMin.z = getBBoxMin(in_bboxNegativeMax.z, in_bboxPositiveMin.z, in_bboxNegativeFlags.z);
    }
}

-- compute.cs
#include regen.stages.compute.defines
// Position data
buffer float in_pos[];
#include regen.shapes.bbox
#include regen.stages.compute.readPosition
#include regen.stages.compute.computeBBox
void main() {
    uint gid = gl_GlobalInvocationID.x;
    uint numElements = in_pos.length() / 3;
    if (gid < numElements) {
        vec3 pos = readPosition(gid);
        computeBBox(gid, pos, numElements);
    }
}
