/*
 * shader-configurer.cpp
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#include "shader-configurer.h"
#include <ogle/states/shader-input-state.h>
#include <ogle/states/light-state.h>
#include <ogle/meshes/mesh-state.h>
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

ShaderConfigurer::ShaderConfigurer(ShaderConfig &cfg)
: cfg_(cfg)
{
}

ShaderConfigurer::ShaderConfigurer()
: numLights_(0)
{
  // default is using seperate attributes.
  cfg_.feedbackMode_ = GL_SEPARATE_ATTRIBS;
  cfg_.feedbackStage_ = GL_VERTEX_SHADER;
  // sets the minimum version
  define("GLSL_VERSION","330");
  // initially no lights added
  define("NUM_LIGHTS", "0");
}

ShaderConfig& ShaderConfigurer::cfg() {
  return cfg_;
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
  if(dynamic_cast<const StateSequence*>(s) != NULL) { return; }

  if(dynamic_cast<const ShaderInputState*>(s) != NULL)
  {
    const ShaderInputState *sis = (const ShaderInputState*)s;

    // remember inputs, they will be enabled automatically
    // when the shader is enabled.
    for(list< ref_ptr<ShaderInput> >::const_iterator
        it=sis->inputs().begin(); it!=sis->inputs().end(); ++it)
    {
      const ref_ptr<ShaderInput> &in = *it;
      cfg_.inputs_[in->name()] = in;
    }

    // remember attribute names that should be recorded
    // by transform feedback
    if(dynamic_cast<const MeshState*>(s) != NULL)
    {
      const MeshState *m = (const MeshState*)s;
      cfg_.feedbackMode_ = m->feedbackMode();
      cfg_.feedbackStage_ = m->feedbackStage();
      for(list< ref_ptr<VertexAttribute> >::const_iterator
          it=m->feedbackAttributes().begin(); it!=m->feedbackAttributes().end(); ++it)
      {
        cfg_.feedbackAttributes_.push_back((*it)->name());
      }
    }
  }
  if(dynamic_cast<const TextureState*>(s) != NULL)
  {
    const TextureState *t = (const TextureState*)s;
    // map for loop index to texture id
    define(
        FORMAT_STRING("TEX_ID" << cfg_.textures_.size()),
        FORMAT_STRING(t->stateID()));
    // remember the number of textures used
    define("NUM_TEXTURES", FORMAT_STRING(cfg_.textures_.size()+1));
    cfg_.textures_.push_back(t);
  }

  addDefines( s->shaderDefines() );
  addFunctions( s->shaderFunctions() );

  for(list< ref_ptr<State> >::const_iterator
      it=s->joined().begin(); it!=s->joined().end(); ++it)
  {
    addState(it->get());
  }
}

void ShaderConfigurer::addDefines(const map<string,string> &defines)
{
  for(map<string,string>::const_iterator it=defines.begin(); it!=defines.end(); ++it)
    define(it->first,it->second);
}
void ShaderConfigurer::addFunctions(const map<string,string> &functions)
{
  for(map<string,string>::const_iterator it=functions.begin(); it!=functions.end(); ++it)
    defineFunction(it->first,it->second);
}

void ShaderConfigurer::define(const string &name, const string &value)
{
  cfg_.defines_[name] = value;
}
void ShaderConfigurer::defineFunction(const string &name, const string &value)
{
  cfg_.functions_[name] = value;
}
