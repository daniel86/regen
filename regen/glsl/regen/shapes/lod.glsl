// This shader is used for dynamically computing the LOD of objects in an
// input array. It performs a culling pass to determine which objects are visible
// and computes LOD level based on their distance to the camera.
// The shader uses a radix sort to sort the objects based on their
// computed keys (e.g., depth).

-- defines
// The number of bits used for each radix pass.
// Assuming a 32-bit key, this means we can have up to 32/RADIX_BITS passes.
// A common value is 4, which gives us 8 passes.
#define RADIX_BITS 4
// Each pass fills buckets based on the different states that the RADIX_BITS bits can take.
// For example, if RADIX_BITS is 4, we have 16 buckets (0-15).
// If RADIX_BITS is 1, we have 2 buckets (0-1). So e.g. the inputs 01 and 11 would
// be added into buckets 0 and 1 respectively for pass 1, and both would be added into
// bucket 1 for pass 2.
#define RADIX_NUM_BUCKETS 16
#define RADIX_ONE_LESS_NUM_BUCKETS 15u
// LOD groups: high to low resolution.
#define MAX_NUM_LOD_GROUPS 4

-- radix.bucket
#ifndef RADIX_BUCKET_included
#define2 RADIX_BUCKET_included
uint radixBucket(uint key) {
    // Get the bucket for the given key and bit offset.
    // e.g. in case of 4-bit sort, this will be 0-15.
    return (key >> radixBitOffset) & RADIX_ONE_LESS_NUM_BUCKETS;
}
#endif

-- radix.histogram
#ifndef RADIX_HISTOGRAM_included
#define2 RADIX_HISTOGRAM_included
uint radixHistogramIndex(uint bucket, uint workGroup) {
    return bucket * CS_NUM_WORK_GROUPS_X + workGroup;
}
#endif

--------------
------ Takes a model matrix and a camera position and computes the distance
------ between the camera and the model.
------ Based on this, it counts how many instances are visible in each LOD group.
------ It also outputs the number of visible instances per workgroup.
--------------
-- radix.cull.cs
#include regen.stages.compute.defines
#include regen.stages.compute.readPosition
#include regen.shapes.lod.defines

// - [write] distance sort keys, computed by culling pass, per instance
buffer uint in_keys[];
// - [write] One per LOD group: how many valid instances passed culling
buffer uint in_lodGroupSize[];
// - number of visible instances in each LOD group (per workgroup)
shared uint sh_lodGroupSize[MAX_NUM_LOD_GROUPS];
// LOD distance thresholds.
// for 2 LOD levels e.g.:
// - [50]       --> [0,0,50]  --> <(0-0),(0-0),(0-50),(50-)>
// for 3 LOD levels e.g.:
// - [20,40]    --> [0,20,40] --> <(0-0),(0-20),(20-40),(40-)>
// for 4 LOD levels e.g.:
// - [20,40,60]               --> <(0-20),(20-40),(40-60),(60-)>
//
uniform vec3 in_lodThresholds;

int getLODGroup(float depth) {
    // Returns the LOD group for a given depth.
    return int(depth >= in_lodThresholds.x)
         + int(depth >= in_lodThresholds.y)
         + int(depth >= in_lodThresholds.z);
}

#ifdef USE_CULLING
#include regen.shapes.culling.isShapeVisible
#endif

float countLOD(vec3 pos) {
    float depth = length(pos - in_cameraPosition.xyz);
#ifdef RADIX_REVERSE_SORT
    // Reverse sort: smaller depth = higher LOD.
    // Note: we must use positive numbers for the uint conversion.
    depth = FLT_MAX - depth;
#endif
    // increment the LOD group size
    atomicAdd(sh_lodGroupSize[getLODGroup(depth)], 1);
    return depth;
}

