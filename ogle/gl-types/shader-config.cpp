/*
 * shader-configuration.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "shader-config.h"

#include <ogle/states/material-state.h>
#include <ogle/states/texture-state.h>
#include <ogle/utility/string-util.h>
#include <ogle/states/texture-state.h>
#include <ogle/gl-types/shader-input.h>

ShaderConfig::ShaderConfig()
: material_(NULL),
  tessCfg_(TESS_PRIMITVE_TRIANGLES, 0)
{
  defines_["GLSL_VERSION"] = "150";
}

const map<string,string>& ShaderConfig::defines() const
{
  return defines_;
}
void ShaderConfig::define(const string &name, const string &value)
{
  defines_[name] = value;
}

void ShaderConfig::setVersion(const string &version)
{
  defines_["GLSL_VERSION"] = version;
}

void ShaderConfig::setUseFog(GLboolean toggle)
{
  defines_["HAS_FOG"] = (toggle==GL_TRUE ? "TRUE" : "FALSE");
}

void ShaderConfig::setIgnoreCameraRotation(GLboolean toggle)
{
  defines_["IGNORE_VIEW_ROTATION"] = (toggle==GL_TRUE ? "TRUE" : "FALSE");
}
void ShaderConfig::setIgnoreCameraTranslation(GLboolean toggle)
{
  defines_["IGNORE_VIEW_TRANSLATION"] = (toggle==GL_TRUE ? "TRUE" : "FALSE");
}

void ShaderConfig::setTesselationCfg(const Tesselation &tess)
{
  tessCfg_ = tess;
  defines_["HAS_TESSELATION"] = "TRUE";
  defines_["GLSL_VERSION"] = "400";
  switch(tess.lodMetric) {
  case TESS_LOD_EDGE_SCREEN_DISTANCE:
    defines_["TESS_LOD"] = "EDGE_SCREEN_DISTANCE";
    break;
  case TESS_LOD_EDGE_DEVICE_DISTANCE:
    defines_["TESS_LOD"] = "EDGE_DEVICE_DISTANCE";
    break;
  case TESS_LOD_CAMERA_DISTANCE_INVERSE:
    defines_["TESS_LOD"] = "CAMERA_DISTANCE_INVERSE";
    break;
  }
  switch(tess.spacing) {
  case TESS_SPACING_EQUAL:
    defines_["TESS_SPACING"] = "equal_spacing";
    break;
  case TESS_SPACING_FRACTIONAL_EVEN:
    defines_["TESS_SPACING"] = "fractional_even_spacing";
    break;
  case TESS_SPACING_FRACTIONAL_ODD:
    defines_["TESS_SPACING"] = "fractional_odd_spacing";
    break;
  }
  switch(tess.ordering) {
  case TESS_ORDERING_CCW:
    defines_["TESS_ORDERING"] = "ccw";
    break;
  case TESS_ORDERING_CW:
    defines_["TESS_ORDERING"] = "cw";
    break;
  case TESS_ORDERING_POINT_MODE:
    defines_["TESS_ORDERING"] = "point_mode";
    break;
  }
  switch(tess.primitive) {
  case TESS_PRIMITVE_TRIANGLES:
    defines_["TESS_PRIMITVE"] = "triangles";
    break;
  case TESS_PRIMITVE_QUADS:
    defines_["TESS_PRIMITVE"] = "quads";
    break;
  case TESS_PRIMITVE_ISOLINES:
    defines_["TESS_PRIMITVE"] = "isolines";
    break;
  }
  defines_["TESS_NUM_VERTICES"] = FORMAT_STRING(tess.numPatchVertices);
}
const Tesselation& ShaderConfig::tessCfg() const
{
  return tessCfg_;
}

void ShaderConfig::setNumBoneWeights(GLuint numBoneWeights)
{
  if(numBoneWeights>0) {
    defines_["HAS_BONES"] = "TRUE";
    defines_["NUM_BONE_WEIGTHS"] = FORMAT_STRING(numBoneWeights);
  } else {
    defines_["HAS_BONES"] = "FALSE";
  }
}

void ShaderConfig::setMaterial(State *material)
{
  if(material_!=NULL) { return; }

  material_ = material;
  if(material_!=NULL) {
    const Material *mat = (Material*)material_;
    defines_["HAS_MATERIAL"] = "TRUE";
    switch(mat->shading()) {
    case Material::GOURAD_SHADING:
      defines_["SHADING"] = "GOURAD";
      break;
    case Material::PHONG_SHADING:
      defines_["SHADING"] = "PHONG";
      break;
    case Material::BLINN_SHADING:
      defines_["SHADING"] = "BLINN";
      break;
    case Material::TOON_SHADING:
      defines_["SHADING"] = "TOON";
      break;
    case Material::ORENNAYER_SHADING:
      defines_["SHADING"] = "ORENNAYER";
      break;
    case Material::MINNAERT_SHADING:
      defines_["SHADING"] = "MINNAERT";
      break;
    case Material::COOKTORRANCE_SHADING:
      defines_["SHADING"] = "COOKTORRANCE";
      break;
    case Material::NO_SHADING:
      defines_["SHADING"] = "NONE";
      break;
    }
    if(mat->twoSided()) {
      defines_["HAS_TWO_SIDES"] = "TRUE";
    }
    if(mat->useAlpha()) {
      defines_["HAS_ALPHA"] = "TRUE";
    }
  }
}
const State* ShaderConfig::material() const
{
  return material_;
}

void ShaderConfig::addLight(State *light)
{
  lights_.insert(light);
  defines_["HAS_LIGHT"] = "TRUE";
  defines_["NUM_LIGHTS"] = FORMAT_STRING(lights_.size());
}
const set<State*>& ShaderConfig::lights() const
{
  return lights_;
}

void ShaderConfig::addTexture(State *state)
{
  TextureState *texState = (TextureState*) state;
  Texture *tex = texState->texture().get();
  map<string,State*>::iterator needle = textures_.find(tex->name());
  if(needle == textures_.end()) {
    textures_[tex->name()] = texState;
    if(texState->useAlpha() && !texState->ignoreAlpha()) {
      defines_["HAS_ALPHA"] = "TRUE";
    }
  }
}
const map<string,State*>& ShaderConfig::textures() const
{
  return textures_;
}

void ShaderConfig::setShaderInput(ref_ptr<ShaderInput> &input)
{
  map<string, ref_ptr<ShaderInput> >::iterator needle = inputs_.find(input->name());
  if(needle == inputs_.end()) {
    inputs_[input->name()] = input;

    if(input->name() == "nor") {
      defines_["HAS_NORMAL"] = "TRUE";
    }
    else if(input->name() == "col") {
      defines_["HAS_COLOR"] = "TRUE";
    }
    else if(input->name() == "tan") {
      defines_["HAS_TANGENT"] = "TRUE";
    }
    else if(input->name() == "modelMatrix") {
      defines_["HAS_MODELMAT"] = "TRUE";
    }
    else if(input->name() == "viewMatrix") {
      defines_["HAS_VIEW_MAT"] = "TRUE";
    }
    else if(input->name() == "projectionMatrix") {
      defines_["HAS_PROJECTION_MAT"] = "TRUE";
    }
    else if(input->numInstances()>1) {
      defines_["HAS_INSTANCES"] = "TRUE";
    }
  }
}
const map< string, ref_ptr<ShaderInput> >& ShaderConfig::inputs() const
{
  return inputs_;
}

void ShaderConfig::setTransformFeedbackAttributes(list< ref_ptr<VertexAttribute> > &atts)
{
  if(!transformFeedbackAttributes_.empty()) { return; }
  transformFeedbackAttributes_ = atts;
}
const list< ref_ptr<VertexAttribute> >& ShaderConfig::transformFeedbackAttributes() const
{
  return transformFeedbackAttributes_;
}

void ShaderConfig::setOutputs(list<ShaderOutput> &outputs)
{
  if(!outputs_.empty()) { return; }
  outputs_ = outputs;
}
const list<ShaderOutput>& ShaderConfig::outputs() const
{
  return outputs_;
}
