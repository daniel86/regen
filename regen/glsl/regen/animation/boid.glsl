
-- getGridIndex
#ifndef getGridIndex_included
#define2 getGridIndex_included
ivec3 getGridCoord(vec3 boidPos) {
    vec3 gridPos = max(vec3(0.0),
        (boidPos - in_gridMin) / in_cellSize);
    return min(
        ivec3(trunc(gridPos)),
        in_gridSize - ivec3(1));
}
int getGridIndex(vec3 boidPos) {
    ivec3 cellIndex = getGridCoord(boidPos);
    cellIndex = clamp(cellIndex, ivec3(0), in_gridSize - ivec3(1));
    return cellIndex.x +
           cellIndex.y * in_gridSize.x +
           cellIndex.z * in_gridSize.x * in_gridSize.y;
}
#endif

--------------
-----
--------------
-- grid.cs
#include regen.stages.compute.defines
// SSBOs
buffer int in_listNext[];
buffer int in_listHeads[];
// Grid UBO
uniform vec3 in_gridMin;
uniform float in_cellSize;
uniform ivec3 in_gridSize;

#include regen.animation.boid.getGridIndex
#include regen.stages.compute.readPosition

void main() {
    uint id = gl_GlobalInvocationID.x;
    uint numElements = in_listNext.length();

    // Initialize in_listHeads to -1 for all cells
    if (id < numElements) {
        //int numCells = in_gridSize.x * in_gridSize.y * in_gridSize.z;
        int numCells = in_listHeads.length();
        for (int i = 0; i < numCells; ++i) {
            in_listHeads[i] = -1;
        }
    }
    barrier();

    // Compute grid cell index
    if (id < numElements) {
        vec3 boidPos = readPosition(id);
        int gridIndex = getGridIndex(boidPos);
        // Insert into the linked list for the cell
        int oldHead = atomicExchange(in_listHeads[gridIndex], int(id));
        in_listNext[id] = oldHead;
    }
}

--------------
-----
--------------
-- simulate.cs
#include regen.stages.compute.defines
// Grid UBO
uniform vec3 in_gridMin;
uniform float in_cellSize;
uniform ivec3 in_gridSize;
// SSBOs
uniform float in_vel[];
uniform int in_listNext[];
uniform int in_listHeads[];

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
#include regen.shapes.bbox.computeBBox

vec2 computeUV(vec3 boidPosition, vec3 mapCenter, vec2 mapSize) {
    return (boidPosition.xz - mapCenter.xz) / mapSize + vec2(0.5);
}

bool avoidCollisions(vec3 boidPos, vec3 boidVel, float dt, inout vec3 force) {
    float repulsionFactor = in_repulsionFactor * in_separationWeight;
    vec3 nextVelocity = boidVel + force * dt;
    vec3 lookAhead = boidPos + nextVelocity * in_lookAheadDistance;
    bool isCollisionFree = true;

	////////////////
	/////// Collision with boundaries.
	////////////////
	if (lookAhead.x < in_simulationBoundsMin.x + in_avoidanceDistance) {
		force.x += repulsionFactor;
	}
	else if (lookAhead.x > in_simulationBoundsMax.x - in_avoidanceDistance) {
		force.x -= repulsionFactor;
	}
	if (lookAhead.y < in_simulationBoundsMin.y + in_avoidanceDistance) {
		force.y += repulsionFactor;
	}
	else if (lookAhead.y > in_simulationBoundsMax.y - in_avoidanceDistance) {
		force.y -= repulsionFactor;
	}
	if (lookAhead.z < in_simulationBoundsMin.z + in_avoidanceDistance) {
		force.z += repulsionFactor;
	}
	else if (lookAhead.z > in_simulationBoundsMax.z - in_avoidanceDistance) {
		force.z -= repulsionFactor;
	}

#ifdef HAS_heightMap
	////////////////
	/////// Collision with height map.
	////////////////
    float avoidDistanceHalf = in_avoidanceDistance * 0.5;
    vec2 boidCoord = computeUV(boidPos, in_mapCenter, in_mapSize);
    float currentY = texture(in_heightMap, boidCoord).x;
    currentY *= in_heightMapFactor;
    currentY += in_mapCenter.y + avoidDistanceHalf;
    // sample the height map at the projected boid position
    boidCoord = computeUV(lookAhead, in_mapCenter, in_mapSize);
    float nextY = texture(in_heightMap, boidCoord).x;
    nextY *= in_heightMapFactor;
    nextY += in_mapCenter.y + avoidDistanceHalf;
    if (boidPos.y < currentY) {
        // The boid is below the minimum height of the height map -> push boid up.
        if (currentY < in_simulationBoundsMax.y - avoidDistanceHalf) {
            // only push up in case the height map y is below the maximum y of boids
            force.y += repulsionFactor;
        }
        isCollisionFree = false;
    }
    if (lookAhead.y < nextY) {
        // Assuming the boid follows the lookAhead direction, it will eventually reach a point where
        // it is below the surface of the height map -> push boid up.
        if (nextY < in_simulationBoundsMax.y - avoidDistanceHalf) {
            // only push up in case the height map y is below the maximum y of boids
            force.y += repulsionFactor;
        }
    }
    if (nextY > in_simulationBoundsMax.y - avoidDistanceHalf) {
        // Assuming the boid follows the lookAhead direction, it will eventually reach a point where
        // it is above the max y of the boid bounds.
        // We could sample normal map here, and compute reflection vector. But this might be a bit
        // overkill for huge number of boids.
        // An easy approach is to push the boid to where it came from:
        force.x -= nextVelocity.x * repulsionFactor;
        force.z -= nextVelocity.z * repulsionFactor;
    }
#endif

	return isCollisionFree;
}

