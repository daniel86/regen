--------------
----- Computes LOD level over distance to a camera.
----- Buffers:
----- in_sortKeys[]	SSBO	Input keys for sorting
----- in_instanceIDMap[]	SSBO	Final output: sorted, culled IDs
----- sortedIDsTemp[]	SSBO	Temporary: local group sort output
----- groupSizes[]	SSBO	Output count per workgroup
----- groupOffsets[]	SSBO	Global write offset per workgroup
-----
--------------
----- Each workgroup:
----- - Computes the key (e.g., depth)
----- - Performs culling (LOD, frustum, visibility test, etc.)
----- - If the instance passes:
-----   - Writes it into local shared memory (for sorting)
-----   - Marks it for inclusion in local histogram
----- - Runs block-wise radix sort (like you're already doing)
----- - Writes sorted IDs to a temporary global buffer
----- - Counts valid instances per workgroup, stores to groupSizes[groupID]
--------------
-- cull-sort.cs
#include regen.stages.compute.defines
#define RADIX_BITS 4
// Output Buffers
// - distance sort keys, computed by culling pass
buffer uint in_sortedKeys[];
// - intermediate output
buffer uint in_sortedIDsTemp[];
// - One per workgroup: how many valid instances passed culling
buffer uint in_groupSizes[];

shared bool sh_visible[WORKGROUP_SIZE];

///////////////////////////////
////////// Culling //////////
///////////////////////////////

bool cull(uint globalID) {
    vec3 pos = in_modelMatrix[globalID][3].xyz;
    // TODO: add real frustum check
    bool visible = true;
    if (visible) {
        float depth = length(pos - in_cameraPos);
        in_sortKeys[globalID] = floatBitsToUint(depth);
    } else {
        // Mark as invalid
        in_sortKeys[globalID] = 0xFFFFFFFFu;
    }
}

///////////////////////////////
////////// Sorting //////////
///////////////////////////////

shared uint sh_sortedKeys[WORKGROUP_SIZE];
shared uint sh_sortedIDs[WORKGROUP_SIZE];
shared uint sh_tempKeys[WORKGROUP_SIZE];
shared uint sh_tempIDs[WORKGROUP_SIZE];
shared uint sh_histogram[BUCKETS];

void loadKeysToSharedMemory(uint globalID, uint groupID, uint localID) {
    // Load keys and instance IDs into shared memory
    if (globalID < in_sortKeys.length()) {
        sh_sortedKeys[localID] = in_sortKeys[globalID];
        sh_sortedIDs[localID] = globalID;
    } else {
        sh_sortedKeys[localID] = 0xFFFFFFFFu;
        sh_sortedIDs[localID] = 0xFFFFFFFFu;
    }
}

// This function counts how many keys fall into each
// radix bucket (e.g., 0 and 1 for 1-bit sort)
void computeLocalHistograms(uint bitOffset, uint localID) {
    // Initialize histogram
    for (uint i = 0; i < BUCKETS; ++i) {
        sh_histogram[i] = 0;
    }
    barrier();
    // Compute histogram
    uint key = sh_sortedKeys[localID];
    if (key != 0xFFFFFFFFu) {
        // Each thread bins its key
        uint bucket = (key >> bitOffset) & (BUCKETS - 1);
        // Atomically increment bin count
        atomicAdd(sh_histogram[bucket], 1);
    }
}

// This computes the prefix sum on sh_histogram[] and stores start indices for output buckets.
void exclusivePrefixSum(uint localID) {
    // NOTE: Only one thread needed for small BUCKETS
    // TODO: Use parallel prefix sum for larger BUCKETS
    if (localID == 0) {
        sh_bucketOffsets[0] = 0;
        for (uint i = 1; i < BUCKETS; ++i) {
            sh_bucketOffsets[i] = sh_bucketOffsets[i - 1] + sh_histogram[i - 1];
        }
    }
}

void reorderKeys(uint bitOffset, uint localID) {
    uint key = sh_sortedKeys[localID];
    uint id = sh_sortedIDs[localID];
    uint bucket = (key >> bitOffset) & (BUCKETS - 1);

    // Initialize insertion offsets from prefix sum
    if (localID < BUCKETS)
        sh_bucketInsertionOffsets[localID] = sh_bucketOffsets[localID];
    barrier();

    if (key != 0xFFFFFFFFu) {
        // Each thread gets position via atomic add
        uint targetIndex = atomicAdd(sh_bucketInsertionOffsets[bucket], 1);
        sh_tempKeys[targetIndex] = key;
        sh_tempIDs[targetIndex] = id;
    }
    barrier();

    // Copy back for next round
    sh_sortedKeys[localID] = sh_tempKeys[localID];
    sh_sortedIDs[localID] = sh_tempIDs[localID];
}

void blockRadixSort(uint globalID, uint groupID, uint localID) {
    // TODO: Skip sorting invalid entries
    // Loads a chunk of in_sortKeys[] (and optionally instance IDs) into shared memory.
    loadKeysToSharedMemory(globalID, groupID, localID);
    // performs radix sort in chunks of RADIX_BITS bits
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
    in_sortedIDsTemp[groupID * WORKGROUP_SIZE + localID] = sh_sortedIDs[localID];
}

uint countVisibleInGroup() {
    uint count = 0;
    for (uint i = 0; i < WORKGROUP_SIZE; ++i) {
        if (sh_visible[i]) count++;
    }
    return count;
}

void main() {
    uint globalID = gl_GlobalInvocationID.x;
    uint localID  = gl_LocalInvocationID.x;
    uint groupID  = gl_WorkGroupID.x;
    // cull pass
    bool isVisible = cull(globalID);
    sh_visible[localID] = visible;

    blockRadixSort(globalID, groupID, localID);
    // At the end of each workgroup
    if (localID == 0) {
        // TODO: implement countVisibleInGroup
        uint count = countVisibleInGroup();
        in_groupSizes[groupID] = count;
    }
}

--------------
----- This small pass takes groupSizes[] and runs a prefix sum over it, to produce groupOffsets[].
--------------
-- prefix-sum.cs
// Input Buffers
buffer uint in_groupSizes[];
// Output Buffers
buffer uint in_groupOffsets[];
buffer uint in_totalVisibleCount;

void main() {
    uint sum = 0;
    for (uint i = 0; i < in_groupSizes.length(); ++i) {
        in_groupOffsets[i] = sum;
        sum += in_groupSizes[i];
    }
    in_totalVisibleCount = sum;
}

--------------
-----
--------------
-- scatter.cs
#include regen.stages.compute.defines

// Input Buffers
buffer uint in_sortKeys[];
buffer uint in_sortedIDsTemp[];
buffer uint in_groupOffsets[];
// Output Buffers
buffer uint in_instanceIDMap[];

void main() {
    uint globalID = gl_GlobalInvocationID.x;
    uint localID  = gl_LocalInvocationID.x;
    uint groupID  = gl_WorkGroupID.x;

    // Fetch the original instance ID for this thread
    uint instanceID = in_sortedIDsTemp[globalID];
    uint sortKey    = in_sortKeys[instanceID];

    // Only write if valid
    if (sortKey != 0xFFFFFFFFu) {
        uint baseOffset = in_groupOffsets[groupID];
        uint writeIndex = baseOffset + localID;
        in_instanceIDMap[writeIndex] = instanceID;
    }
}
