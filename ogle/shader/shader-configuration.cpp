/*
 * shader-configuration.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "shader-configuration.h"

#include <ogle/states/texture-state.h>
#include <ogle/states/attribute-state.h>

ShaderConfiguration::ShaderConfiguration()
: maxNumBoneWeights_(0),
  useTesselation_(false),
  ignoreCameraTranslation_(false),
  ignoreCameraRotation_(false),
  useFog_(false),
  material_(NULL),
  tessCfg_(TESS_PRIMITVE_TRIANGLES, 0)
{
}

void ShaderConfiguration::setUseFog()
{
  useFog_ = true;
}
GLboolean ShaderConfiguration::useFog() const
{
  return useFog_;
}

void ShaderConfiguration::setIgnoreCameraRotation()
{
  ignoreCameraRotation_ = true;
}
GLboolean ShaderConfiguration::ignoreCameraRotation() const
{
  return ignoreCameraRotation_;
}

void ShaderConfiguration::setIgnoreCameraTranslation()
{
  ignoreCameraTranslation_ = true;
}
GLboolean ShaderConfiguration::ignoreCameraTranslation() const
{
  return ignoreCameraTranslation_;
}

void ShaderConfiguration::setTesselationCfg(const Tesselation &tessCfg)
{
  if(!useTesselation_) {
    tessCfg_ = tessCfg;
    useTesselation_ = true;
  }
}
const Tesselation& ShaderConfiguration::tessCfg() const
{
  return tessCfg_;
}
GLboolean ShaderConfiguration::useTesselation() const
{
  return useTesselation_;
}

void ShaderConfiguration::setNumBoneWeights(GLuint numBoneWeights)
{
  if(numBoneWeights>maxNumBoneWeights_) {
    maxNumBoneWeights_ = numBoneWeights;
  }
}
GLuint ShaderConfiguration::maxNumBoneWeights() const
{
  return maxNumBoneWeights_;
}

void ShaderConfiguration::setMaterial(State *material)
{
  if(!material_) {
    material_ = material;
  }
}
const State* ShaderConfiguration::material() const
{
  return material_;
}

void ShaderConfiguration::addLight(State *light)
{
  lights_.insert(light);
}
const set<State*>& ShaderConfiguration::lights() const
{
  return lights_;
}

void ShaderConfiguration::addTexture(State *texState)
{
  TextureState *tex = (TextureState*) texState;
  map<string,State*>::iterator needle = textures_.find(tex->texture()->name());
  if(needle == textures_.end()) {
    textures_[tex->texture()->name()] = tex;
  }
}
set<State*> ShaderConfiguration::textures() const
{
  set<State*> ret;
  for(map<string,State*>::const_iterator
      it=textures_.begin(); it!=textures_.end(); ++it)
  {
    ret.insert(it->second);
  }
  return ret;
}

void ShaderConfiguration::setAttribute(VertexAttribute *att)
{
  map<string,VertexAttribute*>::iterator needle = attributes_.find(att->name());
  if(needle == attributes_.end()) {
    attributes_[att->name()] = att;
  }
}
set<VertexAttribute*> ShaderConfiguration::attributes() const
{
  set<VertexAttribute*> ret;
  for(map<string,VertexAttribute*>::const_iterator
      it=attributes_.begin(); it!=attributes_.end(); ++it)
  {
    ret.insert(it->second);
  }
  return ret;
}

void ShaderConfiguration::setTransformFeedbackAttribute(VertexAttribute *att)
{
  map<string,VertexAttribute*>::iterator needle = transformFeedbackAttributes_.find(att->name());
  if(needle == transformFeedbackAttributes_.end()) {
    transformFeedbackAttributes_[att->name()] = att;
  }
}
set<VertexAttribute*> ShaderConfiguration::transformFeedbackAttributes() const
{
  set<VertexAttribute*> ret;
  for(map<string,VertexAttribute*>::const_iterator
      it=transformFeedbackAttributes_.begin(); it!=transformFeedbackAttributes_.end(); ++it)
  {
    ret.insert(it->second);
  }
  return ret;
}

void ShaderConfiguration::setFragmentOutputs(
    list< ref_ptr<ShaderFragmentOutput> > &fragmentOutputs)
{
  if(fragmentOutputs_.empty()) {
    for(list< ref_ptr<ShaderFragmentOutput> >::iterator
        it=fragmentOutputs.begin(); it!=fragmentOutputs.end(); ++it)
    {
      fragmentOutputs_.insert(it->get());
    }
  }
}
set<ShaderFragmentOutput*> ShaderConfiguration::fragmentOutputs() const
{
  return fragmentOutputs_;
}
