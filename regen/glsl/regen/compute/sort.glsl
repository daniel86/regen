// This shader is used for dynamically computing the LOD of objects in an
// input array. It performs a culling pass to determine which objects are visible
// and computes LOD level based on their distance to the camera.
// The shader uses a radix sort to sort the objects based on their
// computed keys (e.g., depth).

-- radix.bucket
#ifndef RADIX_BUCKET_included
#define2 RADIX_BUCKET_included
uint radixBucket(uint key) {
    // Get the bucket for the given key and bit offset.
    // e.g. in case of 4-bit sort, this will be 0-15.
    return (key >> radixBitOffset) & ONE_LESS_NUM_RADIX_BUCKETS;
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
------ This is a pass in radix sort that computes a global histogram
------ for the keys in the input buffer. It uses shared memory to
------ compute local histograms for each workgroup and then writes
------ the results to a global histogram buffer.
------ The buffer has size NUM_BUCKETS * NUM_WORK_GROUPS, and each workgroup
------ has its own histogram for each bucket.
--------------
-- radix.histogram.cs
#include regen.stages.compute.defines

// - [write] the global histogram, output will reflect the number of elements in each bucket and workgroup.
//           size: NUM_RADIX_BUCKETS * RADIX_NUM_WORK_GROUPS
buffer uint in_globalHistogram[];
// - [read] the sort keys, computed by culling pass, one per instance.
buffer uint in_keys[NUM_SORT_KEYS];
// - [read] The value input buffer, either [0...(NUM_SORT_KEYS-1)] or output from the previous pass.
#ifdef RADIX_CONTIGUOUS_VALUE_BUFFERS
buffer uint in_values[NUM_SORT_KEYS];
uniform uint in_readOffset;
#else
layout(std430) readonly buffer ValueBuffer {
    uint in_values[NUM_SORT_KEYS];
};
#endif
// The local histogram. Counts bucket sizes in each workgroup.
shared uint sh_bucketSize[NUM_RADIX_BUCKETS];
// The bit offset of the current radix pass.
uniform uint radixBitOffset;

#include regen.compute.sort.radix.bucket
#include regen.compute.sort.radix.histogram

void main() {
    uint globalID = gl_GlobalInvocationID.x;
    uint localID = gl_LocalInvocationID.x;
    uint groupID = gl_WorkGroupID.x;
    // Initialize memory
    if (localID < NUM_RADIX_BUCKETS) {
        // Note: here we use localID as bucket index
        sh_bucketSize[localID] = 0;
        // also clear the global histogram. note that each work groups clears its own slots.
        // and should not interfere with other work group slots.
        in_globalHistogram[radixHistogramIndex(localID, groupID)] = 0;
    }
    barrier();
    // Compute local histogram
    if (globalID < NUM_SORT_KEYS) {
#ifdef RADIX_CONTIGUOUS_VALUE_BUFFERS
        uint value = in_values[globalID + in_readOffset];
#else
        uint value = in_values[globalID];
#endif
        uint key = in_keys[value];
        // Atomically increment bin count
        atomicAdd(sh_bucketSize[radixBucket(key)], 1);
    }
    barrier();
    // Write histogram data to global memory. Only write into slots that belong to this workgroup.
    if (localID < NUM_RADIX_BUCKETS) {
        // Note: here we use localID as bucket index
        uint h_i = radixHistogramIndex(localID, groupID);
        // Atomically accumulate across all workgroups
        atomicAdd(in_globalHistogram[h_i], sh_bucketSize[localID]);
    }
}

--------------
------ Radix scattering stage. This shader takes the sorted keys and values from the previous pass
------ and scatters them into their final positions in the output buffer based on the computed offsets.
--------------
-- radix.scatter.cs
#include regen.stages.compute.defines

// The global histogram, it reflects global offsets for each bucket and workgroup.
buffer uint in_globalHistogram[];
// The sort keys, computed by culling pass, one per instance.
buffer uint in_keys[NUM_SORT_KEYS];
#ifdef RADIX_CONTIGUOUS_VALUE_BUFFERS
buffer uint in_values[NUM_SORT_KEYS];
uniform uint in_readOffset;
uniform uint in_writeOffset;
#else
// The value input buffer, either [0...(LOD_NUM_INSTANCES-1)] or output from the previous pass.
layout(std430) readonly buffer ReadBuffer {
    uint in_lastValues[NUM_SORT_KEYS];
};
// The output buffer, where the sorted values will be written to.
layout(std430) writeonly buffer WriteBuffer {
    uint in_nextValues[NUM_SORT_KEYS];
};
#endif
// The bit offset of the current radix pass.
uniform uint radixBitOffset;
// Prefix sum results
shared uint sh_scan[CS_LOCAL_SIZE_X];

#include regen.compute.sort.radix.bucket
#include regen.compute.sort.radix.histogram

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
#ifdef RADIX_CONTIGUOUS_VALUE_BUFFERS
        in_values[scatterIndex + in_writeOffset] = t_value;
#else
        in_nextValues[scatterIndex] = t_value;
#endif
    }
}

void main() {
    uint globalID = gl_GlobalInvocationID.x;
    if (globalID >= NUM_SORT_KEYS) return;

    // Read key/value input
#ifdef RADIX_CONTIGUOUS_VALUE_BUFFERS
    uint value = in_values[globalID + in_readOffset];
#else
    uint value = in_lastValues[globalID];
#endif
    uint key = in_keys[value];
    // Compute the bucket for this thread
    uint bucket = radixBucket(key);
    // Process each bucket individually
#for BUCKET_I to NUM_RADIX_BUCKETS
    scatterBucket(${BUCKET_I}, value, bucket);
    barrier();
#endfor
}
