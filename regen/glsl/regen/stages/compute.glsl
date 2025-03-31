
-- defines
#ifndef cs_defines_included
#define2 cs_defines_included
#version 450
#define FLT_MAX 3.40282e+38
#define FLT_MIN -3.40282e+38
layout(
    local_size_x = ${CS_LOCAL_SIZE_X},
    local_size_y = ${CS_LOCAL_SIZE_Y},
    local_size_z = ${CS_LOCAL_SIZE_Z}) in;
#endif // cs_defines_included

-- position
#include regen.stages.compute.writePosition
#include regen.stages.compute.readPosition

-- readPosition
#ifndef readPosition_included
#define2 readPosition_included
vec3 readPosition(uint i) {
    #ifdef HAS_modelMatrix
    return in_modelMatrix[i][3].xyz;
    #else
    i *= 3;
    return vec3(in_pos[i], in_pos[i + 1], in_pos[i + 2]);
    #endif
}
#endif // readPosition_included

-- writePosition
#ifndef writePosition_included
#define2 writePosition_included
void writePosition(uint i, vec3 pos) {
    i *= 3;
    in_pos[i]     = pos.x;
    in_pos[i + 1] = pos.y;
    in_pos[i + 2] = pos.z;
}
#endif // writePosition_included

-- velocity
#include regen.stages.compute.writeVelocity
#include regen.stages.compute.readVelocity

-- readVelocity
#ifndef readVelocity_included
#define2 readVelocity_included
#ifdef USE_HALF_VELOCITY
vec3 readVelocity(uint id) {
    // velocity is stored as uvec2[COUNT] as half-precision floats
    uvec2 vel_u = in_vel[id];
    return vec3(
        unpackHalf2x16(vel_u.x),
        unpackHalf2x16(vel_u.y).x);
}
#else
vec3 readVelocity(uint id) {
    // velocity is stored as float[COUNT*3]
    id *= 3;
    return vec3(in_vel[id], in_vel[id+1], in_vel[id+2]);
}
#endif
#endif

-- writeVelocity
#ifndef writeVelocity_included
#define2 writeVelocity_included
#ifdef USE_HALF_VELOCITY
void writeVelocity(uint id, vec3 vel) {
    in_vel[id] = uvec2(
        packHalf2x16(vel.xy),
        packHalf2x16(vec2(vel.z, 0.0)));
}
#else
void writeVelocity(uint id, vec3 vel) {
    // velocity is stored as float[NUM_BOIDS*3]
    id *= 3;
    in_vel[id]     = vel.x;
    in_vel[id + 1] = vel.y;
    in_vel[id + 2] = vel.z;
}
#endif
#endif

-- modelMatrix
#include regen.stages.compute.readPosition
#include regen.stages.compute.writeModelMatrix

-- writeModelMatrix
#ifndef writeModelMatrix_included
#define2 writeModelMatrix_included
#include regen.math.quaternion.matrix
#include regen.math.quaternion.1
#ifdef HAS_scaleFactor
#include regen.math.mat4.scale
#endif
void writeModelMatrix(uint id, vec3 pos, vec3 vel) {
    float velAmount = length(vel);
    mat4 modelMatrix = in_modelMatrix[id];
    if (velAmount > 0.0001) {
        vec3 orient = vel / velAmount;
        // Construct quaternion from orientation
#ifdef HAS_baseOrientation
        float pitch = atan(-orient.x, orient.z) + in_baseOrientation;
#else
        float pitch = atan(-orient.x, orient.z);
#endif
        vec4 quaternion = quaternion(pitch, asin(-orient.y), 0.0);
        // Convert quaternion to matrix
        modelMatrix = q_matrix(quaternion);
#ifdef HAS_scaleFactor
        scale(modelMatrix, vec3(in_scaleFactor));
#endif
    }
    modelMatrix[3].xyz = pos;
    // Write back to SSBO
    in_modelMatrix[id] = modelMatrix;
}
#endif // writeModelMatrix_included

--------------
----- Computes model matrix given position and velocity. Normalized velocity
----- is used as orientation.
----- Buffers:
----- - [read] float[] in_pos: world positions
----- - [read] float[] in_vel: velocities
----- - [write] mat4[] in_modelMatrix: model matrix of the object
--------------
-- modelMatrix.cs
#include regen.stages.compute.defines
// Buffers
buffer float in_pos[];
buffer float in_vel[];
buffer mat4 in_modelMatrix[];
#include regen.stages.compute.position
#include regen.stages.compute.velocity
void main() {
    uint id = gl_GlobalInvocationID.x;
    uint numElements = in_modelMatrix.length();
    if (id < numElements) {
        vec3 pos = readPosition(id);
        vec3 velDir = readVelocity(id);
        writeModelMatrix(id, pos, velDir);
    }
}
