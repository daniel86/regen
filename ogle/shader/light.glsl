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
      " * in_lightAmbient" << light << ";" << endl;
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
      " * in_lightDiffuse" << light << " * nDotL;" << endl;
    break;
  case Light::POINT:
  case Light::SPOT:
    s << "        " << diffuseVar << " += " << attenFacVar << " * " << matDiffuseVar <<
      " * in_lightDiffuse" << light << " * nDotL;" << endl;
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
    s << "            " << specularVar << " += " << matSpecularVar << " * in_lightSpecular" <<
        light << " * pow(rDotE, " << matShininessVar << ");" << endl;
    break;
  case Light::POINT:
  case Light::SPOT:
    s << "            " << specularVar << " += " << matSpecularVar << " * in_lightSpecular" <<
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
    s << "        " << lightVecVar << " = in_lightPosition" << light << ".xyz;" << endl;
    break;
  case Light::POINT:
  case Light::SPOT:
    s << "        " << lightVecVar << " = vec3( (in_lightPosition" << light << ").xyz - pos.xyz );" << endl;
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
    s << "        " << attenFacVar << " = 1.0/(in_lightConstantAttenuation" << light << " +" << endl;
    s << "                        in_lightLinearAttenuation" << light << " * dist +" << endl;
    s << "                        in_lightQuadricAttenuation" << light << " * dist * dist );" << endl;
    break;
  case Light::SPOT:
    s << "        float spotEffect = dot( normalize(" << endl;
    s << "                    in_lightSpotDirection" << light << ")," << endl;
    s << "                    normalize( -" << lightVecVar << " ));" << endl;
    s << "        if (spotEffect > in_lightInnerConeAngle" << light << ") {" << endl;
    s << "            spotEffect = pow(spotEffect, in_lightSpotExponent" << light << ");" << endl;
    s << "            float dist = length(" << lightVecVar << ");" << endl;
    s << "            " << attenFacVar << " = spotEffect / (in_lightConstantAttenuation" << light << " +" << endl;
    s << "                        in_lightLinearAttenuation" << light << " * dist +" << endl;
    s << "                        in_lightQuadricAttenuation" << light << " * dist * dist);" << endl;
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
    const list< Light* > &lights)
: LightShader("gourad", args, lights)
{
  addInput( GLSLTransfer( "vec3", "in_lightAmbient" ) );
  addInput( GLSLTransfer( "vec3", "in_lightDiffuse" ) );
  addInput( GLSLTransfer( "vec3", "in_lightSpecular" ) );
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
    const list< Light* > &lights)
: LightShader("gourad", args, lights)
{
  addOutput( GLSLTransfer( "vec3", "out_lightAmbient" ) );
  addOutput( GLSLTransfer( "vec3", "out_lightDiffuse" ) );
  addOutput( GLSLTransfer( "vec3", "out_lightSpecular" ) );
}
string GouradShadingVert::code() const
{
  stringstream s;
  int i = 0;
  s << "void gourad(vec4 pos, vec3 vertexNormal, " << endl;
  s << "           vec4 matAmbient, vec4 matDiffuse, vec4 matSpecular, " << endl;
  s << "           float matShininess, " << endl;
  s << "           float matShininessStrength) {" << endl;
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
    s << ambient(light, "_ambientTerm", "matAmbient");
    s << "    " << endl;
    s << "    normalizedLightVec = normalize(  lightVec );" << endl;
    s << "    nDotL = max( dot( vertexNormal, normalizedLightVec ), 0.0 );" << endl;
    s << "    if (nDotL > 0.0) {" << endl;
    switch(light->getLightType())
    {
    case Light::SPOT:
      s << "        vec3 normalizedSpotDir = normalize(in_lightSpotDirection" << light << ");" << endl;
      s << "        float spotEffect = dot( -normalizedLightVec, normalizedSpotDir );" << endl;
      s << "        float coneDiff = (in_lightInnerConeAngle" << light <<
                                  " - in_lightOuterConeAngle" << light << ");" << endl;
      s << "        float falloff = clamp((spotEffect - in_lightOuterConeAngle" <<
          light << ") / coneDiff, 0.0, 1.0);" << endl;
      s << "    " << diffuse(light, "attenfac", "_diffuseTerm",
          "matDiffuse*falloff") << endl;
      s << "    " << specular(light, "attenfac", "_specularTerm",
          "matShininessStrength*falloff*matSpecular",
          "matShininess", "vertexNormal") << endl;
      break;
    default:
      s << diffuse(light, "attenFac", "_diffuseTerm", "matDiffuse") << endl;
      s << specular(light, "attenFac", "_specularTerm",
          "matShininessStrength*matSpecular", "matShininess", "vertexNormal") << endl;
    }
    s << "    }" << endl;
    ++i;
  }
  s << "    out_lightAmbient = _ambientTerm.rgb;" << endl;
  s << "    out_lightDiffuse = _diffuseTerm.rgb;" << endl;
  s << "    out_lightSpecular = _specularTerm.rgb;" << endl;
  s << "}" << endl;
  return s.str();
}

