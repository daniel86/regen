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
#include regen.shapes.lod.DrawCommand
#include regen.stages.compute.defines
#include regen.shapes.lod.defines
#include regen.states.camera.defines

#ifdef USE_COMPACTION
// - [write] the number of instances per layer, after culling.
// - size = NUM_LAYERS
buffer uint in_numVisibleKeys[];
#endif
// - [write] distance sort keys, computed by culling pass, per instance
// - size = NUM_LAYERS * LOD_NUM_INSTANCES
buffer uint in_keys[];
// - [write] One per LOD group: how many valid instances passed culling, and the base instance offset
// - size = NUM_LAYERS * MAX_NUM_LOD_GROUPS
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

#include regen.stages.compute.readPosition

int getLODGroup(float squaredDistance) {
    // Returns the LOD group for a given depth.
    return int(squaredDistance >= in_lodThresholds.x)
         + int(squaredDistance >= in_lodThresholds.y)
         + int(squaredDistance >= in_lodThresholds.z);
}

#ifdef USE_CULLING
#include regen.shapes.culling.isShapeVisible
#endif

float countLOD(uint layer, vec3 pos) {
    vec3 diff = pos - REGEN_CAM_POS_(layer);
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

uint getDrawBin(uint lod, uint layer) {
    return lod * NUM_LAYERS + layer;
}
uint getGlobalOffset(uint instance, uint layer) {
    return layer * LOD_NUM_INSTANCES + instance;
}

void main() {
    uint globalID = gl_GlobalInvocationID.x;
    uint localID  = gl_LocalInvocationID.x;
    uint groupID  = gl_WorkGroupID.x;
    uint layer = regen_computeLayer; // 0..NUM_LAYERS-1
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
        l_visible = isShapeVisible(layer, globalID, pos);
        if (l_visible) {
            depth = countLOD(layer, pos);
        } else {
            // NOTE: For culled instances we use FLT_MAX as depth value for the sort key,
            //       effectively putting them at the end of the list.
    #ifdef USE_REVERSE_SORT
            depth = 0.0f;
    #else
            depth = FLT_MAX;
    #endif
        }
#else
        depth = countLOD(pos);
#endif
    }
    barrier();

    // Write results to global memory
    if (localID < MAX_NUM_LOD_GROUPS) {
        uint baseInstanceIdx;
        uint drawBin = getDrawBin(localID, layer);
        atomicAdd(INSTANCE_COUNT(in_drawParams[drawBin]), sh_lodGroupSize[localID]);

        // also compute the offset for each higher LOD.
        // Note: this could be done via prefix scan in a separate pass, but it might not be worth it
        //       because we only have a few LOD groups (4 usually).
        for (uint higherLOD = localID + 1; higherLOD < MAX_NUM_LOD_GROUPS; ++higherLOD) {
            drawBin = getDrawBin(higherLOD, layer);
            baseInstanceIdx = 5u - in_drawParams[drawBin].drawMode;
            atomicAdd(in_drawParams[drawBin].data[baseInstanceIdx], sh_lodGroupSize[localID]);
        }
    }
    if (globalID < LOD_NUM_INSTANCES) {
        uint globalWriteIdx = getGlobalOffset(globalID, layer);
#ifdef USE_COMPACTION
        if (l_visible) {
            uint idx = atomicAdd(in_numVisibleKeys[layer], 1);
            uint compactedWriteIdx = getGlobalOffset(idx, layer);
            in_keys[globalWriteIdx] = floatBitsToUint(depth);
            in_instanceIDMap[compactedWriteIdx] = globalID;
        }
#else
        in_keys[globalWriteIdx] = floatBitsToUint(depth);
        in_instanceIDMap[globalWriteIdx] = globalID;
#endif
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

#for PART_IDX to ${NUM_ATTACHED_PARTS}
    resetDrawParams(in_drawParams${PART_IDX});
    // compute number of visible instances + baseInstance for each LOD level
    #for LOD_IDX to ${NUM_BASE_LODS}
    INSTANCE_COUNT(in_drawParams${PART_IDX}[${LOD_IDX}]) +=
        INSTANCE_COUNT(in_drawParamsBase[${LOD_IDX}]);
    #endfor // LOD_IDX
    computeBaseInstance(in_drawParams${PART_IDX});
#endfor // PART_I
}
