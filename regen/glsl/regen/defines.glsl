
-- regen_InstanceID
#ifndef regen_InstanceID_defined_
#define2 regen_InstanceID_defined_
#if SHADER_STAGE==fs
    #ifdef HAS_instanceIDMap
        #ifdef HAS_instanceIDOffset
#define regen_InstanceID in_instanceIDMap[in_instanceID + in_instanceIDOffset]
        #else
#define regen_InstanceID in_instanceIDMap[in_instanceID]
        #endif
    #else
#define regen_InstanceID in_instanceID
    #endif
#else
    #ifdef HAS_instanceIDMap
        #ifdef HAS_instanceIDOffset
#define regen_InstanceID in_instanceIDMap[gl_InstanceID + in_instanceIDOffset]
        #else
#define regen_InstanceID in_instanceIDMap[gl_InstanceID]
        #endif
    #else
#define regen_InstanceID gl_InstanceID
    #endif
#endif
#endif // regen_InstanceID_defined_

-- all
#ifdef HAS_nor && HAS_tan
#define HAS_TANGENT_SPACE
#endif
#if SHADER_STAGE == tes
#define SAMPLE(T,C) texture(T,INTERPOLATE_VALUE(C))
#else
#define SAMPLE(T,C) texture(T,C)
#endif
#ifndef PI
#define PI 3.14159265
#endif
