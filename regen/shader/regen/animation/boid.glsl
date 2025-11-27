
-- getGridCoord
#ifndef getGridCoord_included
#define2 getGridCoord_included
uvec3 getGridCoord(vec3 boidPos) {
    vec3 gridPos = max(vec3(0.0),
        (boidPos - in_gridMin) / in_cellSize);
    return min(
        uvec3(trunc(gridPos)),
        in_gridSize - uvec3(1));
}
#endif
-- getGridIndex
#ifndef getGridIndex_included
#define2 getGridIndex_included
#include regen.animation.boid.getGridCoord
uint getGridIndex(vec3 boidPos) {
    uvec3 cellIndex = getGridCoord(boidPos);
    cellIndex = clamp(cellIndex, uvec3(0), in_gridSize - uvec3(1));
    return cellIndex.x +
           cellIndex.y * in_gridSize.x +
           cellIndex.z * in_gridSize.x * in_gridSize.y;
}
-- getGridIndex2
#ifndef getGridIndex2_included
#define2 getGridIndex2_included
#include regen.stages.compute.readPosition
#include regen.animation.boid.getGridIndex
uint getGridIndex(uint boidID) {
    vec3 boidPos = readPosition(boidID);
    return getGridIndex(boidPos);
}
#endif

--------------
----- Struct defintion for interleaved boid data.
--------------
-- BoidData
struct BoidData {
    vec3 pos;
    float pad1;
    vec3 vel;
    float pad2;
};

--------------
----- Initialized the key buffer for the radix sort, and also resets the
----- grid offsets buffer.
--------------
-- grid.reset.cs
#include regen.stages.compute.defines

buffer uint in_keys[];
buffer uint in_values[];
buffer uint in_globalHistogram[];

#include regen.animation.boid.getGridIndex2

void main() {
    uint gid = gl_GlobalInvocationID.x;

    if (gid < NUM_BOIDS) {
        // Initialize the key buffer, we want to sort the boids by their grid index
        in_keys[gid] = getGridIndex(gid);
        // Initialize the value buffer to [0,...,NUM_BOIDS-1]
        in_values[gid] = gid;
    }

    // Initialize the grid offsets buffer.
    // We initialize it to the max offset here, which is the number of boids.
    if (gid < in_numGridCells) {
        in_globalHistogram[gid] = NUM_BOIDS;
    }
}

--------------
----- Compute offsets for the grid cells given sorted indices.
--------------
-- grid.offsets.cs
#ifdef USE_SORTED_DATA
#input regen.animation.boid.BoidData
#endif
#include regen.stages.compute.defines

buffer uint in_values[];
buffer uint in_globalHistogram[];
#ifdef USE_SORTED_DATA
buffer BoidData in_boidData[];
#endif

#ifdef USE_SORTED_DATA
#include regen.stages.compute.readVelocity
#include regen.stages.compute.readPosition
#endif
#include regen.animation.boid.getGridIndex2

void writeHistogram(uint gid) {
    uint currentGrid = getGridIndex(in_values[gid]);
    uint begin = (gid == 0) ? 0 : (getGridIndex(in_values[gid - 1])+1);
    for (uint cell = begin; cell <= currentGrid; ++cell) {
        in_globalHistogram[cell] = gid;
    }
}

#ifdef USE_SORTED_DATA
void writeBoidData(out BoidData boidData, uint sortedID) {
    boidData.pos = readPosition(sortedID);
    boidData.vel = readVelocity(sortedID);
}
#endif

void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= NUM_BOIDS) return;
    writeHistogram(gid);
#ifdef USE_SORTED_DATA
    writeBoidData(in_boidData[gid], in_values[gid]);
#endif
}

--------------
-----
--------------
-- simulate.cs
#input regen.animation.boid.BoidData
#include regen.stages.compute.defines
// Grid UBO
uniform vec3 in_gridMin;
uniform float in_cellSize;
uniform ivec3 in_gridSize;
// SSBOs
#ifdef HAS_boidData
buffer BoidData in_boidData[];
#else
buffer float in_vel[];
#endif
buffer uint in_globalHistogram[];
uniform float in_boidTimeDelta;

//#define DISABLE_BOID_GRID

#include regen.animation.boid.getGridIndex
#include regen.stages.compute.velocity
#ifdef HAS_modelMatrix
#include regen.stages.compute.modelMatrix
#else
#include regen.stages.compute.position
#endif
#include regen.noise.random
#include regen.math.quaternion.0
#include regen.math.quaternion.rotate

