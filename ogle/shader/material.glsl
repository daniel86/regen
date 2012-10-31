
-- input
#ifdef HAS_MATERIAL
uniform vec4 in_matAmbient;
uniform vec4 in_matDiffuse;
uniform vec4 in_matSpecular;
uniform vec4 in_matEmission;
uniform float in_matShininess;
uniform float in_matShininessStrength;
uniform float in_matRefractionIndex;
#if SHADING == ORENNAYER
  uniform float in_matRoughness;
#endif
#if SHADING == MINNAERT
  uniform float in_matDarkness;
#endif
#ifdef HAS_ALPHA
  uniform float in_matAlpha;
#endif
#endif

-- apply
#ifdef HAS_MATERIAL
#ifdef HAS_SHADING
void materialShading(inout Shading sh) {
    sh.ambient *= in_matAmbient;
    sh.diffuse *= in_matDiffuse;
    sh.specular *= in_matShininessStrength * in_matSpecular;
    sh.emission *= in_matEmission;
}
#endif
#endif

