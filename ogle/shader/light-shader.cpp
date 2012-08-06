/*
 * light-shader.cpp
 *
 *  Created on: 06.11.2011
 *      Author: daniel
 */

#include "light-shader.h"
#include <ogle/utility/string-util.h>

string LightShader::ambient(Light *light,
    const string &ambientVar,
    const string &materialAmbientVar) const
{
  stringstream s;
  switch(light->getLightType())
  {
  case Light::DIRECTIONAL:
  case Light::POINT:
  case Light::SPOT:
    s << "    " << ambientVar << " += " << materialAmbientVar <<
      " * lightAmbient" << light << ";" << endl;
    break;
  }
  return s.str();
}
string LightShader::diffuse(Light *light,
    const string &attenFacVar,
    const string &diffuseVar,
    const string &matDiffuseVar) const
{
  stringstream s;
  switch(light->getLightType())
  {
  case Light::DIRECTIONAL:
    s << "        " << diffuseVar << " += " << matDiffuseVar <<
      " * lightDiffuse" << light << " * nDotL;" << endl;
    break;
  case Light::POINT:
  case Light::SPOT:
    s << "        " << diffuseVar << " += " << attenFacVar << " * " << matDiffuseVar <<
      " * lightDiffuse" << light << " * nDotL;" << endl;
    break;
  }
  return s.str();
}
string LightShader::specular(Light *light,
    const string &attenFacVar,
    const string &specularVar,
    const string &matSpecularVar,
    const string &matShininessVar,
    const string &normalVar) const
{
  stringstream s;
  s << "        if(" << matShininessVar << " > 0.0) {" << endl;
  s << "            vec3 reflected = normalize( reflect( -normalizedLightVec, " << normalVar << " ) );" << endl;
  s << "            float rDotE = max( dot( reflected, posNormalized ), 0.0);" << endl;
  switch(light->getLightType())
  {
  case Light::DIRECTIONAL:
    s << "            " << specularVar << " += " << matSpecularVar << " * lightSpecular" <<
        light << " * pow(rDotE, " << matShininessVar << ");" << endl;
    break;
  case Light::POINT:
  case Light::SPOT:
    s << "            " << specularVar << " += " << matSpecularVar << " * lightSpecular" <<
        light << " * pow(rDotE, " << matShininessVar << ") * " << attenFacVar << ";" << endl;
    break;
  }
  s << "        }" << endl;
  return s.str();
}

string LightShader::lightVec(Light *light, const string &lightVecVar) const
{
  stringstream s;
  s << "    { // default light vec calculation" << endl;
  switch(light->getLightType())
  {
  case Light::DIRECTIONAL:
    s << "        " << lightVecVar << " = lightPosition" << light << ".xyz;" << endl;
    break;
  case Light::POINT:
  case Light::SPOT:
    s << "        " << lightVecVar << " = vec3( (lightPosition" << light << ").xyz - pos.xyz );" << endl;
    break;
  }
  s << "    }" << endl;
  return s.str();
}
string LightShader::attenFac(Light *light,
    const string &lightVecVar,
    const string &attenFacVar) const
{
  stringstream s;
  s << "    { // default attenuation factors" << endl;
  switch(light->getLightType())
  {
  case Light::DIRECTIONAL:
    break;
  case Light::POINT:
    s << "        float dist = length(" << lightVecVar << ");" << endl;
    s << "        " << attenFacVar << " = 1.0/(lightConstantAttenuation" << light << " +" << endl;
    s << "                        lightLinearAttenuation" << light << " * dist +" << endl;
    s << "                        lightQuadricAttenuation" << light << " * dist * dist );" << endl;
    break;
  case Light::SPOT:
    s << "        float spotEffect = dot( normalize(" << endl;
    s << "                    lightSpotDirection" << light << ")," << endl;
    s << "                    normalize( -" << lightVecVar << " ));" << endl;
    s << "        if (spotEffect > lightInnerConeAngle" << light << ") {" << endl;
    s << "            spotEffect = pow(spotEffect, lightSpotExponent" << light << ");" << endl;
    s << "            float dist = length(" << lightVecVar << ");" << endl;
    s << "            " << attenFacVar << " = spotEffect / (lightConstantAttenuation" << light << " +" << endl;
    s << "                        lightLinearAttenuation" << light << " * dist +" << endl;
    s << "                        lightQuadricAttenuation" << light << " * dist * dist);" << endl;
    s << "        } else {" << endl;
    s << "            " << attenFacVar << " = 0.0;" << endl;
    s << "        }" << endl;
    break;
  }
  s << "    }" << endl;
  return s.str();
}

