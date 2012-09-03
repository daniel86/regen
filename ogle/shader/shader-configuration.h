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

  void setUseFog(GLboolean toggle=GL_TRUE);
  GLboolean useFog() const;

  void setIgnoreCameraRotation();
  GLboolean ignoreCameraRotation() const;

  void setIgnoreCameraTranslation();
  GLboolean ignoreCameraTranslation() const;

  void setMaterial(State *material);
  const State* material() const;

  void addLight(State *light);
  set<State*>& lights();

  void addTexture(State *tex);
  map<string,State*>& textures();

  void setShaderInput(ref_ptr<ShaderInput>&);
  map< string, ref_ptr<ShaderInput> >& inputs();

  /**
   * Used to set up transform feedback between shader compiling and linking.
   */
  void setTransformFeedbackAttributes(list< ref_ptr<VertexAttribute> >&);
  /**
   * Used to set up transform feedback between shader compiling and linking.
   */
  list< ref_ptr<VertexAttribute> >& transformFeedbackAttributes();

  void setTesselationCfg(const Tesselation &tessCfg);
  const Tesselation& tessCfg() const;
  GLboolean useTesselation() const;

  void setNumBoneWeights(GLuint numBoneWeights);
  GLuint maxNumBoneWeights() const;

  void setFragmentOutputs(
      list< ref_ptr<ShaderFragmentOutput> > &fragmentOutputs);
  list<ShaderFragmentOutput*>& fragmentOutputs();

protected:
  set<State*> lights_;

  map<string,State*> textures_;

  State* material_;

  map< string, ref_ptr<ShaderInput> > inputs_;
  list< ref_ptr<VertexAttribute> > transformFeedbackAttributes_;

  list<ShaderFragmentOutput*> fragmentOutputs_;

  Tesselation tessCfg_;
  GLboolean useTesselation_;

  GLboolean ignoreCameraRotation_;
  GLboolean ignoreCameraTranslation_;
  GLboolean useFog_;

  GLuint maxNumBoneWeights_;
};

#endif /* SHADER_CONFIGURATION_H_ */