void main() {
    uint globalID = gl_GlobalInvocationID.x;
    uint localID  = gl_LocalInvocationID.x;
    uint groupID  = gl_WorkGroupID.x;
    float depth = 0.0;
#ifdef USE_CULLING
    bool l_visible = false;
#endif

    // Initialize memory
    if (localID < MAX_NUM_LOD_GROUPS) {
        sh_lodGroupSize[localID] = 0;
    }
    barrier();

    if (globalID < LOD_NUM_INSTANCES) {
        vec3 pos = readPosition(globalID);
#ifdef USE_CULLING
        l_visible = isShapeVisible(globalID, pos);
        if (l_visible) {
            depth = countLOD(pos);
        }
#else
        depth = countLOD(pos);
#endif
    }
    barrier();

    // Write results to global memory
    if (localID < MAX_NUM_LOD_GROUPS) {
        atomicAdd(in_lodGroupSize[localID], sh_lodGroupSize[localID]);
    }
    if (globalID < LOD_NUM_INSTANCES) {
        // NOTE: For culled instances we use FLT_MAX as depth value for the sort key,
        //       effectively putting them at the end of the list.
        // TODO: Consider doing a compaction pass to remove culled instances, then use
        //       the compacted buffer as input for sort. But currently num instances is baked into shader,
        //       would need to be replaced by uniform. Compaction would be a kind of rough sort, so we
        //       could use existing global memory for doing this trivially (i.e. adding instance IDs to the
        //       output buffer only if they are visible, then mapping the count to CPU memory, etc.)
#ifdef USE_CULLING
    #ifdef RADIX_REVERSE_SORT
        in_keys[globalID] = (l_visible ? floatBitsToUint(depth) : floatBitsToUint(0.0f));
    #else
        in_keys[globalID] = (l_visible ? floatBitsToUint(depth) : floatBitsToUint(FLT_MAX));
    #endif
#else // No culling, so we can use the depth directly.
        in_keys[globalID] = floatBitsToUint(depth);
#endif
        in_instanceIDMap[globalID] = globalID;
    }
}

--------------
------ This is a pass in radix sort that computes a global histogram
------ for the keys in the input buffer. It uses shared memory to
------ compute local histograms for each workgroup and then writes
------ the results to a global histogram buffer.
------ The buffer has size NUM_BUCKETS * NUM_WORK_GROUPS, and each workgroup
------ has its own histogram for each bucket.
--------------
-- radix.histogram.cs
#include regen.stages.compute.defines
#include regen.shapes.lod.defines

// - [write] the global histogram, output will reflect the number of elements in each bucket and workgroup.
//           size: RADIX_NUM_BUCKETS * RADIX_NUM_WORK_GROUPS
buffer uint in_globalHistogram[];
// - [read] the sort keys, computed by culling pass, one per instance.
buffer uint in_keys[LOD_NUM_INSTANCES];
// - [read] The value input buffer, either [0...(LOD_NUM_INSTANCES-1)] or output from the previous pass.
layout(std430) readonly buffer ValueBuffer {
    uint in_values[LOD_NUM_INSTANCES];
};
// The local histogram. Counts bucket sizes in each workgroup.
shared uint sh_bucketSize[RADIX_NUM_BUCKETS];
// The bit offset of the current radix pass.
uniform uint radixBitOffset;

#include regen.shapes.lod.radix.bucket
#include regen.shapes.lod.radix.histogram

void main() {
    uint globalID = gl_GlobalInvocationID.x;
    uint localID = gl_LocalInvocationID.x;
    uint groupID = gl_WorkGroupID.x;
    // Initialize memory
    if (localID < RADIX_NUM_BUCKETS) {
        // Note: here we use localID as bucket index
        sh_bucketSize[localID] = 0;
        // also clear the global histogram. note that each work groups clears its own slots.
        // and should not interfere with other work group slots.
        in_globalHistogram[radixHistogramIndex(localID, groupID)] = 0;
    }
    barrier();
    // Compute locale histogram
    if (globalID < LOD_NUM_INSTANCES) {
        uint value = in_values[globalID];
        uint key = in_keys[value];
        // Atomically increment bin count
        atomicAdd(sh_bucketSize[radixBucket(key)], 1);
    }
    barrier();
    // Write histogram data to global memory. Only write into slots that belong to this workgroup.
    if (localID < RADIX_NUM_BUCKETS) {
        // Note: here we use localID as bucket index
        uint h_i = radixHistogramIndex(localID, groupID);
        // Atomically accumulate across all workgroups
        atomicAdd(in_globalHistogram[h_i], sh_bucketSize[localID]);
    }
}

--------------
--- Parallel scan: Use a parallel prefix sum algorithm to compute the prefix sum of the histogram.
--- Should be dispatchd with a single work group having a thread for each element in the histogram.
--- This will work for mid-sized histograms, as it is parallelized. However, the number of threads
--- per work group is limited.
--- The histogram size is NUM_WORK_GROUPS * NUM_BUCKETS.
--- NUM_WORK_GROUPS can get large, e.g. with 1.000.000 keys and work group size of 256,
--- we have 3907 work groups, for 4bit radix sort we have 16 buckets,
--- so the histogram size is 3907 * 16 = 62512 times 32-bit which is ~250kB
--- and might be too big for shared memory which is often capped to 64kB.
--- So as an estimate with group size 256, only use shared memory sum up to ~100.000 keys,
--- and with group size 512, only use shared memory sum up to ~200.000 keys.
--------------
-- radix.offsets.parallel.cs
#include regen.stages.compute.defines
#include regen.shapes.lod.defines
// - [read/write] the global histogram. NOTE: input should be counts, output will be (global) offsets.
buffer uint in_globalHistogram[];
shared uint sh_offsets[RADIX_NUM_THREADS];