//////////////////

GouradShadingFrag::GouradShadingFrag(
    vector<string> &args,
    const list< Light* > &lights,
    bool useFog)
: LightShader("gourad", args, lights, useFog)
{
  addInput( GLSLTransfer( "vec3", "f_lightAmbient" ) );
  addInput( GLSLTransfer( "vec3", "f_lightDiffuse" ) );
  addInput( GLSLTransfer( "vec3", "f_lightSpecular" ) );
}
string GouradShadingFrag::code() const
{
  stringstream s;
  s << "void gourad(in vec4 matAmbient, in vec4 matDiffuse, in vec4 matSpecular, "
      "out vec4 ambient, out vec4 diffuse, out vec4 specular)" << endl;
  s << "{" << endl;
  s << "    ambient = matAmbient;" << endl;
  s << "    diffuse = matDiffuse;" << endl;
  s << "    specular = matSpecular;" << endl;
  s << "}" << endl;
  return s.str();
}

GouradShadingVert::GouradShadingVert(
    vector<string> &args,
    const list< Light* > &lights,
    bool useFog)
: LightShader("gourad", args, lights, useFog)
{
  addOutput( GLSLTransfer( "vec3", "f_lightAmbient" ) );
  addOutput( GLSLTransfer( "vec3", "f_lightDiffuse" ) );
  addOutput( GLSLTransfer( "vec3", "f_lightSpecular" ) );
}
string GouradShadingVert::code() const
{
  stringstream s;
  int i = 0;
  s << "void gourad(vec4 pos, vec3 vertexNormal) {" << endl;
  s << "    vec4 _ambientTerm  = vec4(0.0);" << endl;
  s << "    vec4 _diffuseTerm  = vec4(0.0);" << endl;
  s << "    vec4 _specularTerm  = vec4(0.0);" << endl;
  s << "    vec3 posNormalized = normalize( -pos.xyz );" << endl;
  s << "    vec3 normalizedLightVec, lightVec;" << endl;
  s << "    float attenFac, nDotL;" << endl;
  s << "" << endl;
  for(list<Light*>::const_iterator it = lights_.begin();
                  it != lights_.end(); ++it)
  {
    Light *light = *it;
    s << "    // LIGHT" << light << endl;
    s << lightVec(light, "lightVec") << endl;
    s << attenFac(light, "lightVec", "attenFac") << endl << endl;
    s << "    " << endl;
    s << ambient(light, "_ambientTerm", "materialAmbient");
    s << "    " << endl;
    s << "    normalizedLightVec = normalize(  lightVec );" << endl;
    s << "    nDotL = max( dot( vertexNormal, normalizedLightVec ), 0.0 );" << endl;
    s << "    if (nDotL > 0.0) {" << endl;
    switch(light->getLightType())
    {
    case Light::SPOT:
      s << "        vec3 normalizedSpotDir = normalize(lightSpotDirection" << light << ");" << endl;
      s << "        float spotEffect = dot( -normalizedLightVec, normalizedSpotDir );" << endl;
      s << "        float coneDiff = (lightInnerConeAngle" << light <<
                                  " - lightOuterConeAngle" << light << ");" << endl;
      s << "        float falloff = clamp((spotEffect - lightOuterConeAngle" <<
          light << ") / coneDiff, 0.0, 1.0);" << endl;
      s << "    " << diffuse(light, "attenfac", "_diffuseTerm",
          "materialDiffuse*falloff") << endl;
      s << "    " << specular(light, "attenfac", "_specularTerm",
          "materialShininessStrength*falloff*materialSpecular",
          "materialShininess", "vertexNormal") << endl;
      break;
    default:
      s << diffuse(light, "attenFac", "_diffuseTerm", "materialDiffuse") << endl;
      s << specular(light, "attenFac", "_specularTerm",
          "materialShininessStrength*materialSpecular", "materialShininess", "vertexNormal") << endl;
    }
    s << "    }" << endl;
    ++i;
  }
  s << "    f_lightAmbient = _ambientTerm.rgb;" << endl;
  s << "    f_lightDiffuse = _diffuseTerm.rgb;" << endl;
  s << "    f_lightSpecular = _specularTerm.rgb;" << endl;
  s << "}" << endl;
  return s.str();
}

