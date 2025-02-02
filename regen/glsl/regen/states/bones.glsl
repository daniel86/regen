
-- input
#ifdef HAS_BONES
#ifndef REGEN_boneInput_Included_
#define REGEN_boneInput_Included_

in float in_boneWeights[NUM_BONE_WEIGHTS];
in uint in_boneIndices[NUM_BONE_WEIGHTS];

#ifndef USE_BONE_TBO
uniform mat4 in_boneMatrices[NUM_BONES];
#endif
uniform int in_numBoneWeights;
#ifdef HAS_boneOffset
in int in_boneOffset;
#endif
#endif // REGEN_boneInput_Included_
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
        texelFetch(in_boneMatrices, matIndex),
        texelFetch(in_boneMatrices, matIndex+1),
        texelFetch(in_boneMatrices, matIndex+2),
        texelFetch(in_boneMatrices, matIndex+3)
    );
}

-- fetchBoneMatrix
#ifdef HAS_BONES
#ifndef REGEN_fetchBoneMatrix_Included_
#define REGEN_fetchBoneMatrix_Included_
#include regen.states.bones.input
#ifdef USE_BONE_TBO
#include regen.states.bones.fetchBoneMatrixTBO
#else
#include regen.states.bones.fetchBoneMatrixArray
#endif // USE_BONE_TBO
#endif // REGEN_fetchBoneMatrix_Included_
#endif // HAS_BONES

-- transformBone
#ifdef HAS_BONES
#ifndef REGEN_transformBone_Included_
#define REGEN_transformBone_Included_
#include regen.states.bones.fetchBoneMatrix

vec4 transformBone(vec4 x) {
#if NUM_BONE_WEIGHTS==1
    return fetchBoneMatrix(in_boneIndices) * x;
#else
    vec4 y = vec4(0.0);
    #for INDEX to ${NUM_BONE_WEIGHTS}
    y += in_boneWeights[${INDEX}] * fetchBoneMatrix(in_boneIndices[${INDEX}]) * x;
    #endfor
    return y;
#endif
}

vec3 transformBone(vec3 x) {
#if NUM_BONE_WEIGHTS==1
    return mat3(fetchBoneMatrix(in_boneIndices)) * x;
#else
    vec3 y = vec3(0.0);
    #for INDEX to ${NUM_BONE_WEIGHTS}
    y += in_boneWeights[${INDEX}] * mat3(fetchBoneMatrix(in_boneIndices[${INDEX}])) * x;
    #endfor
    return y;
#endif
}
#endif // REGEN_transformBone_Included_
#endif // HAS_BONES