void main() {
    uint tid = gl_LocalInvocationID.x;

    // Load input into shared memory
    if (tid < RADIX_HISTOGRAM_SIZE) {
        sh_offsets[tid] = in_globalHistogram[tid];
    } else {
        sh_offsets[tid] = 0; // pad with 0s if over histogram size
    }
    barrier();

    // === Upsweep (reduce) ===
    for (uint offset = 1; offset < RADIX_NUM_THREADS; offset *= 2) {
        uint index = (tid + 1) * offset * 2 - 1;
        if (index < RADIX_NUM_THREADS) {
            sh_offsets[index] += sh_offsets[index - offset];
        }
        barrier();
    }

    // === Set last element to zero (for exclusive scan) ===
    if (tid == 0) {
        sh_offsets[RADIX_NUM_THREADS - 1] = 0;
    }
    barrier();

    // === Downsweep ===
    for (uint offset = RADIX_NUM_THREADS / 2; offset > 0; offset /= 2) {
        uint index = (tid + 1) * offset * 2 - 1;
        if (index < RADIX_NUM_THREADS) {
            uint t = sh_offsets[index - offset];
            sh_offsets[index - offset] = sh_offsets[index];
            sh_offsets[index] += t;
        }
        barrier();
    }

    // Write result back
    if (tid < RADIX_HISTOGRAM_SIZE) {
        in_globalHistogram[tid] = sh_offsets[tid];
    }
}

--------------
--- Serial scan: Use a single thread to compute the prefix sum of the histogram.
--- This should only be used for small histograms, as it is not parallelized.
--------------
-- radix.offsets.serial.cs
#include regen.stages.compute.defines
#include regen.shapes.lod.defines
// - [read/write] the global histogram. NOTE: input should be counts, output will be (global) offsets.
buffer uint in_globalHistogram[];

void main() {
    uint globalID = gl_GlobalInvocationID.x;
    if (globalID == 0) {
        uint sum = 0;
        for (uint i = 0; i < RADIX_HISTOGRAM_SIZE; ++i) {
            // Compute the prefix sum of the histogram
            uint h_i = in_globalHistogram[i];
            in_globalHistogram[i] = sum;
            sum += h_i;
        }
    }
}

--------------
--- Each workgroup scans WG_SIZE elements and:
---     - Writes scanned output to in_globalHistogram.
---     - Writes total sum of block to a blockSums[] array.
--------------
-- radix.offsets.local.cs
//layout(local_size_x = WG_SIZE) in;
#include regen.stages.compute.defines
#include regen.shapes.lod.defines

// The global histogram, it reflects global offsets for each bucket and workgroup.
buffer uint in_globalHistogram[];
buffer uint in_blockSums[];
shared uint sh_temp[CS_LOCAL_SIZE_X];

void main() {
    uint tid = gl_LocalInvocationID.x;
    uint gid = gl_GlobalInvocationID.x;
    uint groupID = gl_WorkGroupID.x;

    uint index = groupID * CS_LOCAL_SIZE_X + tid;

    // Load to shared memory
    sh_temp[tid] = (index < RADIX_HISTOGRAM_SIZE) ? in_globalHistogram[index] : 0;
    barrier();

    // Upsweep (reduce)
    for (uint offset = 1; offset < CS_LOCAL_SIZE_X; offset <<= 1) {
        uint i = (tid + 1) * offset * 2 - 1;
        if (i < CS_LOCAL_SIZE_X)
            sh_temp[i] += sh_temp[i - offset];
        barrier();
    }

    if (tid == 0) {
        // Save total sum
        in_blockSums[groupID] = sh_temp[CS_LOCAL_SIZE_X - 1];
        sh_temp[CS_LOCAL_SIZE_X - 1] = 0;
    }
    barrier();

    // Downsweep
    for (uint offset = CS_LOCAL_SIZE_X >> 1; offset > 0; offset >>= 1) {
        uint i = (tid + 1) * offset * 2 - 1;
        if (i < CS_LOCAL_SIZE_X) {
            uint t = sh_temp[i - offset];
            sh_temp[i - offset] = sh_temp[i];
            sh_temp[i] += t;
        }
        barrier();
    }

    // Store back result
    if (index < RADIX_HISTOGRAM_SIZE)
        in_globalHistogram[index] = sh_temp[tid];
}

