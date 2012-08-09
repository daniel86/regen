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

#include <ogle/gl-types/vertex-attribute.h>
#include <ogle/gl-types/texture.h>
#include <ogle/gl-types/tesselation-config.h>
#include <ogle/shader/shader-fragment-output.h>

class State;

class ShaderConfiguration
{
public:
  ShaderConfiguration();

  list< ref_ptr<VertexAttribute> > attributes;
  list< ref_ptr<VertexAttribute> > transformFeedbackAttributes;
  list<State*> textures;
  list<State*> lights;
  list<ShaderFragmentOutput*> fragmentOutputs;
  State* material;
  Tesselation tessCfg;
  bool hasBones;
  bool useInstancing;
  bool useFog;
  bool useTesselation;
  bool ignoreCameraRotation;
  bool ignoreCameraTranslation;

  void setUseInstancing(bool);
  void setUseFog(bool);
  void setUseTesselation(bool);

  void setHasBones(bool);
  void setNumBoneWeights(GLuint);

  void setAttributes(const list< ref_ptr<VertexAttribute> >&);
  void addAttribute(ref_ptr<VertexAttribute>&);

  void setTransformFeedbackAttributes(const list< ref_ptr<VertexAttribute> >&);
  void addTransformFeedbackAttribute(ref_ptr<VertexAttribute>&);

  void addTexture(State *tex);

  void addLight(State *light);

  void setMaterial(State *material);

  void setFragmentOutputs(
      list< ref_ptr<ShaderFragmentOutput> > &fragmentOutputs);

  void setTesselationCfg(const Tesselation &tessCfg);
};

#endif /* SHADER_CONFIGURATION_H_ */
