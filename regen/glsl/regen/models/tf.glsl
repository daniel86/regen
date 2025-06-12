
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
#include regen.models.tf.input
#include regen.states.bones.transformBone

vec4 transformModel(vec4 posModel) {
    #ifndef IGNORE_modelOffset
        #ifdef HAS_modelOffset
    posModel.xyz += in_modelOffset.xyz;
        #endif
    #endif // IGNORE_modelOffset
    #ifndef IGNORE_bones
        #ifdef HAS_BONES
    posModel = transformBone(posModel);
        #endif
    #endif // IGNORE_bones
    #ifndef IGNORE_modelMatrix
        #ifdef HAS_modelMatrix
    posModel = in_modelMatrix * posModel;
        #endif
    #endif // IGNORE_modelMatrix
    return posModel;
}

vec3 transformModel(vec3 norModel) {
    #ifndef IGNORE_bones
        #ifdef HAS_BONES
    norModel = transformBone(norModel);
        #endif
    #endif // IGNORE_bones
    #ifndef IGNORE_modelMatrix
        #ifdef HAS_modelMatrix
    norModel = mat3(in_modelMatrix) * norModel;
        #endif
    #endif // IGNORE_modelMatrix
    return norModel;
}
#endif
