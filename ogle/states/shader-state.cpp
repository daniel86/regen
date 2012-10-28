/*
 * shader-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>

#include "shader-state.h"
#include <ogle/utility/string-util.h>
#include <ogle/utility/gl-error.h>
#include <ogle/states/render-state.h>
#include <ogle/states/light-state.h>
#include <ogle/states/material-state.h>
#include <ogle/states/texture-state.h>

ShaderState::ShaderState(ref_ptr<Shader> shader)
: State(),
  shader_(shader)
{
}

ShaderState::ShaderState()
: State()
{
}

string ShaderState::shadePropertiesCode(ShaderConfig &cfg)
{
  stringstream ss;
  GLint count=0;
  list<State*> &lights = cfg.lights();
  if(lights.empty()) { return ""; }

  ss << "void shadeProperties(inout LightProperties props, vec4 posWorld) {" << endl;
  for(list<State*>::iterator it=lights.begin(); it!=lights.end(); ++it) {
    Light *light = (Light*)(*it);
    switch(light->getLightType()) {
    case Light::DIRECTIONAL:
      ss << "    shadeDirectionalProperties(props,posWorld.xyz," <<
      count << "," << light->id() << ");" << endl;
      break;
    case Light::POINT:
      ss << "    shadePointProperties(props,posWorld.xyz," <<
      count << "," << light->id() << ");" << endl;
      break;
    case Light::SPOT:
      ss << "    shadeSpotProperties(props,posWorld.xyz," <<
      count << "," << light->id() << ");" << endl;
      break;
    }
    ++count;
  }
  ss << "}" << endl;
  return ss.str();
}

string ShaderState::shadeCode(ShaderConfig &cfg)
{
  stringstream ss;
  GLint count=0;
  list<State*> &lights = cfg.lights();
  if(lights.empty()) { return ""; }

  ss << "void shade(LightProperties props, inout Shading shading, vec3 posWorld, vec3 norWorld) {" << endl;
  for(list<State*>::iterator it=lights.begin(); it!=lights.end(); ++it) {
    Light *light = (Light*)(*it);
    switch(light->getLightType()) {
    case Light::DIRECTIONAL:
      ss << "    shadeDirectionalLight(props,shading,posWorld,norWorld," <<
                        count << "," << light->id() << ");" << endl;
      break;
    case Light::POINT:
      ss << "    shadePointLight(props,shading,posWorld,norWorld," <<
                        count << "," << light->id() << ");" << endl;
      break;
    case Light::SPOT:
      ss << "    shadeSpotLight(props,shading,posWorld,norWorld," <<
                        count << "," << light->id() << ");" << endl;
      break;
    }
    ++count;
  }
  ss << "}" << endl << endl;
  return ss.str();
}

string ShaderState::texelCode(const TextureState *texState)
{
  string texcoName;

  TextureMapping mapping = texState->mapping();
  switch(mapping) {
  case MAPPING_TEXCO:
    texcoName = FORMAT_STRING("texco" << texState->texcoChannel());
    break;
  default:
    texcoName = FORMAT_STRING("texco_"<<mapping<<"()");
    break;
  }

  string texelLookup = FORMAT_STRING(
      "SAMPLE("<< texState->textureName() << ", " << texcoName << ")");
  if(isApprox(texState->texelFactor(),1.0f)) {
    // TODO: texel factor uniform ?
    texelLookup = FORMAT_STRING(texState->texelFactor() << "*" << texelLookup);
  }
  if(texState->invert()) {
    texelLookup = FORMAT_STRING("vec4(1.0) - " << texelLookup);
  }
  if(!texState->transferKey().empty()) {
    // use a transfer function
    list<string> path;
    boost::split(path, texState->transferKey(), boost::is_any_of("."));
    string &transferName = path.back();
    texelLookup = FORMAT_STRING(transferName << "(" << texelLookup << ")");
  }
  return texelLookup;
}

string ShaderState::blendCode(const TextureState *texState, const string &dst, const string &src)
{
  // TODO: blend factor uniform ?
  if(texState->blendMode() == BLEND_MODE_SRC) {
    return dst + " = " + src;
  } else {
    return FORMAT_STRING("blend_"<<texState->blendMode()<<
        "("<<src<<","<<dst<<","<<texState->blendFactor()<<")");
  }
}

string ShaderState::modifyTransformationCode(ShaderConfig &cfg)
{
  stringstream ss;
  list<State*> &textures = cfg.textures();
  GLint count=0;

  ss << "void modifyTransformation(inout vec4 posWorld, inout vec3 norWorld) {" << endl;
  ss << "    vec4 texel;" << endl;
  for(list<State*>::const_iterator it=textures.begin(); it!=textures.end(); ++it) {
    const TextureState *texState = (TextureState*)(*it);

    if(!texState->mapTo(MAP_TO_HEIGHT)&&
       !texState->mapTo(MAP_TO_DISPLACEMENT)) { continue; }

    ss << "    texel = " << texelCode(texState) << ";" << endl;
    if(texState->ignoreAlpha()) {
      ss << "    texel.a = 1.0;" << endl;
    }

    if(texState->mapTo(MAP_TO_HEIGHT)) {
      ss << "    " << blendCode(texState, "posWorld.xyz", "norWorld*texel.x") << ";" << endl;
    }
    if(texState->mapTo(MAP_TO_DISPLACEMENT)) {
      ss << "    " << blendCode(texState, "posWorld.xyz", "texel.xyz") << ";" << endl;
    }
    ss << endl;

    ++count;
  }
  ss << "}" << endl << endl;

  if(count==0) {
    return "#define modifyTransformation(p,n)\n";
  } else {
    return ss.str();
  }
}

string ShaderState::modifyLightCode(ShaderConfig &cfg)
{
  stringstream ss;
  list<State*> &textures = cfg.textures();
  GLint count=0;

  ss << "void modifyLight(inout Shading shading) {" << endl;
  ss << "    vec4 texel;" << endl;
  for(list<State*>::const_iterator it=textures.begin(); it!=textures.end(); ++it) {
    const TextureState *texState = (TextureState*)(*it);

    if(!texState->mapTo(MAP_TO_DIFFUSE)&&
       !texState->mapTo(MAP_TO_AMBIENT)&&
       !texState->mapTo(MAP_TO_SPECULAR)&&
       !texState->mapTo(MAP_TO_SHININESS)&&
       !texState->mapTo(MAP_TO_LIGHT)) { continue; }

    ss << "    texel = " << texelCode(texState) << ";" << endl;
    if(texState->ignoreAlpha()) {
      ss << "    texel.a = 1.0;" << endl;
    }

    if(texState->mapTo(MAP_TO_DIFFUSE) || texState->mapTo(MAP_TO_LIGHT)) {
      ss << "    " << blendCode(texState, "shading.diffuse", "texel.rgba") << ";" << endl;
    }
    if(texState->mapTo(MAP_TO_AMBIENT) || texState->mapTo(MAP_TO_LIGHT)) {
      ss << "    " << blendCode(texState, "shading.ambient", "texel.rgba") << ";" << endl;
    }
    if(texState->mapTo(MAP_TO_SPECULAR)) {
      ss << "    " << blendCode(texState, "shading.specular", "texel.rgba") << ";" << endl;
    }
    if(texState->mapTo(MAP_TO_SHININESS)) {
      ss << "    " << blendCode(texState, "shading.shininess", "texel.rgba") << ";" << endl;
    }
    ss << endl;

    ++count;
  }
  ss << "}" << endl << endl;

  if(count==0) {
    return "#define modifyLight(s)\n";
  } else {
    return ss.str();
  }
}

string ShaderState::modifyColorCode(ShaderConfig &cfg)
{
  stringstream ss;
  list<State*> &textures = cfg.textures();
  GLint count=0;

  ss << "void modifyColor(inout vec4 color) {" << endl;
  ss << "    vec4 texel;" << endl;
  for(list<State*>::const_iterator it=textures.begin(); it!=textures.end(); ++it) {
    const TextureState *texState = (TextureState*)(*it);

    if(!texState->mapTo(MAP_TO_COLOR)) { continue; }

    ss << "    texel = " << texelCode(texState) << ";" << endl;
    if(texState->ignoreAlpha()) {
      ss << "    texel.a = 1.0;" << endl;
    }

    if(texState->mapTo(MAP_TO_COLOR)) {
      if(texState->ignoreAlpha()) {
        ss << "    " << blendCode(texState, "color.rgb", "texel.rgb") << ";" << endl;
      } else {
        ss << "    " << blendCode(texState, "color.rgba", "texel.rgba") << ";" << endl;
      }
    }
    ss << endl;

    ++count;
  }
  ss << "}" << endl << endl;

  if(count==0) {
    return "#define modifyColor(s)\n";
  } else {
    return ss.str();
  }
}

string ShaderState::modifyAlphaCode(ShaderConfig &cfg)
{
  stringstream ss;
  list<State*> &textures = cfg.textures();
  GLint count=0;

  ss << "void modifyAlpha(inout float alpha) {" << endl;
  ss << "    vec4 texel;" << endl;
  for(list<State*>::const_iterator it=textures.begin(); it!=textures.end(); ++it) {
    const TextureState *texState = (TextureState*)(*it);

    if(!texState->mapTo(MAP_TO_ALPHA)) { continue; }

    ss << "    texel = " << texelCode(texState) << ";" << endl;
    ss << "    " << blendCode(texState, "alpha", "texel.a") << ";" << endl;
    ss << endl;

    ++count;
  }
  ss << "}" << endl << endl;

  if(count==0) {
    return "#define modifyAlpha(s)\n";
  } else {
    return ss.str();
  }
}

string ShaderState::modifyNormalCode(ShaderConfig &cfg)
{
  stringstream ss;
  list<State*> &textures = cfg.textures();
  GLint count=0;

  ss << "void modifyNormal(inout vec3 nor) {" << endl;
  ss << "    vec4 texel;" << endl;
  for(list<State*>::const_iterator it=textures.begin(); it!=textures.end(); ++it) {
    const TextureState *texState = (TextureState*)(*it);

    if(!texState->mapTo(MAP_TO_NORMAL)) { continue; }

    ss << "    texel = " << texelCode(texState) << ";" << endl;
    ss << "    " << blendCode(texState, "nor", "texel.xyz") << ";" << endl;
    ss << endl;

    ++count;
  }
  ss << "}" << endl << endl;

  if(count==0) {
    return "#define modifyNormal(s)\n";
  } else {
    return ss.str();
  }
}

GLboolean ShaderState::createShader(
    ShaderConfig &cfg,
    const string &effectName)
{
  map<GLenum,string> code;
  return createShader(cfg, effectName, code);
}

GLboolean ShaderState::createShader(
    ShaderConfig &cfg,
    const string &effectName,
    map<GLenum,string> &code)
{
  const map<string, ref_ptr<ShaderInput> > specifiedInput = cfg.inputs();
  const map<string, string> &shaderConfig = cfg.defines();
  list<State*> &lights = cfg.lights();
  list<State*> &textures = cfg.textures();
  map<GLenum, set<string> > includes;

  Material::Shading shading = Material::GOURAD_SHADING;
  if(cfg.material()!=NULL) {
    shading = ((Material*)cfg.material())->shading();
  }

  GLboolean hasTesselation = GL_FALSE;
  if(shaderConfig.count("HAS_TESSELATION")>0) {
    hasTesselation = (shaderConfig.find("HAS_TESSELATION")->second == "TRUE");
  }

  set<GLenum> usedStages;
  usedStages.insert(GL_VERTEX_SHADER);
  //usedStages.push_back(GL_GEOMETRY_SHADER);
  usedStages.insert(GL_FRAGMENT_SHADER);
  if(hasTesselation) {
    usedStages.insert(GL_TESS_EVALUATION_SHADER);
    if(cfg.tessCfg().isAdaptive) {
      usedStages.insert(GL_TESS_CONTROL_SHADER);
    }
  }

  // load effect headers
  for(set<GLenum>::iterator it=usedStages.begin(); it!=usedStages.end(); ++it)
  {
    code[*it] = FORMAT_STRING(
        "#include " << effectName << "." <<
        Shader::stagePrefix(*it) << ".header" << endl << endl << code[*it]);
  }

  // ... add light uniforms
  stringstream uniforms;
  for(list<State*>::iterator it=lights.begin(); it!=lights.end(); ++it) {
    Light *light = (Light*)(*it);
    // TODO: automatically look for uniforms...
    uniforms << "uniform vec4 in_lightPosition" << light->id() << ";" << endl;
    uniforms << "uniform vec4 in_lightAmbient" << light->id() << ";" << endl;
    uniforms << "uniform vec4 in_lightDiffuse" << light->id() << ";" << endl;
    uniforms << "uniform vec4 in_lightSpecular" << light->id() << ";" << endl;
    uniforms << "uniform float in_lightInnerConeAngle" << light->id() << ";" << endl;
    uniforms << "uniform float in_lightOuterConeAngle" << light->id() << ";" << endl;
    uniforms << "uniform vec3 in_lightSpotDirection" << light->id() << ";" << endl;
    uniforms << "uniform float in_lightSpotExponent" << light->id() << ";" << endl;
    uniforms << "uniform float in_lightConstantAttenuation" << light->id() << ";" << endl;
    uniforms << "uniform float in_lightLinearAttenuation" << light->id() << ";" << endl;
    uniforms << "uniform float in_lightQuadricAttenuation" << light->id() << ";" << endl;
  }

  // TODO: do not add to all
  set<string> texco, texturesSet;
  for(list<State*>::const_iterator it=textures.begin(); it!=textures.end(); ++it) {
    const TextureState *texState = (TextureState*)(*it);
    cout << "        " << texState->textureName() << endl;

    texturesSet.insert(FORMAT_STRING("uniform " <<
        texState->samplerType() << " " << texState->textureName() << ";"));

    string texcoName = FORMAT_STRING("texco" << texState->texcoChannel());
    map<string, ref_ptr<ShaderInput> >::const_iterator needle = specifiedInput.find(texcoName);

    TextureMapping mapping = texState->mapping();
    switch(mapping) {
    case MAPPING_TEXCO:
      if(needle!=specifiedInput.end()) {
        string texcoType = "vec2";
        if(needle->second->valsPerElement()==1) { texcoType = "float"; }
        else if(needle->second->valsPerElement()==3) { texcoType = "vec3"; }
        else if(needle->second->valsPerElement()==4) { texcoType = "vec4"; }
        texco.insert(FORMAT_STRING(texcoType << " " << texcoName));
      } else {
        string texcoType = "vec2";
        if(texState->samplerType() == "sampler1D")
        {
          texcoType = "vec1";
        }
        else if(texState->samplerType() == "sampler3D" ||
            texState->samplerType() == "cubeSampler")
        {
          texcoType = "vec3";
        }
        texco.insert(FORMAT_STRING(texcoType << " " << texcoName));
      }
      break;
    default:
      break;
    }
  }
  code[GL_VERTEX_SHADER] = FORMAT_STRING(
      uniforms.str() << endl << code[GL_VERTEX_SHADER]);
  code[GL_FRAGMENT_SHADER] = FORMAT_STRING(
      uniforms.str() << endl << code[GL_FRAGMENT_SHADER]);
  if(usedStages.count(GL_TESS_EVALUATION_SHADER)>0)
  code[GL_TESS_EVALUATION_SHADER] = FORMAT_STRING(
      uniforms.str() << endl << code[GL_TESS_EVALUATION_SHADER]);
  for(set<string>::iterator it=texturesSet.begin(); it!=texturesSet.end(); ++it) {
    code[GL_VERTEX_SHADER] = FORMAT_STRING(
        *it << endl << code[GL_VERTEX_SHADER]);
    code[GL_FRAGMENT_SHADER] = FORMAT_STRING(
        *it << endl << code[GL_FRAGMENT_SHADER]);
    if(usedStages.count(GL_TESS_EVALUATION_SHADER)>0)
    code[GL_TESS_EVALUATION_SHADER] = FORMAT_STRING(
        *it << endl << code[GL_TESS_EVALUATION_SHADER]);
  }
  for(set<string>::iterator it=texco.begin(); it!=texco.end(); ++it) {
    code[GL_VERTEX_SHADER] = FORMAT_STRING(
        "in " << *it << ";" << endl << code[GL_VERTEX_SHADER]);
    code[GL_FRAGMENT_SHADER] = FORMAT_STRING(
        "in " << *it << ";" << endl << code[GL_FRAGMENT_SHADER]);
    if(usedStages.count(GL_TESS_EVALUATION_SHADER)>0)
    code[GL_TESS_EVALUATION_SHADER] = FORMAT_STRING(
        "in " << *it << "[TESS_NUM_VERTICES];" << endl << code[GL_TESS_EVALUATION_SHADER]);
  }

  // textures may require additional methods for blending and texel transfer
  for(list<State*>::const_iterator it=textures.begin(); it!=textures.end(); ++it) {
    const TextureState *texState = (TextureState*)(*it);

    // include blending functions
    if(texState->blendMode() != BLEND_MODE_SRC) {
      stringstream blendMode;
      blendMode << "#include blending." << texState->blendMode();
      if(texState->mapTo(MAP_TO_HEIGHT) || texState->mapTo(MAP_TO_DISPLACEMENT)) {
        includes[hasTesselation ? GL_TESS_EVALUATION_SHADER : GL_VERTEX_SHADER].insert(blendMode.str());
      }
      includes[GL_FRAGMENT_SHADER].insert(blendMode.str());
    }

    if(texState->mapping() != MAPPING_TEXCO) {
      stringstream texcoGen;
      texcoGen << "#include texco-gen.texco_" << texState->mapping();
      includes[GL_FRAGMENT_SHADER].insert(texcoGen.str());
    }

    // include transfer functions
    string transferFunction = texState->transferKey();
    if(!boost::contains(transferFunction, "\n") && boost::contains(transferFunction, ".")) {
      transferFunction = "#include " + transferFunction;
      if(texState->mapTo(MAP_TO_HEIGHT) || texState->mapTo(MAP_TO_DISPLACEMENT)) {
        includes[hasTesselation ? GL_TESS_EVALUATION_SHADER : GL_VERTEX_SHADER].insert(transferFunction);
      }
      includes[GL_FRAGMENT_SHADER].insert(transferFunction);
    } else {
      if(texState->mapTo(MAP_TO_HEIGHT) || texState->mapTo(MAP_TO_DISPLACEMENT)) {
        code[hasTesselation ? GL_TESS_EVALUATION_SHADER : GL_VERTEX_SHADER] += transferFunction + "\n";
      }
      code[GL_FRAGMENT_SHADER] += transferFunction + "\n";
    }
  }

  // include some dependencies
  for(map<GLenum, set<string> >::iterator
      it=includes.begin(); it!=includes.end(); ++it)
  {
    for(set<string>::iterator jt=it->second.begin(); jt!=it->second.end(); ++jt)
    {
      code[it->first] += (*jt) + "\n\n";
    }
  }

  { // VS
    if(!cfg.lights().empty() && shading!=Material::NO_SHADING) {
      code[GL_VERTEX_SHADER] += shadePropertiesCode(cfg);
    }
    if(shading == Material::GOURAD_SHADING) {
      code[GL_VERTEX_SHADER] += shadeCode(cfg);
    }
    if(!hasTesselation) {
      code[GL_VERTEX_SHADER] += modifyTransformationCode(cfg);
    }
  }

  { // TCS
  }

  { // TES
    if(hasTesselation) {
      code[GL_TESS_EVALUATION_SHADER] += modifyTransformationCode(cfg);
    }
  }

  { // FS
    if(shading != Material::GOURAD_SHADING && shading != Material::NO_SHADING) {
      code[GL_FRAGMENT_SHADER] += shadeCode(cfg);
    }
    code[GL_FRAGMENT_SHADER] += modifyLightCode(cfg);
    code[GL_FRAGMENT_SHADER] += modifyColorCode(cfg);
    code[GL_FRAGMENT_SHADER] += modifyAlphaCode(cfg);
    code[GL_FRAGMENT_SHADER] += modifyNormalCode(cfg);
  }

  // load main functions
  for(set<GLenum>::iterator it=usedStages.begin(); it!=usedStages.end(); ++it)
  {
    code[*it] = FORMAT_STRING(
        code[*it] << endl <<
        "#include " << effectName << "." << Shader::stagePrefix(*it) << ".main" << endl);
  }

  ref_ptr<Shader> shader = Shader::create(shaderConfig, specifiedInput, code);

  // setup shader outputs
  const list<ShaderOutput> &outputs = cfg.outputs();
  shader->setOutputs(outputs);

  // setup transform feedback attributes
  const list< ref_ptr<VertexAttribute> > &tranformFeedbackAtts = cfg.transformFeedbackAttributes();
  list<string> transformFeedback;
  for(list< ref_ptr<VertexAttribute> >::const_iterator
      it=tranformFeedbackAtts.begin(); it!=tranformFeedbackAtts.end(); ++it)
  {
    transformFeedback.push_back((*it)->name());
  }
  shader->setTransformFeedback(transformFeedback, GL_SEPARATE_ATTRIBS);

  if(!shader->compile()) { return GL_FALSE; }

  if(!shader->link()) { return GL_FALSE; }

  shader->setInputs(specifiedInput);

  shader_ = shader;

  return GL_TRUE;
}

GLboolean ShaderState::createSimple(
    map<string, string> &shaderConfig,
    map<GLenum, string> &shaderNames)
{
  shader_ = Shader::create(shaderConfig, shaderNames);
  return shader_.get() != NULL;
}

string ShaderState::name()
{
  return FORMAT_STRING("ShaderState(" << shader_->id() << ")");
}

void ShaderState::enable(RenderState *state)
{
  state->pushShader(shader_.get());
  State::enable(state);
}

void ShaderState::disable(RenderState *state)
{
  State::disable(state);
  state->popShader();
}

ref_ptr<Shader> ShaderState::shader()
{
  return shader_;
}
void ShaderState::set_shader(ref_ptr<Shader> shader)
{
  shader_ = shader;
}
