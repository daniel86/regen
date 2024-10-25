
-- defines
#if HAS_clipPlane || RENDER_TARGET == DUAL_PARABOLOID || RENDER_TARGET == PARABOLOID
#define HAS_CLIPPING
#endif

-- input
#ifndef REGEN_clipInput_Included_
#define REGEN_clipInput_Included_
#include regen.states.clipping.defines
#ifdef HAS_clipPlane
uniform vec4 in_clipPlane;
#endif
#if RENDER_TARGET == DUAL_PARABOLOID || RENDER_TARGET == PARABOLOID
const float in_paraboloidClipThreshold = 0.1;
#endif
#endif // REGEN_clipInput_Included_

-- isClipped
#ifndef REGEN_isClipped_Included_
#define REGEN_isClipped_Included_
#include regen.states.clipping.input
#ifdef HAS_CLIPPING
bool isClipped(vec3 posWorld)
{
#if RENDER_TARGET == DUAL_PARABOLOID || RENDER_TARGET == PARABOLOID
  if(in_posEye.z<-in_paraboloidClipThreshold) return true;
#endif
#ifdef HAS_clipPlane
  if(dot(posWorld,in_clipPlane.xyz)-in_clipPlane.w<=0.0) return true;
#endif
  return false;
}
#else
#define isClipped(x) false
#endif // HAS_CLIPPING
#endif // REGEN_isClipped_Included_
