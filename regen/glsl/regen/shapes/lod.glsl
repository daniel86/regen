// This shader is used for dynamically computing the LOD of objects in an
// input array. It performs a culling pass to determine which objects are visible
// and computes LOD level based on their distance to the camera.
// The shader uses a radix sort to sort the objects based on their
// computed keys (e.g., depth).

-- DrawCommand
#ifndef INDIRECT_DRAW_DATA_included_
#define2 INDIRECT_DRAW_DATA_included_
struct DrawCommand {
    // storage for either DrawElements or DrawArrays
    // NOTE: baseVertex in elements mode is an integer! we cast it to uint to be able
    //       uniformly handle arrays and elements.
    uint data[5];
    // 0 = skip, 1 = elements, 2 = arrays
    uint drawMode;
    uint _pad[2];
};
#define INSTANCE_COUNT(cmd) cmd.data[1]
#endif

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
#input regen.shapes.lod.DrawCommand
#include regen.stages.compute.defines
#include regen.stages.compute.readPosition
#include regen.shapes.lod.defines

// - [write] distance sort keys, computed by culling pass, per instance
buffer uint in_keys[];
// - [write] One per LOD group: how many valid instances passed culling, and the base instance offset
buffer DrawCommand in_drawParams[];
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
#ifdef IS_ARRAY_cameraPosition
    vec3 diff = pos - in_cameraPosition[0].xyz;
#else
    vec3 diff = pos - in_cameraPosition.xyz;
#endif
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
        atomicAdd(INSTANCE_COUNT(in_drawParams[localID]), sh_lodGroupSize[localID]);
        // also compute the offset for each LOD group.
        // Note: this could be done via prefix scan in a separate pass, but it might not be worth it
        //       because we only have a few LOD groups (4 usually).
        for (uint i = localID + 1; i < MAX_NUM_LOD_GROUPS; ++i) {
            uint baseInstanceIdx = 5u - in_drawParams[i].drawMode;
            atomicAdd(in_drawParams[i].data[baseInstanceIdx], sh_lodGroupSize[localID]);
        }
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

--------------
------ Takes as input the indirect draw buffer of the culling pass, computed
------ for the first part in a component hierarchy.
------ Here we compute the indirect draw data for the other parts.
------ input: NUM_BASE_LODS defines the number of base LODs for the base mesh.
------ input: NUM_ATTACHED_PARTS defines the number of parts attached to the base mesh.
------ input: NUM_PART_LOD_{i} defines the number of LODs for part i.
------ input/output: in_drawParams{i} is the indirect draw data for the part with index i.
--------------
-- copy-indirect.cs
#input regen.shapes.lod.DrawCommand
#include regen.stages.compute.defines

uint getPartLevel(uint lodLevel, uint numPartLevels, uint numBaseLevels) {
    // Check conditions
    bool sameLevels = (numPartLevels == numBaseLevels);
    bool lodInRange = (lodLevel < numBaseLevels);
    bool case1 = sameLevels && lodInRange;
    bool case2 = (numPartLevels == 1u);
    bool case3 = (numPartLevels < numBaseLevels) || (lodLevel >= numPartLevels);

    // Compute result for 2-level parts
    uint res2 = (lodLevel < 2u) ? 0u : 1u;
    // Compute result for 3-level parts
    uint res3 = (lodLevel == 0u) ? 0u : ((lodLevel == 3u) ? 2u : 1u);

    // Compose final result using ternaries and mix
    uint result = case1 ? lodLevel :
                  case2 ? 0u :
                  case3 ? ((numPartLevels == 2u) ? res2 :
                           (numPartLevels == 3u) ? res3 : lodLevel)
                        : lodLevel;

    return result;
}

void resetDrawParams(inout DrawCommand drawParams[4]) {
#for LOD_IDX to 4
    BASE_INSTANCE(drawParams[${LOD_IDX}]) = 0;
    INSTANCE_COUNT(drawParams[${LOD_IDX}]) = 0;
#endfor
}

void computeBaseInstance(inout DrawCommand drawParams[4]) {
    // Compute baseInstance for each LOD level
    BASE_INSTANCE(drawParams[1]) = INSTANCE_COUNT(drawParams[0]);
    BASE_INSTANCE(drawParams[2]) = BASE_INSTANCE(drawParams[1]) + INSTANCE_COUNT(drawParams[1]);
    BASE_INSTANCE(drawParams[3]) = BASE_INSTANCE(drawParams[2]) + INSTANCE_COUNT(drawParams[2]);
}

void main() {
    if (globalID != 0) return;

    uint partLevel = 0u;
#for PART_IDX to ${NUM_ATTACHED_PARTS}
    resetDrawParams(in_drawParams${PART_IDX});
    // compute number of visible instances + baseInstance for each LOD level
    #for LOD_IDX to ${NUM_BASE_LODS}
    partLevel = getPartLevel(
        ${LOD_IDX},
        ${NUM_PART_LOD_${PART_IDX}},
        ${NUM_BASE_LODS});
    INSTANCE_COUNT(in_drawParams${PART_IDX}[partLevel]) +=
        INSTANCE_COUNT(in_drawParamsBase[${LOD_IDX}]);
    #endfor // LOD_IDX
    computeBaseInstance(in_drawParams${PART_IDX});
#endfor // PART_I
}
