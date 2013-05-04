/*
 * shader-configurer.cpp
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#include <regen/gl-types/shader-input-container.h>
#include <regen/shading/light-state.h>
#include <regen/meshes/mesh-state.h>
#include <regen/utility/string-util.h>

#include "state-configurer.h"
using namespace regen;

State::Config StateConfigurer::configure(const StateNode *node)
{
  StateConfigurer configurer;
  configurer.addNode(node);
  return configurer.cfg_;
}
State::Config StateConfigurer::configure(const State *state)
{
  StateConfigurer configurer;
  configurer.addState(state);
  return configurer.cfg_;
}

/////////////
/////////////

StateConfigurer::StateConfigurer(const State::Config &cfg)
: cfg_(cfg) {}

StateConfigurer::StateConfigurer()
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

State::Config& StateConfigurer::cfg()
{
  return cfg_;
}

void StateConfigurer::setVersion(GLuint version)
{
  cfg_.setVersion(version);
}

void StateConfigurer::addNode(const StateNode *node)
{
  if(node->hasParent()) {
    addNode(node->parent());
  }
  addState(node->state().get());
}

void StateConfigurer::addState(const State *s)
{
  if(s->isHidden()) { return; }

  const HasInput *sis = dynamic_cast<const HasInput*>(s);
  if(sis != NULL)
  {
    const ref_ptr<ShaderInputContainer> &container = sis->inputContainer();

    // remember inputs, they will be enabled automatically
    // when the shader is enabled.
    for(ShaderInputList::const_iterator it=container->inputs().begin(); it!=container->inputs().end(); ++it)
    {
      cfg_.inputs_[it->name_] = it->in_;

      define(REGEN_STRING("HAS_"<<it->in_->name()), "TRUE");
      if(it->in_->numInstances()>1)
      { define("HAS_INSTANCES", "TRUE"); }
    }
  }
  if(dynamic_cast<const FeedbackState*>(s) != NULL)
  {
    const FeedbackState *m = (const FeedbackState*)s;
    cfg_.feedbackMode_ = m->feedbackMode();
    cfg_.feedbackStage_ = m->feedbackStage();
    for(list< ref_ptr<ShaderInput> >::const_iterator
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
        REGEN_STRING("TEX_ID" << cfg_.textures_.size()),
        REGEN_STRING(t->stateID()));
    // remember the number of textures used
    define("NUM_TEXTURES", REGEN_STRING(cfg_.textures_.size()+1));
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

void StateConfigurer::addDefines(const map<string,string> &defines)
{
  for(map<string,string>::const_iterator it=defines.begin(); it!=defines.end(); ++it)
    define(it->first,it->second);
}
void StateConfigurer::addFunctions(const map<string,string> &functions)
{
  for(map<string,string>::const_iterator it=functions.begin(); it!=functions.end(); ++it)
    defineFunction(it->first,it->second);
}

void StateConfigurer::define(const string &name, const string &value)
{
  cfg_.defines_[name] = value;
}
void StateConfigurer::defineFunction(const string &name, const string &value)
{
  cfg_.functions_[name] = value;
}
