/*
 * filter-sequence.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "filter-sequence.h"
using namespace regen::scene;
using namespace regen;

#include <regen/states/state-configurer.h>
#include <regen/scene/resource-manager.h>

#define REGEN_FILTER_SEQUENCE_NODE_CATEGORY "filter-sequence"

FilterSequenceNodeProvider::FilterSequenceNodeProvider()
: NodeProcessor(REGEN_FILTER_SEQUENCE_NODE_CATEGORY)
{}

void FilterSequenceNodeProvider::processInput(
    SceneParser *parser,
    SceneInputNode &input,
    const ref_ptr<StateNode> &parent)
{
  if(!input.hasAttribute("texture") && !input.hasAttribute("fbo")) {
    REGEN_WARN("Ignoring " << input.getDescription() << " without input texture.");
    return;
  }

  ref_ptr<Texture> tex;
  if(input.hasAttribute("texture")) {
    tex = parser->getResources()->getTexture(parser,input.getValue("texture"));
    if(tex.get()==NULL) {
      REGEN_WARN("Unable to find Texture for " << input.getDescription() << ".");
      return;
    }
  }
  else {
    ref_ptr<FBO> fbo = parser->getResources()->getFBO(parser,input.getValue("fbo"));
    if(fbo.get()==NULL) {
      REGEN_WARN("Unable to find FBO for " << input.getDescription() << ".");
      return;
    }
    GLuint attachment = input.getValue<GLuint>("attachment",0);
    vector< ref_ptr<Texture> > textures = fbo->colorTextures();
    if(attachment >= textures.size()) {
      REGEN_WARN("FBO " << input.getValue("fbo") <<
          " has less then " << (attachment+1) << " attachments.");
      return;
    }
    tex = textures[attachment];
  }

  ref_ptr<FilterSequence> filterSeq = ref_ptr<FilterSequence>::alloc(
      tex,
      input.getValue<bool>("bind-input",true));
  filterSeq->set_format(glenum::textureFormat(
      input.getValue<string>("format", "NONE")));
  filterSeq->set_internalFormat(glenum::textureInternalFormat(
      input.getValue<string>("internal-format", "NONE")));
  filterSeq->set_pixelType(glenum::pixelType(
      input.getValue<string>("pixel-type", "NONE")));
  if(input.hasAttribute("clear-color")) {
    filterSeq->setClearColor(
        input.getValue<Vec4f>("clear-color",Vec4f(0.0f)));
  }

  const list< ref_ptr<SceneInputNode> > &childs = input.getChildren();
  for(list< ref_ptr<SceneInputNode> >::const_iterator
      it=childs.begin(); it!=childs.end(); ++it)
  {
    ref_ptr<SceneInputNode> n = *it;
    if(!n->hasAttribute("shader")) {
      REGEN_WARN("Ignoring filter '" << n->getDescription() << "' without shader.");
      continue;
    }
    filterSeq->addFilter(ref_ptr<Filter>::alloc(
        n->getValue("shader"),
        n->getValue<GLfloat>("scale",1.0f)));
  }

  parent->state()->joinStates(filterSeq);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(parent.get());
  filterSeq->createShader(shaderConfigurer.cfg());

  // Make the filter output available to the Texture resource provider.
  parser->getResources()->putTexture(input.getName(), filterSeq->output());
}