struct BoidParams {
    uint gridSizeXY;
    float visualRangeSq;
    float avoidanceDistanceSq;
};

struct BoidAccum {
    vec3 avgPosition;
    vec3 avgVelocity;
    vec3 separation;
    uint numNeighbors;
};

vec2 computeUV(vec3 boidPosition, vec3 mapCenter, vec2 mapSize) {
    return (boidPosition.xz - mapCenter.xz) / mapSize + vec2(0.5);
}

float avoidCollisions(BoidData boid, float dt, inout vec3 force) {
    float repulsionFactor = in_repulsionFactor * in_separationWeight;
    vec3 nextVelocity = boid.vel + force * dt;
    vec3 lookAhead = boid.pos + nextVelocity * in_lookAheadDistance;
    float isCollisionFree = 1.0;

	////////////////
	/////// Collision with boundaries.
	////////////////
	vec3 lowerBound = in_simulationBoundsMin + vec3(in_avoidanceDistance);
	vec3 upperBound = in_simulationBoundsMax - vec3(in_avoidanceDistance);
	// Calculate repulsion direction: +1 if too close to min bound, -1 if too close to max bound, 0 otherwise
	vec3 repelDir = step(lookAhead, lowerBound) - step(upperBound, lookAhead);
	// Apply repulsion force
	force += repelDir * repulsionFactor;

#ifdef HAS_heightMap
	////////////////
	/////// Collision with height map.
	////////////////
    float avoidDistanceHalf = in_avoidanceDistance * 0.5;
    vec2 boidCoord = computeUV(boid.pos, in_mapCenter, in_mapSize);
    float currentY = texture(in_heightMap, boidCoord).x;
    currentY *= in_heightMapFactor;
    currentY += in_mapCenter.y + avoidDistanceHalf;
    // sample the height map at the projected boid position
    boidCoord = computeUV(lookAhead, in_mapCenter, in_mapSize);
    float nextY = texture(in_heightMap, boidCoord).x;
    nextY *= in_heightMapFactor;
    nextY += in_mapCenter.y + avoidDistanceHalf;
    {
        // The boid is below the minimum height of the height map -> push boid up.
        float isBelowMap = step(boid.pos.y, currentY);
        // only push up in case the height map y is below the maximum y of boids
        float isMapInRange = step(currentY, in_simulationBoundsMax.y - avoidDistanceHalf);
        force.y += repulsionFactor * isMapInRange * isBelowMap;
        isCollisionFree = (1.0 - isBelowMap);
    }
    {
        // Assuming the boid follows the lookAhead direction, it will eventually reach a point where
        // it is below the surface of the height map -> push boid up.
        // but only push up in case the height map y is below the maximum y of boids
        force.y += repulsionFactor *
                step(lookAhead.y, nextY) *
                step(nextY, in_simulationBoundsMax.y - avoidDistanceHalf);
    }
    {
        // Assuming the boid follows the lookAhead direction, it will eventually reach a point where
        // it is above the max y of the boid bounds.
        // We could sample normal map here, and compute reflection vector. But this might be a bit
        // overkill for huge number of boids.
        // An easy approach is to push the boid to where it came from:
        float isCollisionAhead = 1.0 - step(nextY, in_simulationBoundsMax.y - avoidDistanceHalf);
        isCollisionAhead *= repulsionFactor;
        force.x -= nextVelocity.x * isCollisionAhead;
        force.z -= nextVelocity.z * isCollisionAhead;
    }
#endif
	return isCollisionFree;
}

void homesickness(inout uint seed, inout vec3 force, vec3 boidPos, float isBoidLost) {
	// a boid seems to have lost track, and wants to go home!
	// first find closest home point...
    vec3 closest = vec3(0.0);
#if NUM_BOID_HOMES > 0
    float closestDistance = FLT_MAX;
    #for HI to NUM_BOID_HOMES
    {
        float nextDistance = distance(BOID_HOME${HI}, boidPos);
        float isCloser = step(nextDistance, closestDistance); // 1 if nextDistance < closestDistance
        closest = mix(closest, BOID_HOME${HI}, isCloser);
        closestDistance = mix(closestDistance, nextDistance, isCloser);
    }
    #endfor
#else
    float closestDistance = distance(boidPos, home);
#endif
	// second steer towards the closest home point ...
	vec3 dir = mix(
	    random3(seed),
	    (closest - boidPos) / max(closestDistance, 0.001),
	    step(0.001, closestDistance));
    force += dir * in_repulsionFactor * in_separationWeight * 0.1 * isBoidLost;
}

