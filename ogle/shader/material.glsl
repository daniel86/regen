
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
void materialShading(
        inout vec4 ambient,
        inout vec4 diffuse,
        inout vec4 specular,
        inout vec4 emission,
        inout float shininess)
{
    ambient *= in_matAmbient;
    diffuse *= in_matDiffuse;
    specular *= in_matShininessStrength * in_matSpecular;
    emission *= in_matEmission;
}
#endif
#endif