void homesickness(inout uint seed, inout vec3 force, vec3 boidPos) {
	// a boid seems to have lost track, and wants to go home!
	// first find closest home point...
    vec3 closest = vec3(0.0);
#if NUM_BOID_HOMES > 0
    float closestDistance = FLT_MAX, nextDistance;
    #for HI to NUM_BOID_HOMES
    nextDistance = distance(BOID_HOME${HI}, boidPos);
    if (nextDistance < closestDistance) {
        closestDistance = nextDistance;
        closest = BOID_HOME${HI};
    }
    #endfor
#else
    float closestDistance = distance(boidPos, home);
#endif
	// second steer towards the closest home point ...
	vec3 dir = closest - boidPos;
	if (closestDistance < 0.001) {
        dir = random3(seed);
	} else {
	    dir /= closestDistance;
	}
    force += dir * in_repulsionFactor * in_separationWeight * 0.1;
}

void limitVelocity(inout vec3 vel, vec3 lastVel) {
    // limit translation speed
    float maxSpeed = in_maxBoidSpeed;
    float currentSpeed = length(vel);
    if (currentSpeed > maxSpeed) {
        vel = normalize(vel) * maxSpeed;
    }
    if (currentSpeed > 0.001) {
        // limit angular speed
        vec3 nextDirection = normalize(vel);
        vec3 lastDirection = normalize(lastVel);
        float angle = acos(dot(nextDirection,lastDirection));
        if (angle > in_maxAngularSpeed) {
            vec3 axis = normalize(cross(nextDirection, lastDirection));
            vec4 q = quaternion(axis, in_maxAngularSpeed);
            vel = q_rotate(q, lastDirection) * length(vel);
        }
    }
}

vec3 getCellCenter(ivec3 boidCell) {
	vec3 v = vec3(
		float(boidCell.x),
		float(boidCell.y),
		float(boidCell.z));
	return in_gridMin + v * in_cellSize + vec3(in_visualRange);
}

ivec3[8] getVisibleCells(vec3 boidPos, ivec3 boidCell) {
	vec3 cellPosition = getCellCenter(boidCell);
	ivec3 dir = ivec3(
	    (boidPos.x < cellPosition.x ? -1 : 1),
	    (boidPos.y < cellPosition.y ? -1 : 1),
	    (boidPos.z < cellPosition.z ? -1 : 1));
	return ivec3[8](
	    boidCell,
        boidCell + ivec3(dir.x, 0, 0),
        boidCell + ivec3(0, dir.y, 0),
        boidCell + ivec3(0, 0, dir.z),
        boidCell + ivec3(dir.x, dir.y, 0),
        boidCell + ivec3(dir.x, 0, dir.z),
        boidCell + ivec3(0, dir.y, dir.z),
        boidCell + ivec3(dir.x, dir.y, dir.z)
    );
}