////////////////////

PhongShadingFrag::PhongShadingFrag(
    vector<string> &args,
    const list< Light* > &lights,
    bool useFog,
    const string &name)
: LightShader(name, args, lights, useFog)
{
  addInput( GLSLTransfer( "vec3", "f_lightVec", numLights_, true, "smooth" ) );
  addInput( GLSLTransfer( "float", "f_attenFacs", numLights_, true, "smooth" ) );
}
string PhongShadingFrag::code() const
{
  stringstream s;
  int i = 0;

  s << "void " << myName_ << "(vec4 pos, vec3 fragmentNormal, float brightness, " << endl;
  s << "           vec4 matAmbient, vec4 matDiffuse, vec4 matSpecular, " << endl;
  s << "           float matShininess, " << endl;
  s << "           float matShininessStrength, " << endl;
  s << "           inout vec4 ambient, inout vec4 diffuse, inout vec4 specular)" << endl;
  s << "{" << endl;
  s << "    vec3 posNormalized = normalize( -pos.xyz );" << endl;
  s << "    vec3 normalizedLightVec;" << endl;
  s << "    float nDotL;" << endl;
  s << "" << endl;
  for(list<Light*>::const_iterator it = lights_.begin();
                  it != lights_.end(); ++it)
  {
    Light *light = *it;
    s << "    // LIGHT" << light << endl;
    s << "    " << endl;
    s << ambient(light, "ambient", "matAmbient");
    s << "    normalizedLightVec = normalize(  lightVec[" << i << "] );" << endl;
    s << "    nDotL = max( dot( fragmentNormal, normalizedLightVec ), 0.0 );" << endl;
    s << "    if (nDotL > 0.0) {" << endl;
    switch(light->getLightType())
    {
    case Light::SPOT:
      s << "        vec3 normalizedSpotDir = normalize(lightSpotDirection" << light << ");" << endl;
      s << "        float spotEffect = dot( -normalizedLightVec, normalizedSpotDir );" << endl;
      s << "        float coneDiff = (lightInnerConeAngle" << light <<
                                  " - lightOuterConeAngle" << light << ");" << endl;
      s << "        float falloff = clamp((spotEffect - lightOuterConeAngle" <<
          light << ") / coneDiff, 0.0, 1.0);" << endl;
      s << diffuse(light, FORMAT_STRING("attenFacs[" << i << "]"), "diffuse", "matDiffuse*falloff") << endl;
      s << specular(light, FORMAT_STRING("attenFacs[" << i << "]"), "specular",
          "matShininessStrength*falloff*matSpecular",
          "matShininess",
          "fragmentNormal") << endl;
      break;
    default:
      s << diffuse(light, FORMAT_STRING("attenFacs[" << i << "]"),
          "diffuse", "matDiffuse") << endl;
      s << specular(light, FORMAT_STRING("attenFacs[" << i << "]"),
          "specular",
          "matShininessStrength*matSpecular", "matShininess", "fragmentNormal") << endl;
    }
    s << "    }" << endl;
    ++i;
  }
  s << "}" << endl;

  return s.str();
}

PhongShadingVert::PhongShadingVert(
    vector<string> &args,
    const list< Light* > &lights,
    bool useFog)
: LightShader("phong", args, lights, useFog)
{
  addOutput( GLSLTransfer( "vec3", "f_lightVec", numLights_, true, "smooth" ) );
  addOutput( GLSLTransfer( "float", "f_attenFacs", numLights_, true, "smooth" ) );
}
string PhongShadingVert::code() const
{
  stringstream s;
  int i = 0;

  s << "void phong(vec4 pos)" << endl;
  s << "{" << endl;
  for(list<Light*>::const_iterator it = lights_.begin(); it != lights_.end(); ++it)
  {
    Light *light = *it;
    s << lightVec(light, FORMAT_STRING("f_lightVec[" << i << "]")) << endl;
    s << attenFac(light, FORMAT_STRING("f_lightVec[" << i << "]"),
        FORMAT_STRING("attenFacs[" << i << "]")) << endl;
    ++i;
  }
  s << "}" << endl;
  return s.str();
}

