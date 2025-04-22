// This shader is used for dynamically computing the LOD of objects in an
// input array. It performs a culling pass to determine which objects are visible
// and computes LOD level based on their distance to the camera.
// The shader uses a block-wise radix sort to sort the objects based on their
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
// - number of visible instances (per workgroup)
shared uint sh_visibleCount;
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

void main() {
    uint globalID = gl_GlobalInvocationID.x;
    uint localID  = gl_LocalInvocationID.x;
    uint groupID  = gl_WorkGroupID.x;
    float depth = 0.0;
    bool isCulled = true;

    // Initialize memory
    if (localID < MAX_NUM_LOD_GROUPS) {
        sh_lodGroupSize[localID] = 0;
    }
    if (localID == 0) {
        sh_visibleCount = 0;
    }
    barrier();

    if (globalID < LOD_NUM_INSTANCES) {
        vec3 pos = readPosition(globalID);
        // TODO: add real frustum check
        isCulled = false;

        if (!isCulled) {
            depth = length(pos - in_cameraPosition.xyz);
            // increase visibility count
            atomicAdd(sh_visibleCount, 1);
            // increment the LOD group size
            atomicAdd(sh_lodGroupSize[getLODGroup(depth)], 1);
        }
    }
    barrier();

    // Atomic global accumulation from shared count
    if (localID < MAX_NUM_LOD_GROUPS) {
        atomicAdd(in_lodGroupSize[localID], sh_lodGroupSize[localID]);
    }
    // Write sort key into global memory
    if (!isCulled) {
        in_keys[globalID] = floatBitsToUint(depth);
    } else {
        in_keys[globalID] = 0xFFFFFFFFu;
    }
    // Also initialize the value buffer to [0...(LOD_NUM_INSTANCES-1)]
    if (globalID < LOD_NUM_INSTANCES) {
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
        if (key != 0xFFFFFFFFu) {
            // Atomically increment bin count
            uint bucket = radixBucket(key);
            atomicAdd(sh_bucketSize[bucket], 1);
        }
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
------ This shader computes the offsets for each bucket and workgroup in the global
------ memory. It is used to determine the write locations for the sorted keys in the radix sort pass.
--------------
-- radix.offsets.cs
#include regen.stages.compute.defines
#include regen.shapes.lod.defines

// - [read/write] the global histogram. NOTE: input should be counts, output will be (global) offsets.
//           size: RADIX_NUM_BUCKETS * NUM_WORK_GROUPS
buffer uint in_globalHistogram[];

void main() {
    uint globalID = gl_GlobalInvocationID.x;
    if (globalID == 0) {
        uint sum = 0;
        for (uint i = 0; i < RADIX_NUM_BUCKETS * RADIX_NUM_WORK_GROUPS; ++i) {
            // Compute the prefix sum of the histogram
            uint h_i = in_globalHistogram[i];
            in_globalHistogram[i] = sum;
            sum += h_i;
        }
    }
}

--------------
------ Radix scattering stage. This shader takes the sorted keys and values from the previous pass
------ and scatters them into their final positions in the output buffer based on the computed offsets.
--------------
-- radix.scatter.cs
#include regen.stages.compute.defines
#include regen.shapes.lod.defines

// - [read] the global histogram, it reflects global offsets for each bucket and workgroup.
//           size: RADIX_NUM_BUCKETS * NUM_WORK_GROUPS
buffer uint in_globalHistogram[];
// - [read] the sort keys, computed by culling pass, one per instance.
buffer uint in_keys[LOD_NUM_INSTANCES];
// - [read] The value input buffer, either [0...(LOD_NUM_INSTANCES-1)] or output from the previous pass.
layout(std430) readonly buffer ReadBuffer {
    uint in_lastValues[LOD_NUM_INSTANCES];
};
// - [write] The output buffer, where the sorted values will be written to.
layout(std430) writeonly buffer WriteBuffer {
    uint in_nextValues[LOD_NUM_INSTANCES];
};
// The bit offset of the current radix pass.
uniform uint radixBitOffset;

shared uint sh_bucket[CS_LOCAL_SIZE_X];     // Each thread's bucket
shared uint sh_flags[CS_LOCAL_SIZE_X];      // Per-bucket flag
shared uint sh_scan[CS_LOCAL_SIZE_X];       // Prefix sum results

#include regen.shapes.lod.radix.bucket
#include regen.shapes.lod.radix.histogram

void main() {
    uint globalID = gl_GlobalInvocationID.x;
    uint localID = gl_LocalInvocationID.x;
    uint groupID = gl_WorkGroupID.x;

    if (globalID >= LOD_NUM_INSTANCES)
        return;

    // Read key/value input
    uint value = in_lastValues[globalID];
    uint key = in_keys[value];

    uint bucket = radixBucket(key);
    sh_bucket[localID] = bucket;
    barrier();

    // Process each bucket individually
    for (uint b = 0; b < RADIX_NUM_BUCKETS; ++b) {
        // Step 1: Set flags
        sh_flags[localID] = (sh_bucket[localID] == b) ? 1 : 0;
        barrier();

        // Step 2: Inclusive prefix sum over flags (naive scan)
        sh_scan[localID] = sh_flags[localID];
        for (uint offset = 1; offset < CS_LOCAL_SIZE_X; offset <<= 1) {
            uint temp = (localID >= offset) ? sh_scan[localID - offset] : 0;
            barrier();
            sh_scan[localID] += temp;
            barrier();
        }

        // Step 3: Scatter if in this bucket
        if (sh_bucket[localID] == b) {
            uint localOffset = sh_scan[localID] - 1;
            uint histogramIndex = radixHistogramIndex(b, groupID);
            uint scatterIndex = in_globalHistogram[histogramIndex] + localOffset;
            in_nextValues[scatterIndex] = value;
        }
        barrier();
    }
}
