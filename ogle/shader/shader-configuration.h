/*
 * shader-configuration.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef SHADER_CONFIGURATION_H_
#define SHADER_CONFIGURATION_H_

#include <set>
#include <list>
using namespace std;

#include <ogle/gl-types/shader-input.h>
#include <ogle/gl-types/texture.h>
#include <ogle/gl-types/tesselation-config.h>
#include <ogle/shader/shader-fragment-output.h>

class State;

class ShaderConfiguration
{
public:
  ShaderConfiguration();

  void setUseFog();
  GLboolean useFog() const;

  void setIgnoreCameraRotation();
  GLboolean ignoreCameraRotation() const;

  void setIgnoreCameraTranslation();
  GLboolean ignoreCameraTranslation() const;

  void setMaterial(State *material);
  const State* material() const;

  void addLight(State *light);
  const set<State*>& lights() const;

  void addTexture(State *tex);
  map<string,State*>& textures();

  void setShaderInput(ShaderInput*);
  map<string,ShaderInput*>& inputs();

  /**
   * Used to set up transform feedback between shader compiling and linking.
   */
  void setTransformFeedbackAttribute(ShaderInput*);
  /**
   * Used to set up transform feedback between shader compiling and linking.
   */
  map<string,ShaderInput*>& transformFeedbackAttributes();

  void setTesselationCfg(const Tesselation &tessCfg);
  const Tesselation& tessCfg() const;
  GLboolean useTesselation() const;

  void setNumBoneWeights(GLuint numBoneWeights);
  GLuint maxNumBoneWeights() const;

  void setFragmentOutputs(
      list< ref_ptr<ShaderFragmentOutput> > &fragmentOutputs);
  list<ShaderFragmentOutput*> fragmentOutputs() const;

protected:
  set<State*> lights_;

  map<string,State*> textures_;

  State* material_;

  map<string,ShaderInput*> inputs_;
  map<string,ShaderInput*> transformFeedbackAttributes_;

  list<ShaderFragmentOutput*> fragmentOutputs_;

  Tesselation tessCfg_;
  GLboolean useTesselation_;

  GLboolean ignoreCameraRotation_;
  GLboolean ignoreCameraTranslation_;
  GLboolean useFog_;

  GLuint maxNumBoneWeights_;
};

#endif /* SHADER_CONFIGURATION_H_ */