-- radix.offsets.global.cs
// NUM_BLOCKS = ceil(HISTOGRAM_SIZE / WG_SIZE)
// layout(local_size_x = BLOCK_SUMS_SIZE) in;
#include regen.stages.compute.defines
#include regen.shapes.lod.defines

buffer uint in_blockSums[];
buffer uint in_blockOffsets[];
shared uint sh_temp[CS_LOCAL_SIZE_X];

void main() {
    uint tid = gl_LocalInvocationID.x;

    sh_temp[tid] = in_blockSums[tid];
    barrier();

    for (uint offset = 1; offset < CS_LOCAL_SIZE_X; offset <<= 1) {
        uint i = (tid + 1) * offset * 2 - 1;
        if (i < CS_LOCAL_SIZE_X)
            sh_temp[i] += sh_temp[i - offset];
        barrier();
    }

    if (tid == 0)
        sh_temp[CS_LOCAL_SIZE_X - 1] = 0;
    barrier();

    for (uint offset = CS_LOCAL_SIZE_X >> 1; offset > 0; offset >>= 1) {
        uint i = (tid + 1) * offset * 2 - 1;
        if (i < CS_LOCAL_SIZE_X) {
            uint t = sh_temp[i - offset];
            sh_temp[i - offset] = sh_temp[i];
            sh_temp[i] += t;
        }
        barrier();
    }

    in_blockOffsets[tid] = sh_temp[tid];
}

-- radix.offsets.distribute.cs
//layout(local_size_x = WG_SIZE) in;
#include regen.stages.compute.defines
#include regen.shapes.lod.defines
// The global histogram, it reflects global offsets for each bucket and workgroup.
buffer uint in_globalHistogram[];
buffer uint in_blockOffsets[];

void main() {
    uint tid = gl_LocalInvocationID.x;
    uint groupID = gl_WorkGroupID.x;

    uint index = groupID * CS_LOCAL_SIZE_X + tid;
    if (index >= RADIX_HISTOGRAM_SIZE) return;

    uint offset = in_blockOffsets[groupID];
    in_globalHistogram[index] += offset;
}

--------------
------ Radix scattering stage. This shader takes the sorted keys and values from the previous pass
------ and scatters them into their final positions in the output buffer based on the computed offsets.
--------------
-- radix.scatter.cs
#include regen.stages.compute.defines
#include regen.shapes.lod.defines

// The global histogram, it reflects global offsets for each bucket and workgroup.
buffer uint in_globalHistogram[];
// The sort keys, computed by culling pass, one per instance.
buffer uint in_keys[LOD_NUM_INSTANCES];
// The value input buffer, either [0...(LOD_NUM_INSTANCES-1)] or output from the previous pass.
layout(std430) readonly buffer ReadBuffer {
    uint in_lastValues[LOD_NUM_INSTANCES];
};
// The output buffer, where the sorted values will be written to.
layout(std430) writeonly buffer WriteBuffer {
    uint in_nextValues[LOD_NUM_INSTANCES];
};
// The bit offset of the current radix pass.
uniform uint radixBitOffset;
// Prefix sum results
shared uint sh_scan[CS_LOCAL_SIZE_X];

#include regen.shapes.lod.radix.bucket
#include regen.shapes.lod.radix.histogram

void scatterBucket(uint b, uint t_value, uint t_bucket) {
    uint localID = gl_LocalInvocationID.x;
    uint groupID = gl_WorkGroupID.x;

    sh_scan[localID] = uint(t_bucket == b);
    barrier();

    // - Parallel scan (O(log n)) to compute the prefix sum of the local histogram.
    for (uint offset = 1; offset < CS_LOCAL_SIZE_X; offset <<= 1) {
        uint temp = (localID >= offset) ? sh_scan[localID - offset] : 0;
        barrier();
        sh_scan[localID] += temp;
        barrier();
    }

    // - Scatter if in this bucket
    if (t_bucket == b) {
        uint localOffset = sh_scan[localID] - 1;
        uint histogramIndex = radixHistogramIndex(b, groupID);
        uint scatterIndex = in_globalHistogram[histogramIndex] + localOffset;
        in_nextValues[scatterIndex] = t_value;
    }
}

void main() {
    uint globalID = gl_GlobalInvocationID.x;
    if (globalID >= LOD_NUM_INSTANCES) return;

    // Read key/value input
    uint value = in_lastValues[globalID];
    uint key = in_keys[value];
    // Compute the bucket for this thread
    uint bucket = radixBucket(key);
    // Process each bucket individually
#for BUCKET_I to RADIX_NUM_BUCKETS
    scatterBucket(${BUCKET_I}, value, bucket);
    barrier();
#endfor
}