////////////////////

PhongShadingFrag::PhongShadingFrag(
    vector<string> &args,
    const list< Light* > &lights,
    const string &name)
: LightShader(name, args, lights)
{
  addInput( GLSLTransfer( "vec3", "in_lightVec", numLights_, true, FRAGMENT_INTERPOLATION_SMOOTH ) );
  addInput( GLSLTransfer( "float", "in_attenFacs", numLights_, true, FRAGMENT_INTERPOLATION_SMOOTH ) );
}
string PhongShadingFrag::code() const
{
  stringstream s;
  int i = 0;

  s << "void " << myName_ << "(vec4 pos, vec3 fragmentNormal, " << endl;
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
    s << "    normalizedLightVec = normalize(  in_lightVec[" << i << "] );" << endl;
    s << "    nDotL = max( dot( fragmentNormal, normalizedLightVec ), 0.0 );" << endl;
    s << "    if (nDotL > 0.0) {" << endl;
    switch(light->getLightType())
    {
    case Light::SPOT:
      s << "        vec3 normalizedSpotDir = normalize(in_lightSpotDirection" << light << ");" << endl;
      s << "        float spotEffect = dot( -normalizedLightVec, normalizedSpotDir );" << endl;
      s << "        float coneDiff = (in_lightInnerConeAngle" << light <<
                                  " - in_lightOuterConeAngle" << light << ");" << endl;
      s << "        float falloff = clamp((spotEffect - in_lightOuterConeAngle" <<
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
    const list< Light* > &lights)
: LightShader("phong", args, lights)
{
  addOutput( GLSLTransfer( "vec3", "out_lightVec", numLights_, true, FRAGMENT_INTERPOLATION_SMOOTH ) );
  addOutput( GLSLTransfer( "float", "out_attenFacs", numLights_, true, FRAGMENT_INTERPOLATION_SMOOTH ) );
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
    s << lightVec(light, FORMAT_STRING("out_lightVec[" << i << "]")) << endl;
    s << attenFac(light, FORMAT_STRING("out_lightVec[" << i << "]"),
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
    const list< Light* > &lights)
: PhongShadingFrag(args, lights, "blinnShading")
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
  s << "            " << specularVar << " += " << matSpecularVar << " * in_lightSpecular" <<
      light << " * specfac;" << endl;
  s << "        }" << endl;
  return s.str();
}

//////////////////

BlinnShadingFrag::BlinnShadingFrag(vector<string> &args,
    const list< Light* > &lights)
: PhongShadingFrag(args, lights, "blinnShading")
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
  s << "            " << specularVar << " += " << matSpecularVar << " * in_lightSpecular" <<
      light << " * pow(max(0.0,dot("<<normalVar<<",halfVecNormalized)), " << matShininessVar << ");" << endl;
  s << "        }" << endl;
  return s.str();
}

//////////////////

OrenNayerShadingFrag::OrenNayerShadingFrag(vector<string> &args,
    const list< Light* > &lights)
: PhongShadingFrag(args, lights, "orenNayerShading")
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
  s << "            float r = in_materialRoughness*in_materialRoughness;" << endl;
  s << "            float a = 1.0-0.5*r/(r+0.33);" << endl;
  s << "            float b = 0.45*r/(r+0.09);" << endl;
  s << "            if (cos_phi_diff>=0) {" << endl;
  s << "                b*=sin(alpha)*tan(beta);" << endl;
  s << "            } else {" << endl;
  s << "                b=0.0;" << endl;
  s << "            }" << endl;
  s << "            float diffuseFactor = cos_theta_i * (a+b);" << endl;
  s << "            " << diffuseVar << " += " << matDiffuseVar << " * in_lightDiffuse" << light << " * diffuseFactor;" << endl;
  s << "        }" << endl;
  return s.str();
}

