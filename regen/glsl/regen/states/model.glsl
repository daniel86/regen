
-- input
#ifndef REGEN_modelInput_INCLUDED
#define2 REGEN_modelInput_INCLUDED
#ifdef HAS_modelMatrix
uniform mat4 in_modelMatrix;
#endif
#endif

-- transformModel
#ifndef REGEN_transformModel_INCLUDED
#define2 REGEN_transformModel_INCLUDED
#include regen.states.model.input
#include regen.states.bones.transformBone

vec4 transformModel(vec4 posModel) {
#if HAS_BONES && HAS_modelMatrix
    return in_modelMatrix * transformBone(posModel);
#elif HAS_BONES
    return transformBone(posModel);
#elif HAS_modelMatrix
    return in_modelMatrix * posModel;
#else
    return posModel;
#endif
}
#endif
