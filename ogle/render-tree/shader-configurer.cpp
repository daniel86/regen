/*
 * shader-configurer.cpp
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#include "shader-configurer.h"
#include <ogle/states/shader-input-state.h>
#include <ogle/states/light-state.h>
#include <ogle/states/mesh-state.h>
#include <ogle/utility/string-util.h>

ShaderConfig ShaderConfigurer::configure(const StateNode *node)
{
  ShaderConfigurer configurer;
  configurer.addNode(node);
  return configurer.cfg_;
}
ShaderConfig ShaderConfigurer::configure(const State *state)
{
  ShaderConfigurer configurer;
  configurer.addState(state);
  return configurer.cfg_;
}

/////////////
/////////////

ShaderConfigurer::ShaderConfigurer()
{
  define("GLSL_VERSION","330");
}

void ShaderConfigurer::addNode(const StateNode *node)
{
  if(node->hasParent()) {
    addNode(node->parent());
  }
  addState(node->state().get());
}

void ShaderConfigurer::addState(const State *s)
{
  if(s->isHidden()) { return; }

  if(dynamic_cast<const ShaderInputState*>(s) != NULL)
  {
    // remember inputs, they will be enabled automatically
    // when the shader is enabled.
    const ShaderInputState *sis = (const ShaderInputState*)s;
    for(list< ref_ptr<ShaderInput> >::const_iterator
        it=sis->inputs().begin(); it!=sis->inputs().end(); ++it)
    {
      const ref_ptr<ShaderInput> &in = *it;
      // only use unknown inputs. known inputs are already defined by children.
      map<string, ref_ptr<ShaderInput> >::iterator needle = cfg_.inputs_.find(in->name());
      if(needle != cfg_.inputs_.end()) continue;

      cfg_.inputs_[in->name()] = in;

      if(in->numInstances()>1)
      {
        define("HAS_INSTANCES", "TRUE");
      }
    }

    if(dynamic_cast<const MeshState*>(s) != NULL)
    {
      const MeshState *m = (const MeshState*)s;
      // XXX: what if multiple meshes in the tree ?
      if(cfg_.transformFeedbackAttributes_.empty()) {
        cfg_.transformFeedbackAttributes_ = m->tfAttributes();
        cfg_.transformFeedbackMode_ = GL_SEPARATE_ATTRIBS;
      }
    }
  }
  else if(dynamic_cast<const Light*>(s) != NULL)
  {
    const Light *lightState = (const Light*)s;
    define(
        FORMAT_STRING("LIGHT" << cfg_.lights_.size() << "_ID"),
        FORMAT_STRING(lightState->id()));
    define("NUM_LIGHTS", FORMAT_STRING(cfg_.lights_.size()+1));
    cfg_.lights_.push_back(lightState);
  }
  else if(dynamic_cast<const TextureState*>(s) != NULL)
  {
    const TextureState *t = (const TextureState*)s;
#define __TEX_NAME(x) FORMAT_STRING(x << cfg_.textures_.size())

    define(__TEX_NAME("TEX_NAME"),         t->name());
    define(__TEX_NAME("TEX_SAMPLER_TYPE"), t->samplerType());
    define(__TEX_NAME("TEX_DIM"),          FORMAT_STRING(t->dimension()));
    define(__TEX_NAME("TEX_MAPTO"),        FORMAT_STRING(t->mapTo()));
    define(__TEX_NAME("TEX_BLEND_FACTOR"), FORMAT_STRING(t->blendFactor()));

    if(!t->blendFunction().empty()) {
      defineFunction(t->blendName(), t->blendFunction());
      define(__TEX_NAME("TEX_BLEND_KEY"), t->blendName());
      define(__TEX_NAME("TEX_BLEND_NAME"), t->blendName());
    }
    else {
      define(__TEX_NAME("TEX_BLEND_KEY"),  FORMAT_STRING("blending." << t->blendMode()));
      define(__TEX_NAME("TEX_BLEND_NAME"), FORMAT_STRING("blend_" << t->blendMode()));
    }

    if(!t->transferKey().empty()) {
      define(__TEX_NAME("TEX_TRANSFER_KEY"), t->transferKey());
      define(__TEX_NAME("TEX_TRANSFER_NAME"), t->transferName());
    }
    if(!t->transferFunction().empty()) {
      defineFunction(t->transferName(), t->transferFunction());
      define(__TEX_NAME("TEX_TRANSFER_KEY"), t->transferName());
      define(__TEX_NAME("TEX_TRANSFER_NAME"), t->transferName());
    }

    if(!t->mappingFunction().empty()) {
      defineFunction(t->mappingName(), t->mappingFunction());
      define(__TEX_NAME("TEX_MAPPING_KEY"), t->mappingName());
      define(__TEX_NAME("TEX_MAPPING_NAME"), t->mappingName());
    } else {
      define(__TEX_NAME("TEX_MAPPING_KEY"),  FORMAT_STRING("textures.texco_" << t->mapping()));
      define(__TEX_NAME("TEX_MAPPING_NAME"), FORMAT_STRING("texco_" << t->mapping()));
    }
    if(t->mapping()==MAPPING_TEXCO) {
      define(__TEX_NAME("TEX_TEXCO"), FORMAT_STRING("texco" << t->texcoChannel()));
    }

    define("NUM_TEXTURES", FORMAT_STRING(cfg_.textures_.size()+1));
    cfg_.textures_.push_back(t);

#undef __TEX_NAME
  }

  addDefines( s->shaderDefines() );

  for(list< ref_ptr<State> >::const_iterator
      it=s->joined().begin(); it!=s->joined().end(); ++it)
  {
    addState(it->get());
  }
}

void ShaderConfigurer::addDefines(const map<string,string> &defines)
{
  for(map<string,string>::const_iterator
      it=defines.begin(); it!=defines.end(); ++it)
  {
    define(it->first,it->second);
  }
}

void ShaderConfigurer::define(const string &name, const string &value)
{
  // XXX: special handling for some values. for example glsl version.
  cfg_.defines_[name] = value;
}
void ShaderConfigurer::defineFunction(const string &name, const string &value)
{
  cfg_.functions_[name] = value;
}