//////////////////

MinnaertShadingFrag::MinnaertShadingFrag(vector<string> &args,
    const list< Light* > &lights)
: PhongShadingFrag(args, lights, "minnaertShading")
{
  addConstant( GLSLConstant("float", "in_materialDarkness", "1.0") );
}
string MinnaertShadingFrag::diffuse(Light *light,
    const string &attenFacVar,
    const string &diffuseVar,
    const string &matDiffuseVar) const {
  stringstream s;
  s << "        { // Minnaert diffuse light" << endl;
  s << "            " << diffuseVar << " += " << matDiffuseVar << " * in_lightDiffuse" <<
      light << " * pow( nDotL, in_materialDarkness);" << endl;
  s << "        }" << endl;
  return s.str();
}

//////////////////

CookTorranceShadingFrag::CookTorranceShadingFrag(vector<string> &args,
    const list< Light* > &lights)
: PhongShadingFrag(args, lights, "cookTorranceShading")
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
  s << "                " << specularVar << " += " << matSpecularVar << " * in_lightSpecular" << light << " * specularFactor;" << endl;
  s << "            }" << endl;
  s << "        }" << endl;
  return s.str();
}

/*
 * normal_mapping.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include <sstream>

#include "normal-mapping.h"
#include <ogle/gl-types/vertex-attribute.h>

BumpMapFrag::BumpMapFrag(vector<string> &args, GLboolean isTwoSided)
: ShaderFunctions("bump", args),
  isTwoSided_(isTwoSided)
{
}

string BumpMapFrag::code() const
{
  stringstream s;
  s << "void bump(vec4 texel, inout vec3 normal)" << endl;
  s << "{" << endl;
  s << "    normal = normalize( texel.xyz * 2.0 - vec3(1.0) );" << endl;
  if(isTwoSided_) {
    s << "    if(!gl_FrontFacing) { normal *= -1.0; };" << endl;
  }
  s << "}" << endl;
  return s.str();
}

BumpMapVert::BumpMapVert(vector<string> &args,
    const list<Light*> &lights)
: ShaderFunctions("bump", args),
  lights_(lights)
{

}
string BumpMapVert::code() const
{
  stringstream s;
  s << "void bump(vec3 vnor, vec4 vtan, vec4 vpos, out vec4 posTangent)" << endl;
  s << "{" << endl;
  s << "    // get the tangent in eye space (multiplication by gl_NormalMatrix transforms to eye space)" << endl;
  s << "    // the tangent should point in positive u direction on the uv plane in the tangent space." << endl;
  s << "    vec3 t = normalize( vtan.xyz );" << endl;
  s << "    // calculate the binormal, cross makes sure tbn matrix is orthogonal" << endl;
  s << "    // multiplicated by handeness." << endl;
  s << "    vec3 b = cross(vnor, t) * vtan.w;" << endl;
  s << "    // transpose tbn matrix will do the transformation to tangent space" << endl;
  s << "    vec3 buf;" << endl;
  s << "" << endl;
  s << "    // do the transformation of the eye vector (used for specuar light)" << endl;
  s << "    buf.x = dot( vpos.xyz, t );" << endl;
  s << "    buf.y = dot( vpos.xyz, b );" << endl;
  s << "    buf.z = dot( vpos.xyz, vnor );" << endl;
  s << "    posTangent = normalize( vec4(buf,vpos.w) );" << endl;
  s << "" << endl;
  s << "    // do the transformation of the light vectors" << endl;

  for(unsigned int i=0; i<lights_.size(); ++i)
  {
    s << "    buf.x = dot( out_lightVec[" << i << "], t );" << endl;
    s << "    buf.y = dot( out_lightVec[" << i << "], b );" << endl;
    s << "    buf.z = dot( out_lightVec[" << i << "], vnor ) ;" << endl;
    s << "    out_lightVec[" << i << "] = normalize( buf  );" << endl;
  }
  s << "}" << endl;

  return s.str();
}


