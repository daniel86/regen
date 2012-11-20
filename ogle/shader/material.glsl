
-- input
#ifdef HAS_MATERIAL
uniform vec3 in_matAmbient;
uniform vec3 in_matDiffuse;
uniform vec3 in_matSpecular;
uniform float in_matShininess;
uniform float in_matRefractionIndex;
#ifdef HAS_ALPHA
  uniform float in_matAlpha;
#endif
#endif

