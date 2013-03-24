/*
 * shader-configurer.cpp
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#include <regen/states/shader-input-state.h>
#include <regen/shading/light-state.h>
#include <regen/meshes/mesh-state.h>
#include <regen/utility/string-util.h>

#include "shader-configurer.h"
using namespace ogle;

ShaderState::Config ShaderConfigurer::configure(const StateNode *node)
{
  ShaderConfigurer configurer;
  configurer.addNode(node);
  return configurer.cfg_;
}
ShaderState::Config ShaderConfigurer::configure(const State *state)
{
  ShaderConfigurer configurer;
  configurer.addState(state);
  return configurer.cfg_;
}

/////////////
/////////////

ShaderConfigurer::ShaderConfigurer(const ShaderState::Config &cfg)
: cfg_(cfg) {}

ShaderConfigurer::ShaderConfigurer()
: numLights_(0)
{
  // default is using seperate attributes.
  cfg_.feedbackMode_ = GL_SEPARATE_ATTRIBS;
  cfg_.feedbackStage_ = GL_VERTEX_SHADER;
  // sets the minimum version
  cfg_.setVersion(330);
  // initially no lights added
  define("NUM_LIGHTS", "0");
}

ShaderState::Config& ShaderConfigurer::cfg()
{
  return cfg_;
}

void ShaderConfigurer::setVersion(GLuint version)
{
  cfg_.setVersion(version);
}

void ShaderConfigurer::addNode(const StateNode *node)
{
  // TODO: collect child shader inputs where the shader is
  // active and associate them with shader locations after
  // compiling so that they can be enabled automatically without any
  // lookups.
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
    const ShaderInputState *sis = (const ShaderInputState*)s;

    // remember inputs, they will be enabled automatically
    // when the shader is enabled.
    for(ShaderInputState::InputItConst it=sis->inputs().begin(); it!=sis->inputs().end(); ++it)
    { cfg_.inputs_[it->name_] = it->in_; }
  }
  if(dynamic_cast<const FeedbackState*>(s) != NULL)
  {
    const FeedbackState *m = (const FeedbackState*)s;
    cfg_.feedbackMode_ = m->feedbackMode();
    cfg_.feedbackStage_ = m->feedbackStage();
    for(list< ref_ptr<VertexAttribute> >::const_iterator
        it=m->feedbackAttributes().begin(); it!=m->feedbackAttributes().end(); ++it)
    {
      cfg_.feedbackAttributes_.push_back((*it)->name());
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

  setVersion( s->shaderVersion() );
  addDefines( s->shaderDefines() );
  addFunctions( s->shaderFunctions() );

  if(dynamic_cast<const StateSequence*>(s) != NULL) {
    // add global sequence state
    addState(((StateSequence*)s)->globalState().get());
    // do not add joined states of sequences
    return;
  }
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
