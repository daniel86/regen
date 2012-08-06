/*
 * light-shader.h
 *
 *  Created on: 06.11.2011
 *      Author: daniel
 */

#ifndef LIGHT_SHADER_H_
#define LIGHT_SHADER_H_

using namespace std;
#include <vector>
#include <string>

#include <ogle/shader/shader-function.h>
#include <ogle/states/light-state.h>

/**
 * Base class for shaders implementing the light calculations.
 */
class LightShader: public ShaderFunctions {
public:
  LightShader(const string &name, vector<string> &args,
      const list< Light* > &lights, bool useFog)
  : ShaderFunctions(name, args),
    lights_(lights),
    numLights_(lights.size()),
    useFog_(useFog)
  {}

  /**
   * Default ambient term for the specified light.
   */
  virtual string ambient(Light *light,
      const string &ambientVar,
      const string &matAmbientVar) const;
  /**
   * Default diffuse term for the specified light.
   */
  virtual string diffuse(Light *light,
      const string &attenFacVar,
      const string &diffuseVar,
      const string &matDiffuseVar) const;
  /**
   * Default specular term for the specified light.
   */
  virtual string specular(Light *light,
      const string &attenFacVar,
      const string &specularVar,
      const string &matSpecularVar,
      const string &matShininessVar,
      const string &normalVar) const;
  /**
   * Default light vector calculations for specified light.
   */
  virtual string lightVec(Light *light,
      const string &lightVecVar) const;
  /**
   * Default attenuation factor calculations for specified light.
   */
  virtual string attenFac(Light *light,
      const string &lightVecVar, const string &attenFacVar) const;
protected:
  unsigned int numLights_;
  list< Light* > lights_;
  bool useFog_;
};

/**
 * Gourad shading calculates the light intensity in the
 * vertex shader and interpolates the value for fragments.
 */
class GouradShadingVert : public LightShader {
public:
  GouradShadingVert(vector<string> &args, const list< Light* > &lights, bool useFog);
  string code() const;
};
/**
 * Gourad calculates color with precomputed light intensities.
 */
class GouradShadingFrag : public LightShader {
public:
  GouradShadingFrag(vector<string> &args, const list< Light* > &lights, bool useFog);
  string code() const;
};

/**
 * Calculates light intensity using interpolated light vector.
 */
class PhongShadingFrag : public LightShader {
public:
  PhongShadingFrag(vector<string> &args, const list< Light* > &lights, bool useFog, const string &name="phong");
  string code() const;
};
/**
 * Saves the light vector for light intensity calculation
 * in the fragment shader.
 */
class PhongShadingVert : public LightShader {
public:
  PhongShadingVert(vector<string> &args, const list< Light* > &lights, bool useFog);
  string code() const;
};

class ToonShadingFrag : public PhongShadingFrag {
public:
  ToonShadingFrag(vector<string> &args, const list< Light* > &lights, bool useFog);
  virtual string diffuse(Light *light,
      const string &attenFacVar,
      const string &diffuseVar,
      const string &matDiffuseVar) const;
  virtual string specular(Light *light,
      const string &attenFacVar,
      const string &specularVar,
      const string &matSpecularVar,
      const string &matShininessVar,
      const string &normalVar) const;
};
typedef PhongShadingVert ToonShadingVert;

/**
 * Phong-lightning with Oren-Nayer diffuse calculation.
 */
class OrenNayerShadingFrag : public PhongShadingFrag {
public:
  OrenNayerShadingFrag(vector<string> &args, const list< Light* > &lights, bool useFog);
  virtual string diffuse(Light *light,
      const string &attenFacVar,
      const string &diffuseVar,
      const string &matDiffuseVar) const;
};
typedef PhongShadingVert OrenNayerShadingVert;

/**
 * Phong-lightning with Minnaert diffuse calculation.
 */
class MinnaertShadingFrag : public PhongShadingFrag {
public:
  MinnaertShadingFrag(vector<string> &args, const list< Light* > &lights, bool useFog);
  virtual string diffuse(Light *light,
      const string &attenFacVar,
      const string &diffuseVar,
      const string &matDiffuseVar) const;
};
typedef PhongShadingVert MinnaertShadingVert;

/**
 * Phong-lightning with blinn specular calculation.
 */
class BlinnShadingFrag : public PhongShadingFrag {
public:
  BlinnShadingFrag(vector<string> &args, const list< Light* > &lights, bool useFog);
  virtual string specular(Light *light,
      const string &attenFacVar,
      const string &specularVar,
      const string &matSpecularVar,
      const string &matShininessVar,
      const string &normalVar) const;
};
typedef PhongShadingVert BlinnShadingVert;

/**
 * Phong-lightning with Cook-Torrance specular calculation.
 */
class CookTorranceShadingFrag : public PhongShadingFrag {
public:
  CookTorranceShadingFrag(vector<string> &args, const list< Light* > &lights, bool useFog);
  virtual string specular(Light *light,
      const string &attenFacVar,
      const string &specularVar,
      const string &matSpecularVar,
      const string &matShininessVar,
      const string &normalVar) const;
};
typedef PhongShadingVert CookTorranceShadingVert;

#endif /* LIGHT_SHADER_H_ */
