
-- regen_InstanceID
#ifndef regen_InstanceID_defined_
#define2 regen_InstanceID_defined_
#ifdef HAS_instanceIDMap
    #define regen_InstanceID in_instanceIDMap[gl_InstanceID]
#else
    #define regen_InstanceID gl_InstanceID
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
