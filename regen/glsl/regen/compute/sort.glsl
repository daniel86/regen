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
    return (key >> radixBitOffset) & ${ONE_LESS_NUM_RADIX_BUCKETS}u;
}
#endif

-- radix.histogram
#ifndef RADIX_HISTOGRAM_included
#define2 RADIX_HISTOGRAM_included
uint radixHistogramIndex(uint layer, uint bucket, uint workGroup) {
    return layer * HISTOGRAM_SIZE +
        bucket * CS_NUM_WORK_GROUPS_X + workGroup;
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
//           size: NUM_LAYERS * HISTOGRAM_SIZE
buffer uint in_globalHistogram[];
// - [read] the sort keys, computed by culling pass, one per instance and layer.
//           size: NUM_LAYERS * NUM_SORT_KEYS
buffer uint in_keys[];
// - [read] The value input buffer, either [0...(NUM_SORT_KEYS-1)] or output from the previous pass.
//           size: NUM_LAYERS * NUM_SORT_KEYS
#ifdef RADIX_CONTIGUOUS_VALUE_BUFFERS
buffer uint in_values[];
uniform uint in_readOffset;
#else
layout(std430) readonly buffer ValueBuffer {
    uint in_values[];
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
    uint layer = regen_computeLayer;
#ifdef HAS_numVisibleKeys
    uint numSortKeys = in_numVisibleKeys[layer];
#else
    uint numSortKeys = NUM_SORT_KEYS;
#endif
    uint idx;

    // Initialize memory
    if (localID < NUM_RADIX_BUCKETS) {
        // Note: here we use localID as bucket index
        sh_bucketSize[localID] = 0;
        // also clear the global histogram. note that each work groups clears its own slots.
        // and should not interfere with other work group slots.
        idx = radixHistogramIndex(layer, localID, groupID);
        in_globalHistogram[idx] = 0;
    }
    barrier();

    // Compute local histogram
    if (globalID < numSortKeys) {
        uint layerOffset = layer * NUM_SORT_KEYS;
#ifdef RADIX_CONTIGUOUS_VALUE_BUFFERS
        uint value = in_values[layerOffset + globalID + in_readOffset];
#else
        uint value = in_values[layerOffset + globalID];
#endif
        uint key = in_keys[layerOffset + value];
        // Atomically increment bin count
        atomicAdd(sh_bucketSize[radixBucket(key)], 1);
    }
    barrier();

    // Write histogram data to global memory. Only write into slots that belong to this workgroup.
    if (localID < NUM_RADIX_BUCKETS) {
        // Note: here we use localID as bucket index
        idx = radixHistogramIndex(layer, localID, groupID);
        // Atomically accumulate across all workgroups
        atomicAdd(in_globalHistogram[idx], sh_bucketSize[localID]);
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
//         size: NUM_LAYERS * NUM_SORT_KEYS
buffer uint in_keys[];
#ifdef RADIX_CONTIGUOUS_VALUE_BUFFERS
// The value input buffer, either [0...(LOD_NUM_INSTANCES-1)] or output from the previous pass.
//          size: NUM_LAYERS * NUM_SORT_KEYS
buffer uint in_values[];
uniform uint in_readOffset;
uniform uint in_writeOffset;
#else
// The value input buffer, either [0...(LOD_NUM_INSTANCES-1)] or output from the previous pass.
//          size: NUM_LAYERS * NUM_SORT_KEYS
layout(std430) readonly buffer ReadBuffer {
    uint in_lastValues[];
};
// The output buffer, where the sorted values will be written to.
//          size: NUM_LAYERS * NUM_SORT_KEYS
layout(std430) writeonly buffer WriteBuffer {
    uint in_nextValues[];
};
#endif
// The bit offset of the current radix pass.
uniform uint radixBitOffset;
// Prefix sum results
shared uint sh_scan[CS_LOCAL_SIZE_X];

#include regen.compute.sort.radix.bucket
#include regen.compute.sort.radix.histogram

void scatterBucket(
        uint layer, uint layerOffset,
        uint b, uint t_value, uint t_bucket) {
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
        uint idx = radixHistogramIndex(layer, b, groupID);
        idx = layerOffset + in_globalHistogram[idx] + localOffset;
#ifdef RADIX_CONTIGUOUS_VALUE_BUFFERS
        in_values[idx + in_writeOffset] = t_value;
#else
        in_nextValues[idx] = t_value;
#endif
    }
}

void main() {
    uint globalID = gl_GlobalInvocationID.x;
    uint layer = regen_computeLayer; // 0..NUM_LAYERS-1
#ifdef HAS_numVisibleKeys
    if (globalID >= in_numVisibleKeys[layer]) return;
#else
    if (globalID >= NUM_SORT_KEYS) return;
#endif
    uint layerOffset = layer * NUM_SORT_KEYS;

    // Read key/value input
#ifdef RADIX_CONTIGUOUS_VALUE_BUFFERS
    uint value = in_values[layerOffset + globalID + in_readOffset];
#else
    uint value = in_lastValues[layerOffset + globalID];
#endif
    uint key = in_keys[layerOffset + value];
    // Compute the bucket for this thread
    uint bucket = radixBucket(key);

    // Process each bucket individually
#for BUCKET_I to NUM_RADIX_BUCKETS
    scatterBucket(layer, layerOffset, ${BUCKET_I}, value, bucket);
    barrier();
#endfor
}