////////////////

/**
void shade_diffuse_toon(vec3 n, vec3 l, vec3 v, float size, float tsmooth, out float is)
{
        float rslt = dot(n, l);
        float ang = acos(rslt);

        if(ang < size) is = 1.0;
        else if(ang > (size + tsmooth) || tsmooth == 0.0) is = 0.0;
        else is = 1.0 - ((ang - size)/tsmooth);
}
void shade_toon_spec(vec3 n, vec3 l, vec3 v, float size, float tsmooth, out float specfac)
{
        vec3 h = normalize(l + v);
        float rslt = dot(h, n);
        float ang = acos(rslt);

        if(ang < size) rslt = 1.0;
        else if(ang >= (size + tsmooth) || tsmooth == 0.0) rslt = 0.0;
        else rslt = 1.0 - ((ang - size)/tsmooth);

        specfac = rslt;
}
 */

ToonShadingFrag::ToonShadingFrag(
    vector<string> &args,
    const list< Light* > &lights, bool useFog)
: PhongShadingFrag(args, lights, useFog, "blinnShading")
{
}
string ToonShadingFrag::diffuse(Light *light,
    const string &attenFacVar,
    const string &diffuseVar,
    const string &matDiffuseVar) const {
  float size=1.0;
  float tsmooth=0.25;
  stringstream s;
  s << "        { // Toon diffuse light" << endl;
  s << "            float diffuseFactor;" << endl;

  s << "    float intensity = dot( normalizedLightVec , fragmentNormal );" << endl;
  s << "    vec3 eye = -pos.xyz;" << endl;
  s << "    if( intensity > 0.95 ) diffuseFactor = 1.0;" << endl;
  s << "    else if( intensity > 0.5  ) diffuseFactor = 0.7;" << endl;
  s << "    else if( intensity > 0.25 ) diffuseFactor = 0.4;" << endl;
  s << "    else diffuseFactor = 0.0;" << endl;
  s << "    " << diffuseVar << " = " << matDiffuseVar <<
      " * lightDiffuse" << light <<
      " * vec4 ( diffuseFactor, diffuseFactor, diffuseFactor, 1.0 );" << endl;
  s << "        }" << endl;
  return s.str();
}
string ToonShadingFrag::specular(Light *light,
    const string &attenFacVar,
    const string &specularVar,
    const string &matSpecularVar,
    const string &matShininessVar,
    const string &normalVar) const
{
  float size=0.5;
  float tsmooth=0.1;
  stringstream s;
  s << "        if(" << matShininessVar << " > 0.0) {" << endl;
  s << "            vec3 h = normalize(normalizedLightVec + pos.xyz);" << endl;
  s << "            float specfac = dot(h, fragmentNormal);" << endl;
  s << "            float ang = acos(specfac);" << endl;
  s << "            if(ang < " << size << ") specfac = 1.0;" << endl;
  s << "            else if(ang >= (" << size << " + " << tsmooth << ") || " <<
      tsmooth << " == 0.0) specfac = 0.0;" << endl;
  s << "            else specfac = 1.0 - ((ang - " << size << ")/" << tsmooth << ");" << endl;
  s << "            " << specularVar << " += " << matSpecularVar << " * lightSpecular" <<
      light << " * specfac;" << endl;
  s << "        }" << endl;
  return s.str();
}

//////////////////

BlinnShadingFrag::BlinnShadingFrag(vector<string> &args,
    const list< Light* > &lights, bool useFog)
: PhongShadingFrag(args, lights, useFog, "blinnShading")
{
}
string BlinnShadingFrag::specular(Light *light,
    const string &attenFacVar,
    const string &specularVar,
    const string &matSpecularVar,
    const string &matShininessVar,
    const string &normalVar) const
{
  stringstream s;
  s << "        if(" << matShininessVar << " > 0.0) {" << endl;
  s << "            vec3 halfVecNormalized = normalize( normalizedLightVec + posNormalized );" << endl;
  s << "            " << specularVar << " += " << matSpecularVar << " * lightSpecular" <<
      light << " * pow(max(0.0,dot("<<normalVar<<",halfVecNormalized)), " << matShininessVar << ");" << endl;
  s << "        }" << endl;
  return s.str();
}

