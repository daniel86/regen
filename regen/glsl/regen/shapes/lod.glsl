// This shader is used for dynamically computing the LOD of objects in an
// input array. It performs a culling pass to determine which objects are visible
// and computes LOD level based on their distance to the camera.
// The shader uses a radix sort to sort the objects based on their
// computed keys (e.g., depth).

-- defines
// LOD groups: high to low resolution.
#define MAX_NUM_LOD_GROUPS 4

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

int getLODGroup(float squaredDistance) {
    // Returns the LOD group for a given depth.
    return int(squaredDistance >= in_lodThresholds.x)
         + int(squaredDistance >= in_lodThresholds.y)
         + int(squaredDistance >= in_lodThresholds.z);
}

#ifdef USE_CULLING
#include regen.shapes.culling.isShapeVisible
#endif

float countLOD(vec3 pos) {
    vec3 diff = pos - in_cameraPosition.xyz;
    float depthSquared = dot(diff, diff);
#ifdef USE_REVERSE_SORT
    // Reverse sort: smaller depth = higher LOD.
    // Note: we must use positive numbers for the uint conversion.
    depthSquared = FLT_MAX - depthSquared;
#endif
    // increment the LOD group size
    atomicAdd(sh_lodGroupSize[getLODGroup(depthSquared)], 1);
    return depthSquared;
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
    #ifdef USE_REVERSE_SORT
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