vec3 simulateBoid(uint boidID, vec3 boidPos, vec3 boidVel, float dt) {
    //vec3 dir = normalize(boidVel);
    vec3 force = vec3(0.0);
	vec3 avgPosition = vec3(0.0);
	vec3 avgVelocity = vec3(0.0);
    vec3 separation = vec3(0.0);
    int numBoids = in_listNext.length();
    int numNeighbors = 0;
    // compute a seed for the boid
    uint seed = boidID;
    seed ^= uint(in_timeDeltaMS); // time-based variation
    seed ^= floatBitsToUint(dot(boidPos, vec3(12.9898, 78.233, 45.164))); // position entropy

#ifdef DISABLE_BOID_GRID
    for (int i = 0; i < numBoids && numNeighbors < in_maxNumNeighbors; ++i) {
        if (i == boidID) { continue; }
        vec3 neighborPos = readPosition(uint(i));
        vec3 neighborVel = readVelocity(uint(i));
        vec3 dir = boidPos - neighborPos;
        float distance = length(dir);
        if (distance < in_visualRange) {
            numNeighbors += 1;
            avgPosition += neighborPos;
            avgVelocity += neighborVel;
            if (distance < in_avoidanceDistance) {
                if (distance < 0.001) {
                    dir = random3(seed);
                    //dir.y = -abs(dir.y);
                    dir = normalize(dir);
                } else {
                    dir /= distance * distance;
                }
                separation += dir;
            }
        }
    }
#else
    ivec3 boidIndx3D = getGridCoord(boidPos);
    ivec3[8] visible = getVisibleCells(boidPos, boidIndx3D);
    int gridSizeXY = in_gridSize.x * in_gridSize.y;
    float visualRangeSq = in_visualRange * in_visualRange;
    float avoidanceDistanceSq = in_avoidanceDistance * in_avoidanceDistance;

    for (int vIndex=0; vIndex<8; ++vIndex) {
        ivec3 cell3D = visible[vIndex];
        if (any(lessThan(cell3D, ivec3(0))) ||
            any(greaterThanEqual(cell3D, in_gridSize))) {
            continue;
        }
        int cellIndex = cell3D.x + cell3D.y * in_gridSize.x + cell3D.z * gridSizeXY;
        int neighborIndex = in_listHeads[cellIndex];

        while (neighborIndex >= 0 && numNeighbors < in_maxNumNeighbors) {
            // read the next cell element
            uint nextIndex = uint(neighborIndex);
            neighborIndex = in_listNext[neighborIndex];
            if (nextIndex == boidID) { continue; }
            vec3 neighborPos = readPosition(nextIndex);
            vec3 dir = boidPos - neighborPos;
            float distSq = dot(dir, dir);
            if (distSq > visualRangeSq) { continue; }

            numNeighbors += 1;
            avgPosition += neighborPos;
            avgVelocity += readVelocity(nextIndex);

            if (distSq < avoidanceDistanceSq) {
                if (distSq < 0.001) {
                    dir = normalize(random3(seed));
                } else {
                    dir /= distSq;
                }
                separation += dir;
            }
        }
    }
#endif

    int isBoidLost = int(numNeighbors == 0);
    if (isBoidLost == 0) {
        avgPosition /= float(numNeighbors);
        avgVelocity /= float(numNeighbors);
        force += separation * in_separationWeight +
            (avgVelocity - boidVel) * in_alignmentWeight +
            (avgPosition - boidPos) * in_coherenceWeight;
    }

	// put some restrictions on the boid's velocity.
	// a boid that cannot avoid collisions is considered lost.
	isBoidLost += int(avoidCollisions(boidPos, boidVel, dt, force) == false);
	//bool isInDanger = !dangers_.empty() && !avoidDanger(boidID, boidPos);
	//if (!isInDanger) {
	//	// note: ignore attractors if in danger
	//	attract(boidID, boidPos);
	//}
	// drift towards home if lost
	if (isBoidLost > 0) {
	    homesickness(seed, force, boidPos);
    }

    return force;
}

void main() {
    uint gid = gl_GlobalInvocationID.x;
    uint numElements = in_listNext.length();

    if (gid < numElements) {
        vec3 pos = readPosition(gid);
        vec3 vel = readVelocity(gid);
        vec3 lastVel = vel;
        float dt_s = in_timeDeltaMS * 0.001;

        // Simulate boid
        vec3 force = simulateBoid(gid, pos, vel, dt_s);
        // Update position and velocity
        pos += vel * dt_s;
        vel += force * dt_s;
        limitVelocity(vel, lastVel);
        barrier();

        // Write back to SSBOs
#ifdef HAS_modelMatrix
        writeModelMatrix(gid, pos, vel);
#else
        writePosition(gid, pos);
#endif
        writeVelocity(gid, vel);
        computeBBox(gid, pos, numElements);
    }
}
