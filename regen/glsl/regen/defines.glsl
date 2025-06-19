
-- regen_InstanceID
#ifndef regen_InstanceID_defined_
#define2 regen_InstanceID_defined_

#ifdef HAS_LOD && HAS_INSTANCES
    #ifndef HAS_instanceIDMap
#error "HAS_LOD + HAS_INSTANCES defined but HAS_instanceIDMap not defined"
    #endif
#endif

#if SHADER_STAGE==fs
    #ifdef HAS_instanceIDMap
#define regen_InstanceID in_instanceIDMap[in_instanceID]
    #else // HAS_instanceIDMap
#define regen_InstanceID in_instanceID
    #endif // HAS_instanceIDMap
#elif SHADER_STAGE==gs
    #ifdef HAS_instanceIDMap
#define regen_InstanceID in_instanceIDMap[in_instanceID[0]]
    #else // HAS_instanceIDMap
#define regen_InstanceID in_instanceID[0]
    #endif // HAS_instanceIDMap
#else
    #ifdef HAS_instanceIDMap
#define regen_InstanceID in_instanceIDMap[gl_InstanceID + gl_BaseInstance]
    #else
#define regen_InstanceID (gl_InstanceID + gl_BaseInstance)
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
