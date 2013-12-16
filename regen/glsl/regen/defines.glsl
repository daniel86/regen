
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
