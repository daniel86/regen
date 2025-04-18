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
#define RADIX_NUM_BUCKETS (1 << RADIX_BITS)
// LOD groups: high resolution, medium resolution, low resolution.
#define MAX_NUM_LOD_GROUPS 4
#define LOD_NUM_INSTANCES in_sortKeys.length()

--------------
--------------
-- compute.cs
#include regen.stages.compute.defines
#include regen.stages.compute.readPosition
#include regen.shapes.lod.defines

// FIXME something is wrong with parallel offset sum.
//#define USE_PARALLEL_OFFSET_SUM

////////////////////////
// Temporary Buffers
////////////////////////
// - distance sort keys, computed by culling pass, per instance
buffer uint in_sortKeys[];
// - intermediate sorted output of workgroups, each element is inserted into the
//   final output with a work group offset.
buffer uint in_sortedIDsTemp[];
// - Write from each workgroup how many visible instances it has
buffer uint in_workGroupSize[CS_GROUP_SIZE_X];
buffer uint in_workGroupOffset[CS_GROUP_SIZE_X];

////////////////////////
// Output Buffers
////////////////////////
// - The final sorted instance IDs
buffer uint in_instanceIDMap[];
// - One per LOD group: how many valid instances passed culling
//   NOTE: this buffer must be cleared to 0 before the dispatch on the CPU! (use glClearBufferSubData)
buffer uint in_lodGroupSize[];

////////////////////////
// Shared memory.
////////////////////////
// This is shared among all threads in a workgroup.
shared bool sh_visible[CS_LOCAL_SIZE_X];
shared uint sh_visibleCount;
shared uint sh_sortedKeys[CS_LOCAL_SIZE_X];
shared uint sh_sortedIDs[CS_LOCAL_SIZE_X];
shared uint sh_tempKeys[CS_LOCAL_SIZE_X];
shared uint sh_tempIDs[CS_LOCAL_SIZE_X];
shared uint sh_histogram[RADIX_NUM_BUCKETS];
shared uint sh_bucketOffset[RADIX_NUM_BUCKETS];
shared uint sh_bucketInsertionOffset[RADIX_NUM_BUCKETS];
shared uint sh_lodGroupSize[MAX_NUM_LOD_GROUPS];

////////////////////////
// Uniforms
////////////////////////
// LOD distance thresholds.
// for 2 LOD levels e.g.:
// - [50]       --> [0,0,50]  --> <(0-0),(0-0),(0-50),(50-)>
// for 3 LOD levels e.g.:
// - [20,40]    --> [0,20,40] --> <(0-0),(0-20),(20-40),(40-)>
// for 4 LOD levels e.g.:
// - [20,40,60]               --> <(0-20),(20-40),(40-60),(60-)>
//
uniform vec3 in_lodThresholds;

///////////////////////////////
////////// Culling //////////
///////////////////////////////

bool cull(uint globalID, uint localID, uint groupID) {
    if (globalID == 0) {
        sh_visibleCount = 0;
    }
    barrier();
    bool isVisible;

    if (globalID < LOD_NUM_INSTANCES) {
        vec3 pos = readPosition(globalID);
        // TODO: add real frustum check
        isVisible = true;

        if (isVisible) {
            float depth = length(pos - in_cameraPosition.xyz);
            in_sortKeys[globalID] = floatBitsToUint(depth);
            // increase visibility count
            atomicAdd(sh_visibleCount, 1);
        } else {
            // mark as invalid
            in_sortKeys[globalID] = 0xFFFFFFFFu;
        }
    } else {
        isVisible = false;
    }
    barrier();

    // Each work group counts its visible instances
    if (localID == 0) {
        in_workGroupSize[groupID] = sh_visibleCount;
    }
    return isVisible;
}

///////////////////////////////
////////// Sorting //////////
///////////////////////////////

