/*
 * shader-configuration.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "shader-config.h"

#include <ogle/states/material-state.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/light-state.h>
#include <ogle/utility/string-util.h>
#include <ogle/states/texture-state.h>
#include <ogle/gl-types/shader-input.h>

ShaderConfig::ShaderConfig()
: material_(NULL),
  tessCfg_(TESS_PRIMITVE_TRIANGLES, 0)
{
  defines_["GLSL_VERSION"] = "330";
}

const map<string,string>& ShaderConfig::defines() const
{
  return defines_;
}
void ShaderConfig::define(const string &name, const string &value)
{
  defines_[name] = value;
}

const map<string, string>& ShaderConfig::functions() const
{
  return functions_;
}
void ShaderConfig::defineFunction(const string &name, const string &value)
{
  functions_[name] = value;
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

void ShaderConfig::setTesselationCfg(const TesselationConfig &tess)
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
  default:
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
const TesselationConfig& ShaderConfig::tessCfg() const
{
  return tessCfg_;
}

void ShaderConfig::setBones(GLuint numBoneWeights, GLuint numBones)
{
  if(numBoneWeights>0) {
    defines_["HAS_BONES"] = "TRUE";
  }
  else {
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
    case Material::DEFERRED_PHONG_SHADING:
      defines_["SHADING"] = "DEFERRED_PHONG";
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

void ShaderConfig::addLight(State *lightState)
{

  Light *light = (Light*)lightState;

  string lightName = FORMAT_STRING("LIGHT" << lights_.size());
  string lightType;
  if(dynamic_cast<SpotLight*>(light)!=NULL) {
    lightType = "SPOT";
  } else if(dynamic_cast<PointLight*>(light)!=NULL) {
    lightType = "POINT";
  } else {
    lightType = "DIRECTIONAL";
  }
  defines_[FORMAT_STRING(lightName << "_TYPE")] = lightType;
  defines_[FORMAT_STRING(lightName << "_ID")] = FORMAT_STRING(light->id());

  defines_["HAS_LIGHT"] = "TRUE";
  defines_["NUM_LIGHTS"] = FORMAT_STRING(lights_.size()+1);

  lights_.push_back(lightState);
}
list<State*>& ShaderConfig::lights()
{
  return lights_;
}

void ShaderConfig::addTexture(State *state)
{
  TextureState *texState = (TextureState*) state;

  string texName = FORMAT_STRING("TEXTURE" << textures_.size());

  defines_[FORMAT_STRING(texName << "_NAME")] =
      FORMAT_STRING(texState->name());
  defines_[FORMAT_STRING(texName << "_SAMPLER_TYPE")] =
      FORMAT_STRING(texState->samplerType());
  defines_[FORMAT_STRING(texName << "_DIM")] =
      FORMAT_STRING(texState->dimension());
  defines_[FORMAT_STRING(texName << "_MAPTO")] =
      FORMAT_STRING(texState->mapTo());

  defines_[FORMAT_STRING(texName << "_BLEND_FACTOR")] =
      FORMAT_STRING(texState->blendFactor());
  if(!texState->blendFunction().empty()) {
    functions_[texState->blendName()] = texState->blendFunction();
    defines_[FORMAT_STRING(texName << "_BLEND_KEY")] = texState->blendName();
    defines_[FORMAT_STRING(texName << "_BLEND_NAME")] = texState->blendName();
  } else {
    defines_[FORMAT_STRING(texName << "_BLEND_KEY")] =
        FORMAT_STRING("blending." << texState->blendMode());
    defines_[FORMAT_STRING(texName << "_BLEND_NAME")] =
        FORMAT_STRING("blend_" << texState->blendMode());
  }

  if(!texState->transferKey().empty()) {
    defines_[FORMAT_STRING(texName << "_TRANSFER_KEY")] = texState->transferKey();
    defines_[FORMAT_STRING(texName << "_TRANSFER_NAME")] = texState->transferName();
  }
  if(!texState->transferFunction().empty()) {
    functions_[texState->transferName()] = texState->transferFunction();
    defines_[FORMAT_STRING(texName << "_TRANSFER_KEY")] = texState->transferName();
    defines_[FORMAT_STRING(texName << "_TRANSFER_NAME")] = texState->transferName();
  }

  if(!texState->mappingFunction().empty()) {
    functions_[texState->mappingName()] = texState->mappingFunction();
    defines_[FORMAT_STRING(texName << "_MAPPING_KEY")] = texState->mappingName();
    defines_[FORMAT_STRING(texName << "_MAPPING_NAME")] = texState->mappingName();
  } else {
    defines_[FORMAT_STRING(texName << "_MAPPING_KEY")] =
        FORMAT_STRING("textures.texco_" << texState->mapping());
    defines_[FORMAT_STRING(texName << "_MAPPING_NAME")] =
        FORMAT_STRING("texco_" << texState->mapping());
  }
  if(texState->mapping()==MAPPING_TEXCO) {
    string texcoName = FORMAT_STRING("texco" << texState->texcoChannel());
    defines_[FORMAT_STRING(texName << "_TEXCO")] = texcoName;
  }

  defines_["NUM_TEXTURES"] = FORMAT_STRING(textures_.size()+1);

  if(texState->useAlpha() && !texState->ignoreAlpha()) {
    defines_["HAS_ALPHA"] = "TRUE";
  }

  textures_.push_back(state);
}
list<State*>& ShaderConfig::textures()
{
  return textures_;
}

void ShaderConfig::setShaderInput(const ref_ptr<ShaderInput> &input)
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

void ShaderConfig::setTransformFeedbackAttributes(list< ref_ptr<VertexAttribute> > &atts,
    GLenum attributeLayout)
{
  if(!transformFeedbackAttributes_.empty()) { return; }
  transformFeedbackAttributes_ = atts;
  transformFeedbackMode_ = attributeLayout;
}
const list< ref_ptr<VertexAttribute> >& ShaderConfig::transformFeedbackAttributes() const
{
  return transformFeedbackAttributes_;
}
GLenum ShaderConfig::transformFeedbackMode() const
{
  return transformFeedbackMode_;
}