void limitVelocity(inout vec3 vel, vec3 lastVel) {
    float currentSpeed = length(vel);
    float lastSpeed = length(lastVel);
    float isMoving = step(0.001, currentSpeed);
    // if too fast, slow down to max speed
    vel *= mix(1.0,
        in_maxBoidSpeed / max(currentSpeed, 0.0001),
        step(in_maxBoidSpeed, currentSpeed));
    // also limit angular speed
    {
        float nextSpeed = length(vel);
        vec3 nextDirection = vel / max(nextSpeed, 0.001);
        vec3 lastDirection = lastVel / max(lastSpeed, 0.001);
        // find angle between last and next direction
        float angle = acos(dot(nextDirection,lastDirection));
        // compute axis of rotation
        vec3 axis = cross(nextDirection, lastDirection);
        float axisLength = length(axis);
        axis /= max(axisLength, 0.001);
        // compute quaternion for rotation
        vec4 q = quaternion(axis, in_maxAngularSpeed);
        vel = mix(vel,
            q_rotate(q, lastDirection) * nextSpeed,
            step(in_maxAngularSpeed, angle) * step(0.001, axisLength));
    }
}

vec3 getCellCenter(uvec3 boidCell) {
	vec3 v = vec3(
		float(boidCell.x),
		float(boidCell.y),
		float(boidCell.z));
	return in_gridMin + v * in_cellSize + vec3(in_visualRange);
}

uvec3[8] getVisibleCells(vec3 boidPos, uvec3 boidCell) {
	vec3 cellPosition = getCellCenter(boidCell);
	uvec3 dir = uvec3(
	    (boidPos.x < cellPosition.x ? -1 : 1),
	    (boidPos.y < cellPosition.y ? -1 : 1),
	    (boidPos.z < cellPosition.z ? -1 : 1));
	return uvec3[8](
	    boidCell,
        boidCell + uvec3(dir.x, 0, 0),
        boidCell + uvec3(0, dir.y, 0),
        boidCell + uvec3(0, 0, dir.z),
        boidCell + uvec3(dir.x, dir.y, 0),
        boidCell + uvec3(dir.x, 0, dir.z),
        boidCell + uvec3(0, dir.y, dir.z),
        boidCell + uvec3(dir.x, dir.y, dir.z)
    );
}

void accumulateNeighbor(
            in BoidData boid,
            in BoidData neighbor,
            in BoidParams params,
            inout BoidAccum acc,
            inout uint seed) {
    vec3 dir = boid.pos - neighbor.pos;
    float distSq = dot(dir, dir);
    float isVisible = 1.0 - step(params.visualRangeSq, distSq);
    dir = mix(
        normalize(random3(seed)),
        dir / max(distSq, 0.001),
        step(0.001, distSq));
    acc.numNeighbors += 1 * uint(isVisible > 0.5);
    acc.avgPosition += neighbor.pos * isVisible;
    acc.avgVelocity += neighbor.vel * isVisible;
    acc.separation  += dir * step(distSq, params.avoidanceDistanceSq) * isVisible;
}

void accumulateCell(
            in BoidData boid,
            in BoidParams params,
            in uint boidID,
            in uvec3 cell3D,
            inout BoidAccum acc,
            inout uint seed) {
    if (acc.numNeighbors >= in_maxNumNeighbors ||
        any(lessThan(cell3D, uvec3(0))) ||
        any(greaterThanEqual(cell3D, in_gridSize))) {
        return;
    }
    uint cellIndex = cell3D.x + cell3D.y * in_gridSize.x + cell3D.z * params.gridSizeXY;
    // get neighbor indices from the grid offsets, i.e. we use begin/end offset of the cell
    // that points into the sorted index buffer. so for each cell we read the range [begin, end)
    // and check if the boid is in the range.
    uint begin = in_globalHistogram[cellIndex];
    // NOTE: we add an additional element to the end of the grid offsets buffer,
    //       so +1 is safe here without going out of bounds.
    uint end = in_globalHistogram[cellIndex + 1];
#ifdef HAS_boidData
    for (uint i = begin; i < end && acc.numNeighbors < in_maxNumNeighbors; ++i) {
        accumulateNeighbor(boid, in_boidData[i], params, acc, seed);
    }
#else
    BoidData neighbor;
    for (uint i = begin; i < end && acc.numNeighbors < in_maxNumNeighbors; ++i) {
        uint nextIndex = in_values[i];
        neighbor.pos = readPosition(nextIndex);
        neighbor.vel = readVelocity(nextIndex);
        accumulateNeighbor(boid, neighbor, params, acc, seed);
    }
#endif
}

