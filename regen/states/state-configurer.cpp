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

StateConfig StateConfigurer::configure(const StateNode *node)
{
  StateConfigurer configurer;
  configurer.addNode(node);
  return configurer.cfg_;
}
StateConfig StateConfigurer::configure(const State *state)
{
  StateConfigurer configurer;
  configurer.addState(state);
  return configurer.cfg_;
}

/////////////
/////////////

StateConfigurer::StateConfigurer(const StateConfig &cfg)
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

StateConfig& StateConfigurer::cfg()
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

  const HasInput *x0 = dynamic_cast<const HasInput*>(s);
  const FeedbackState *x1 = dynamic_cast<const FeedbackState*>(s);
  const TextureState *x2 = dynamic_cast<const TextureState*>(s);
  const StateSequence *x3 = dynamic_cast<const StateSequence*>(s);

  if(x0 != NULL)
  {
    const ref_ptr<ShaderInputContainer> &container = x0->inputContainer();

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
  if(x1)
  {
    cfg_.feedbackMode_ = x1->feedbackMode();
    cfg_.feedbackStage_ = x1->feedbackStage();
    for(list< ref_ptr<ShaderInput> >::const_iterator
        it=x1->feedbackAttributes().begin(); it!=x1->feedbackAttributes().end(); ++it)
    {
      cfg_.feedbackAttributes_.push_back((*it)->name());
    }
  }
  if(x2)
  {
    // map for loop index to texture id
    define(
        REGEN_STRING("TEX_ID" << cfg_.textures_.size()),
        REGEN_STRING(x2->stateID()));
    // remember the number of textures used
    define("NUM_TEXTURES", REGEN_STRING(cfg_.textures_.size()+1));
    cfg_.textures_[x2->name()] = x2->texture();
    cfg_.inputs_[x2->name()] = x2->texture();
  }

  setVersion( s->shaderVersion() );
  addDefines( s->shaderDefines() );
  addFunctions( s->shaderFunctions() );

  if(x3) {
    // add global sequence state
    addState(x3->globalState().get());
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
