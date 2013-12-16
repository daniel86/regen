
-- input
#ifdef HAS_BONES
#ifndef __boneInput_Included__
#define __boneInput_Included__
#if NUM_BONE_WEIGHTS==1
in float in_boneWeights;
in uint in_boneIndices;
#elif NUM_BONE_WEIGHTS==2
in vec2 in_boneWeights;
in uvec2 in_boneIndices;
#elif NUM_BONE_WEIGHTS==3
in vec3 in_boneWeights;
in uvec3 in_boneIndices;
#else
in vec4 in_boneWeights;
in uvec4 in_boneIndices;
#endif
#ifndef USE_BONE_TBO
uniform mat4 in_boneMatrices[NUM_BONES];
#endif
uniform int in_numBoneWeights;
#ifdef HAS_boneOffset
in int in_boneOffset;
#endif
#endif // __boneInput_Included__
#endif // HAS_BONES

-- fetchBoneMatrixArray
#define fetchBoneMatrix(i) in_boneMatrices[i]

-- fetchBoneMatrixTBO
#if HAS_INSTANCES && HAS_boneOffset
int boneMatrixIndex(uint i) {
    return (NUM_BONES_PER_MESH*in_boneOffset + int(i))*4;
}
#else
#define boneMatrixIndex(i) int(i)*4
#endif
mat4 fetchBoneMatrix(uint i) {
    int matIndex = boneMatrixIndex(i);
    return mat4(
        texelFetchBuffer(in_boneMatrices, matIndex),
        texelFetchBuffer(in_boneMatrices, matIndex+1),
        texelFetchBuffer(in_boneMatrices, matIndex+2),
        texelFetchBuffer(in_boneMatrices, matIndex+3)
    );
}

-- fetchBoneMatrix
#ifdef HAS_BONES
#ifndef __fetchBoneMatrix_Included__
#define __fetchBoneMatrix_Included__
#include regen.states.bones.input
#ifdef USE_BONE_TBO
#include regen.states.bones.fetchBoneMatrixTBO
#else
#include regen.states.bones.fetchBoneMatrixArray
#endif // USE_BONE_TBO
#endif // __fetchBoneMatrix_Included__
#endif // HAS_BONES

-- transformBone
#ifdef HAS_BONES
#ifndef __transformBone_Included__
#define __transformBone_Included__
#include regen.states.bones.fetchBoneMatrix

vec4 transformBone(vec4 x) {
#if NUM_BONE_WEIGHTS==1
    return fetchBoneMatrix(in_boneIndices) * x;
#elif NUM_BONE_WEIGHTS==2
    return in_boneWeights.x * fetchBoneMatrix(in_boneIndices.x) * x +
           in_boneWeights.y * fetchBoneMatrix(in_boneIndices.y) * x;
#elif NUM_BONE_WEIGHTS==3
    return in_boneWeights.x * fetchBoneMatrix(in_boneIndices.x) * x +
           in_boneWeights.y * fetchBoneMatrix(in_boneIndices.y) * x +
           in_boneWeights.z * fetchBoneMatrix(in_boneIndices.z) * x;
#else
    return in_boneWeights.x * fetchBoneMatrix(in_boneIndices.x) * x +
           in_boneWeights.y * fetchBoneMatrix(in_boneIndices.y) * x +
           in_boneWeights.z * fetchBoneMatrix(in_boneIndices.z) * x +
           in_boneWeights.w * fetchBoneMatrix(in_boneIndices.w) * x;
#endif
}
#endif // __transformBone_Included__
#endif // HAS_BONES