//////////////////

OrenNayerShadingFrag::OrenNayerShadingFrag(vector<string> &args,
    const list< Light* > &lights, bool useFog)
: PhongShadingFrag(args, lights, useFog, "orenNayerShading")
{
}
string OrenNayerShadingFrag::diffuse(Light *light,
    const string &attenFacVar,
    const string &diffuseVar,
    const string &matDiffuseVar) const {
  stringstream s;
  s << "        { // Oren-Nayer diffuse light" << endl;
  s << "            float vDotN = dot(posNormalized, fragmentNormal);" << endl;
  s << "            float cos_theta_i = nDotL;" << endl;
  s << "            float theta_r = acos(vDotN);" << endl;
  s << "            float theta_i = acos(cos_theta_i);" << endl;
  s << "            float cos_phi_diff = dot(normalize(posNormalized-fragmentNormal*vDotN), " << endl;
  s << "                                     normalize(normalizedLightVec-fragmentNormal*nDotL));" << endl;
  s << "            float alpha = max(theta_i,theta_r);" << endl;
  s << "            float beta = min(theta_i,theta_r);" << endl;
  s << "            float r = materialRoughness*materialRoughness;" << endl;
  s << "            float a = 1.0-0.5*r/(r+0.33);" << endl;
  s << "            float b = 0.45*r/(r+0.09);" << endl;
  s << "            if (cos_phi_diff>=0) {" << endl;
  s << "                b*=sin(alpha)*tan(beta);" << endl;
  s << "            } else {" << endl;
  s << "                b=0.0;" << endl;
  s << "            }" << endl;
  s << "            float diffuseFactor = cos_theta_i * (a+b);" << endl;
  s << "            " << diffuseVar << " += " << matDiffuseVar << " * lightDiffuse" << light << " * diffuseFactor;" << endl;
  s << "        }" << endl;
  return s.str();
}

//////////////////

MinnaertShadingFrag::MinnaertShadingFrag(vector<string> &args,
    const list< Light* > &lights, bool useFog)
: PhongShadingFrag(args, lights, useFog, "minnaertShading")
{
  addUniform( (GLSLUniform) {"float", "materialDarkness"} );
}
string MinnaertShadingFrag::diffuse(Light *light,
    const string &attenFacVar,
    const string &diffuseVar,
    const string &matDiffuseVar) const {
  stringstream s;
  s << "        { // Minnaert diffuse light" << endl;
  s << "            " << diffuseVar << " += " << matDiffuseVar << " * lightDiffuse" <<
      light << " * pow( nDotL, materialDarkness);" << endl;
  s << "        }" << endl;
  return s.str();
}

//////////////////

CookTorranceShadingFrag::CookTorranceShadingFrag(vector<string> &args,
    const list< Light* > &lights, bool useFog)
: PhongShadingFrag(args, lights, useFog, "cookTorranceShading")
{
}
string CookTorranceShadingFrag::specular(Light *light,
    const string &attenFacVar,
    const string &specularVar,
    const string &matSpecularVar,
    const string &matShininessVar,
    const string &normalVar) const
{
  stringstream s;
  s << "        if(" << matShininessVar << " > 0.0) {" << endl;
  s << "            vec3 halfVecNormalized = normalize( normalizedLightVec + posNormalized );" << endl;
  s << "            float nDotH = dot(fragmentNormal, halfVecNormalized);" << endl;
  s << "            if(nDotH >= 0.0) {" << endl;
  s << "                float nDotV = max(dot(fragmentNormal, posNormalized), 0.0);" << endl;
  s << "                float specularFactor = pow(nDotH, " << matShininessVar << ");" << endl;
  s << "                specularFactor = specularFactor/(0.1+nDotV);" << endl;
  s << "                " << specularVar << " += " << matSpecularVar << " * lightSpecular" << light << " * specularFactor;" << endl;
  s << "            }" << endl;
  s << "        }" << endl;
  return s.str();
}