// This function counts how many keys fall into each radix bucket (e.g., 0 and 1 for 1-bit sort).
// The write location of each bucket is the start index of the bucket in the output array,
// which is determined by the prefix sum of the histogram (computed below).
void computeLocalHistograms(uint bitOffset, uint localID) {
    // Initialize histogram
    if (localID < RADIX_NUM_BUCKETS) {
        sh_histogram[localID] = 0;
    }
    barrier();
    // Compute histogram
    uint key = sh_sortedKeys[localID];
    if (key != 0xFFFFFFFFu) {
        // Each thread bins its key
        uint bucket = (key >> bitOffset) & uint(RADIX_NUM_BUCKETS - 1);
        // Atomically increment bin count
        atomicAdd(sh_histogram[bucket], 1);
    }
}

// This computes the prefix sum on sh_histogram[] and stores start indices for output buckets.
// The offset of a bucket is trivially the sum of elements before it.
void exclusivePrefixSum(uint localID) {
    // NOTE: Only one thread needed for small NUM_BUCKETS (<32), here we have 16.
    if (localID == 0) {
        sh_bucketOffset[0] = 0;
        for (uint i = 1; i < RADIX_NUM_BUCKETS; ++i) {
            sh_bucketOffset[i] = sh_bucketOffset[i - 1] + sh_histogram[i - 1];
        }
    }
}

#ifdef USE_PARALLEL_OFFSET_SUM
shared uint sh_offsetScan[CS_GROUP_SIZE_X];
void computeOffsetsParallel(uint globalID, uint localID) {
    uint temp = 0;

    // Load input from global buffer (e.g. in_workGroupSize) to shared memory
    sh_offsetScan[localID] = in_workGroupSize[localID];
    barrier();

    // Up-sweep / reduce phase
    for (uint offset = 1; offset < CS_GROUP_SIZE_X; offset <<= 1) {
        if (localID >= offset) {
            temp = sh_offsetScan[localID - offset];
        }
        barrier();  // Wait for all threads
        if (localID >= offset) {
            sh_offsetScan[localID] += temp;
        }
        barrier();
    }

    // Convert to exclusive scan
    if (localID == 0) {
        sh_offsetScan[CS_GROUP_SIZE_X - 1] = 0;
    }
    barrier();

    // Down-sweep phase
    //for (uint offset = CS_GROUP_SIZE_X >> 1; offset >= 1; offset >>= 1) {
    for (uint offset = CS_GROUP_SIZE_X >> 1; offset > 0; offset >>= 1) {
        if (localID >= offset) {
            temp = sh_offsetScan[localID - offset];
        }
        barrier();
        if (localID >= offset) {
            uint t = sh_offsetScan[localID];
            sh_offsetScan[localID] = t + temp;
        }
        barrier();
    }

    // Write to output
    if (localID < CS_GROUP_SIZE_X) {
        in_workGroupOffset[localID] = sh_offsetScan[localID];
    }
}
#endif

void reorderKeys(uint bitOffset, uint localID) {
    uint key = sh_sortedKeys[localID];
    uint id = sh_sortedIDs[localID];
    // Find the right bucket for this key.
    // e.g. in case of 4-bit sort, this will be 0-15.
    uint bucket = (key >> bitOffset) & uint(RADIX_NUM_BUCKETS - 1);

    // Initialize insertion offset from prefix sum
    if (localID < RADIX_NUM_BUCKETS)
        sh_bucketInsertionOffset[localID] = sh_bucketOffset[localID];
    barrier();

    if (key != 0xFFFFFFFFu) {
        // Each thread gets position via atomic add
        uint targetIndex = atomicAdd(sh_bucketInsertionOffset[bucket], 1);
        sh_tempKeys[targetIndex] = key;
        sh_tempIDs[targetIndex] = id;
    }
    barrier();

    // Copy back for next round
    if (key != 0xFFFFFFFFu) {
        sh_sortedKeys[localID] = sh_tempKeys[localID];
        sh_sortedIDs[localID] = sh_tempIDs[localID];
    } else {
        sh_sortedKeys[localID] = 0xFFFFFFFFu;
        sh_sortedIDs[localID] = 0xFFFFFFFFu;
    }
}