vec3 simulateBoid(BoidData boid, uint boidID, float dt) {
    //vec3 dir = normalize(boidVel);
    vec3 force = vec3(0.0);
    BoidAccum acc;
    acc.avgPosition = vec3(0.0);
    acc.avgVelocity = vec3(0.0);
    acc.separation = vec3(0.0);
    acc.numNeighbors = 0;
    // compute a seed for the boid
    uint seed = boidID;
    seed ^= uint(in_boidTimeDelta); // time-based variation
    seed ^= floatBitsToUint(dot(boid.pos, vec3(12.9898, 78.233, 45.164))); // position entropy
    // create params struct
    BoidParams params;
    params.gridSizeXY          = in_gridSize.x * in_gridSize.y;
    params.visualRangeSq       = in_visualRange * in_visualRange;
    params.avoidanceDistanceSq = in_avoidanceDistance * in_avoidanceDistance;

#ifdef DISABLE_BOID_GRID
    for (uint i = 0; i < numBoids && acc.numNeighbors < in_maxNumNeighbors; ++i) {
        vec3 neighborPos = readPosition(i);
        vec3 neighborVel = readVelocity(i);
        vec3 dir = boid.pos - neighborPos;
        float distSq = dot(dir, dir);
        if (distSq < params.visualRangeSq) {
            acc.numNeighbors += 1;
            acc.avgPosition += neighborPos;
            acc.avgVelocity += neighborVel;
            if (distSq < params.avoidanceDistanceSq) {
                float safeDist = max(distSq, 0.001);
                dir = mix(
                    normalize(random3(seed)),
                    dir / safeDist,
                    step(0.001, distSq));
                acc.separation += dir;
            }
        }
    }
#else
    {
        // compute boid index
        uvec3 boidIndx3D = getGridCoord(boid.pos);
        uvec3[8] visible = getVisibleCells(boid.pos, boidIndx3D);
        // compute influence of n neighboring boids
#for CELL_I to 8
        accumulateCell(boid, params, boidID, visible[${CELL_I}], acc, seed);
#endfor
    }
#endif

    float isBoidInSwarm = step(2.0, float(acc.numNeighbors));
    {
        float safeDivisor = max(float(acc.numNeighbors), 1.0);
        acc.avgPosition /= safeDivisor;
        acc.avgVelocity /= safeDivisor;
        force += isBoidInSwarm * (acc.separation * in_separationWeight +
            (acc.avgVelocity - boid.vel) * in_alignmentWeight +
            (acc.avgPosition - boid.pos) * in_coherenceWeight);
    }

	// put some restrictions on the boid's velocity.
	// a boid that cannot avoid collisions is considered lost.
	float isCollisionFree = avoidCollisions(boid, dt, force);

	// drift towards home if lost or in collision
	homesickness(seed, force, boid.pos,
	    step(1, (1.0 - isBoidInSwarm) + (1.0 - isCollisionFree)));

    return force;
}

void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid > NUM_BOIDS) return;

#ifdef HAS_boidData
    BoidData boid = in_boidData[gid];
#else
    BoidData boid;
    #ifndef DISABLE_BOID_GRID
    #ifndef USE_GRID_LIST
    // It should be best to process the boids cell-by-cell.
    // This can be done by mapping gid using the sorted indices stored in in_values.
    gid = in_values[gid];
    #endif
    #endif
    boid.pos = readPosition(gid);
    boid.vel = readVelocity(gid);
#endif
    vec3 lastVel = boid.vel;
    // Simulate boid
    vec3 force = simulateBoid(boid, gid, in_boidTimeDelta);
    // Update position and velocity
    boid.pos += boid.vel * in_boidTimeDelta;
    boid.vel += force * in_boidTimeDelta;
    limitVelocity(boid.vel, lastVel);

    // Write back to SSBOs
#ifdef HAS_boidData
    uint writeIdx = in_values[gid];
#else
    uint writeIdx = gid;
#endif
#ifdef HAS_modelMatrix
    writeModelMatrix(writeIdx, boid.pos, boid.vel);
#else
    writePosition(writeIdx, boid.pos);
#endif
    writeVelocity(writeIdx, boid.vel);
}
