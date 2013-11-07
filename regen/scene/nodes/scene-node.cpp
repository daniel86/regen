/*
 * scene-node.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "scene-node.h"
using namespace regen::scene;
using namespace regen;

#include <regen/scene/resource-manager.h>

#define REGEN_NODE_CATEGORY "node"

/**
 * Sorts children using Eye-Depth comparator.
 */
class SortByModelMatrix : public State
{
public:
  /**
   * Default constructor.
   * @param n The scene node.
   * @param cam Camera reference.
   * @param frontToBack If true sorting is done front to back.
   */
  SortByModelMatrix(
      const ref_ptr<StateNode> &n,
      const ref_ptr<Camera> &cam,
      GLboolean frontToBack)
  : State(),
    n_(n),
    comparator_(cam,frontToBack)
  {}

  void enable(RenderState *state)
  { n_->childs().sort(comparator_); }

protected:
  ref_ptr<StateNode> n_;
  NodeEyeDepthComparator comparator_;
};

SceneNodeProcessor::SceneNodeProcessor()
: NodeProcessor(REGEN_NODE_CATEGORY)
{}

void SceneNodeProcessor::processInput(
    SceneParser *parser,
    SceneInputNode &input,
    const ref_ptr<StateNode> &parent)
{
  if(input.hasAttribute("import")) {
    // Handle node imports
    const ref_ptr<SceneInputNode> &root = parser->getRoot();
    const string importID = input.getValue("import");
    ref_ptr<SceneInputNode> imported =
        root->getFirstChild(REGEN_NODE_CATEGORY, importID);
    if(imported.get()!=NULL) {
      processInput(parser, *imported.get(), parent);
    }
  }
  else {
    ref_ptr<State> state = ref_ptr<State>::alloc();
    ref_ptr<StateNode> newNode = ref_ptr<StateNode>::alloc(state);
    parent->addChild(newNode);

    // Sort node children by model view matrix.
    GLuint sortMode = input.getValue<GLuint>("sort",0);
    if(sortMode!=0u) {
      ref_ptr<Camera> sortCam =
          parser->getResources()->getCamera(parser,input.getValue<string>("sort-camera",""));
      if(sortCam.get()==NULL) {
        REGEN_WARN("Unable to find Camera for '" << input.getDescription() << "'.");
      }
      else {
        state->joinStatesFront(
            ref_ptr<SortByModelMatrix>::alloc(newNode,sortCam,(sortMode==1)));
      }
    }

    // Add this node to shadow maps
    if(input.hasAttribute("shadow-maps")) {
      // Add to all ShadowMap's
      if(input.getValue("shadow-maps") == "*") {
        ref_ptr<SceneInputNode> root = parser->getRoot();
        list< ref_ptr<SceneInputNode> > childs = root->getChildren("shadow-map");
        // Add to all ShadowMaps
        for(list< ref_ptr<SceneInputNode> >::const_iterator
            jt=childs.begin(); jt!=childs.end(); ++jt)
        {
          const ref_ptr<SceneInputNode> &x = *jt;
          ref_ptr<ShadowMap> shadowMap =
              parser->getResources()->getShadowMap(parser,x->getName());
          if(shadowMap.get()==NULL) {
            REGEN_WARN("Unable to find ShadowMap for '" << x->getDescription() << "'.");
          }
          else {
            shadowMap->addCaster(newNode);
          }
        }
      }
      // Add to set of ShadowMap's
      else {
        vector<string> targets;
        boost::split(targets, input.getValue("shadow-maps"), boost::is_any_of(","));

        for(vector<string>::iterator it=targets.begin(); it!=targets.end(); ++it) {
          const string &shadowMapID = *it;

          ref_ptr<ShadowMap> shadowMap =
              parser->getResources()->getShadowMap(parser,shadowMapID);
          if(shadowMap.get()==NULL) {
            REGEN_WARN("Unable to find ShadowMap for '" << input.getDescription() << "'.");
          }
          else {
            shadowMap->addCaster(newNode);
          }
        }
      }
    }

    // Process node children
    const list< ref_ptr<SceneInputNode> > &childs = input.getChildren();
    for(list< ref_ptr<SceneInputNode> >::const_iterator
        it=childs.begin(); it!=childs.end(); ++it)
    {
      const ref_ptr<SceneInputNode> &x = *it;
      // First try node processor
      ref_ptr<NodeProcessor> nodeProcessor = parser->getNodeProcessor(x->getCategory());
      if(nodeProcessor.get()!=NULL) {
        nodeProcessor->processInput(parser, *x.get(), newNode);
        continue;
      }
      // Second try state processor
      ref_ptr<StateProcessor> stateProcessor = parser->getStateProcessor(x->getCategory());
      if(stateProcessor.get()!=NULL) {
        stateProcessor->processInput(parser, *x.get(), newNode->state());
        continue;
      }
      REGEN_WARN("No processor registered for '" << x->getDescription() << "'.");
    }
  }
}