void blockRadixSort(uint globalID, uint localID, uint groupID) {
    // Loads a segment of in_sortKeys[] (and optionally instance IDs) into shared memory.
    // The segment has size equal to the workgroup size, each group sorts its own segment.
    if (globalID < LOD_NUM_INSTANCES) {
        sh_sortedKeys[localID] = in_sortKeys[globalID];
        sh_sortedIDs[localID] = globalID;
    } else {
        sh_sortedKeys[localID] = 0xFFFFFFFFu;
        sh_sortedIDs[localID] = 0xFFFFFFFFu;
    }
    barrier();

    // Performs radix sort in chunks of RADIX_BITS bits.
    // With 32 bits and RADIX_BITS = 4, we have 8 passes.
    // Meaning that each iteration considers 4 bits of the key, the keys will be
    // added to 16 buckets each frame.
    for (uint bitOffset = 0; bitOffset < 32; bitOffset += RADIX_BITS) {
        // Builds a histogram of key digit values for the current radix bit range.
        computeLocalHistograms(bitOffset, localID);
        barrier();
        // Computes prefix sums of the histogram — needed to determine final write locations.
        exclusivePrefixSum(localID);
        barrier();
        // Using the prefix sums, writes sorted elements back into a temporary shared array.
        reorderKeys(bitOffset, localID);
        barrier();
    }

    // store to temp buffer
    if (globalID < LOD_NUM_INSTANCES) {
        in_sortedIDsTemp[globalID] = sh_sortedIDs[localID];
    }
}

void countVisibleInLOD(uint localID) {
    if (localID < MAX_NUM_LOD_GROUPS) {
        sh_lodGroupSize[localID] = 0;
    }
    barrier();

    // Only count visible instances, i.e. those that passed culling
    if (sh_visible[localID]) {
        uint key = sh_sortedKeys[localID];
        // convert back to float
        float depth = uintBitsToFloat(key);
        // compute the LOD group
        uint lodGroup;
        if (depth < in_lodThresholds.x) {
            lodGroup = 0;
        } else if (depth < in_lodThresholds.y) {
            lodGroup = 1;
        } else if (depth < in_lodThresholds.z) {
            lodGroup = 2;
        } else {
            lodGroup = 3;
        }
        // increment the group size
        atomicAdd(sh_lodGroupSize[lodGroup], 1);
    }
    barrier();

    if (localID < MAX_NUM_LOD_GROUPS) {
        // atomic global accumulation from shared count
        atomicAdd(in_lodGroupSize[localID], sh_lodGroupSize[localID]);
    }
}

void main() {
    uint globalID = gl_GlobalInvocationID.x;
    uint localID  = gl_LocalInvocationID.x;
    uint groupID  = gl_WorkGroupID.x;
    // cull pass
    bool isVisible = cull(globalID, localID, groupID);
    if (globalID < LOD_NUM_INSTANCES) {
        sh_visible[localID] = isVisible;
    } else {
        sh_visible[localID] = false;
    }
    barrier();

    blockRadixSort(globalID, localID, groupID);
    barrier();

    countVisibleInLOD(localID);
    barrier();

    // Compute a global prefix sum of groupSize[] to get groupOffset[]
    if (globalID == 0) {
#ifdef USE_PARALLEL_OFFSET_SUM
        // parallel prefix sum of work group offsets
        computeOffsetsParallel(globalID, localID);
#else
        // serial prefix sum of work group offsets
        in_workGroupOffset[0] = 0;
        for (uint i = 1; i < CS_GROUP_SIZE_X; ++i) {
            in_workGroupOffset[i] = in_workGroupOffset[i - 1] + in_workGroupSize[i - 1];
        }
#endif
    }
    barrier();

    if (globalID < LOD_NUM_INSTANCES) {
        // Fetch the instance ID for this thread
        uint instanceID = in_sortedIDsTemp[globalID];
        uint sortKey    = in_sortKeys[instanceID];
        // Only write if valid
        if (sortKey != 0xFFFFFFFFu) {
            in_instanceIDMap[in_workGroupOffset[groupID] + localID] = instanceID;
        }
    }
}
