
-- cs
#include regen.stages.compute.defines
// Note: global memory must be cleared each frame on the CPU!
buffer ivec4 in_bboxMin;
buffer ivec4 in_bboxMax;
// Shared memory to compute min/max per workgroup.
shared ivec3 sh_min;
shared ivec3 sh_max;

#include regen.stages.compute.readPosition

int biasedBits(float f) {
    int i = floatBitsToInt(f);
    return i ^ ((i >> 31) & 0x7FFFFFFF);
}
//float debiased(int bits) {
//    return intBitsToFloat((i >= 0) ? i ^ signBit : ~i);
//}

#define _atomicMinMax(a,b,key) \
    atomicMin(a, key);\
    atomicMax(b, key);

void main() {
    uint gid = gl_GlobalInvocationID.x;
    uint lid = gl_LocalInvocationID.x;
    if (gid >= NUM_ELEMENTS) return;
    // read position data
    vec3 pos = readPosition(gid);
    // initialize shared memory
    if (lid == 0) {
        sh_min = ivec3(biasedBits(FLT_MAX));
        sh_max = ivec3(biasedBits(-FLT_MAX));
    }
    barrier();
    // update shared memory, i.e. each group computes its own min/max.
    {
        int key = biasedBits(pos.x);
        _atomicMinMax(sh_min.x, sh_max.x, key);
        key = biasedBits(pos.y);
        _atomicMinMax(sh_min.y, sh_max.y, key);
        key = biasedBits(pos.z);
        _atomicMinMax(sh_min.z, sh_max.z, key);
    }
    barrier();
    // First thread in each workgroup writes result to global memory
    if (lid == 0) {
        atomicMin(in_bboxMin.x, sh_min.x);
        atomicMin(in_bboxMin.y, sh_min.y);
        atomicMin(in_bboxMin.z, sh_min.z);
        atomicMax(in_bboxMax.x, sh_max.x);
        atomicMax(in_bboxMax.y, sh_max.y);
        atomicMax(in_bboxMax.z, sh_max.z);
    }
}

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
-- compute.bad.cs
#include regen.stages.compute.defines

// Note: global memory must be cleared each frame on the CPU!
buffer ivec4 in_bboxPositiveMin;
buffer ivec4 in_bboxPositiveMax;
buffer ivec4 in_bboxNegativeMin;
buffer ivec4 in_bboxNegativeMax;
buffer ivec4 in_bboxPositiveFlags;
buffer ivec4 in_bboxNegativeFlags;
// Shared memory to compute min/max per workgroup.
shared ivec3 sh_positiveMin;
shared ivec3 sh_positiveMax;
shared ivec3 sh_negativeMin;
shared ivec3 sh_negativeMax;
shared ivec3 sh_positiveFlags;
shared ivec3 sh_negativeFlags;
#define atomicMinVec3(mem, val) \
    atomicMin(mem.x, val.x); \
    atomicMin(mem.y, val.y); \
    atomicMin(mem.z, val.z)
#define atomicMaxVec3(mem, val) \
    atomicMax(mem.x, val.x); \
    atomicMax(mem.y, val.y); \
    atomicMax(mem.z, val.z)

#include regen.stages.compute.readPosition

// compute bounding box from min/max values of positive and negative ranges
float getBBoxMax(int neg, int pos, int hasPos) {
    return (hasPos == 1) ? intBitsToFloat(pos) : -intBitsToFloat(neg);
}
float getBBoxMin(int neg, int pos, int hasNeg) {
    return (hasNeg == 1) ? -intBitsToFloat(neg) : intBitsToFloat(pos);
}

void main() {
    uint gid = gl_GlobalInvocationID.x;
    uint lid = gl_LocalInvocationID.x;
    if (gid >= NUM_ELEMENTS) return;
    // read position data
    vec3 pos = readPosition(gid);

    // initialize shared memory
    if (lid == 0) {
        sh_positiveMin = ivec3(floatBitsToInt(FLT_MAX));
        sh_positiveMax = ivec3(floatBitsToInt(0.0));
        sh_negativeMin = ivec3(floatBitsToInt(FLT_MAX));
        sh_negativeMax = ivec3(floatBitsToInt(0.0));
        sh_positiveFlags = ivec3(0);
        sh_negativeFlags = ivec3(0);
    }
    barrier();

    // update shared memory, i.e. each group computes its own min/max.
    // use atomic operations on shared memory to avoid race conditions.
    if (pos.x < 0.0) {
        atomicMin(sh_negativeMin.x, floatBitsToInt(-pos.x));
        atomicMax(sh_negativeMax.x, floatBitsToInt(-pos.x));
        atomicMax(sh_negativeFlags.x, 1);
    } else {
        atomicMin(sh_positiveMin.x, floatBitsToInt(pos.x));
        atomicMax(sh_positiveMax.x, floatBitsToInt(pos.x));
        atomicMax(sh_positiveFlags.x, 1);
    }
    if (pos.y < 0.0) {
        atomicMin(sh_negativeMin.y, floatBitsToInt(-pos.y));
        atomicMax(sh_negativeMax.y, floatBitsToInt(-pos.y));
        atomicMax(sh_negativeFlags.y, 1);
    } else {
        atomicMin(sh_positiveMin.y, floatBitsToInt(pos.y));
        atomicMax(sh_positiveMax.y, floatBitsToInt(pos.y));
        atomicMax(sh_positiveFlags.y, 1);
    }
    if (pos.z < 0.0) {
        atomicMin(sh_negativeMin.z, floatBitsToInt(-pos.z));
        atomicMax(sh_negativeMax.z, floatBitsToInt(-pos.z));
        atomicMax(sh_negativeFlags.z, 1);
    } else {
        atomicMin(sh_positiveMin.z, floatBitsToInt(pos.z));
        atomicMax(sh_positiveMax.z, floatBitsToInt(pos.z));
        atomicMax(sh_positiveFlags.z, 1);
    }
    barrier();

    // First thread in each workgroup writes result to global memory
    if (lid == 0) {
        atomicMinVec3(in_bboxPositiveMin, sh_positiveMin);
        atomicMaxVec3(in_bboxPositiveMax, sh_positiveMax);
        atomicMinVec3(in_bboxNegativeMin, sh_negativeMin);
        atomicMaxVec3(in_bboxNegativeMax, sh_negativeMax);
        atomicMaxVec3(in_bboxPositiveFlags, sh_positiveFlags);
        atomicMaxVec3(in_bboxNegativeFlags, sh_negativeFlags);
    }
}
