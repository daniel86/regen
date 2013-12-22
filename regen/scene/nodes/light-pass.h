/*
 * light-pass.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_LIGHT_PASS_H_
#define REGEN_SCENE_LIGHT_PASS_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>
#include <regen/states/state-configurer.h>
#include <regen/scene/states/input.h>
#include <regen/scene/states/texture.h>
#include <regen/scene/resource-manager.h>

#define REGEN_LIGHT_PASS_NODE_CATEGORY "light-pass"

#include <regen/states/light-pass.h>

namespace regen {
namespace scene {
  /**
   * Processes SceneInput and creates LightPass nodes.
   */
  class LightPassNodeProvider : public NodeProcessor {
  public:
    LightPassNodeProvider()
    : NodeProcessor(REGEN_LIGHT_PASS_NODE_CATEGORY)
    {}

    // Override
    void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<StateNode> &parent)
    {
      if(!input.hasAttribute("shader")) {
        REGEN_WARN("Missing shader attribute for " << input.getDescription() << ".");
        return;
      }
      ref_ptr<LightPass> x = ref_ptr<LightPass>::alloc(
          input.getValue<Light::Type>("type", Light::SPOT),
          input.getValue("shader"));
      parent->state()->joinStates(x);

      x->setShadowFiltering(input.getValue<ShadowFilterMode>(
          "shadow-filter", SHADOW_FILTERING_NONE));
      bool useShadows = false, toggle = true;

      const list< ref_ptr<SceneInputNode> > &childs = input.getChildren();
      for(list< ref_ptr<SceneInputNode> >::const_iterator
          it=childs.begin(); it!=childs.end(); ++it)
      {
        ref_ptr<SceneInputNode> n = *it;
        ref_ptr<Light> light = parser->getResources()->getLight(parser,n->getName());
        if(light.get()==NULL) {
          REGEN_WARN("Unable to find Light for '" << n->getDescription() << "'.");
          continue;
        }
        list< ref_ptr<ShaderInput> > inputs;

        ref_ptr<Texture> shadowMap;
        ref_ptr<Texture> shadowColorMap;
        ref_ptr<LightCamera> shadowCamera;
        if(n->hasAttribute("shadow-camera")) {
          shadowCamera = ref_ptr<LightCamera>::upCast(
              parser->getResources()->getCamera(parser,n->getValue("shadow-camera")));
          if(shadowCamera.get()==NULL) {
            REGEN_WARN("Unable to find LightCamera for '" << n->getDescription() << "'.");
          }
        }
        if(n->hasAttribute("shadow-buffer") || n->hasAttribute("shadow-texture")) {
          shadowMap = TextureStateProvider::getTexture(parser, *n.get(),
                  "shadow-texture", "shadow-buffer", "shadow-attachment");
          if(shadowMap.get()==NULL) {
            REGEN_WARN("Unable to find ShadowMap for '" << n->getDescription() << "'.");
          }
        }
        if(n->hasAttribute("shadow-buffer") || n->hasAttribute("shadow-color-texture")) {
          shadowColorMap = TextureStateProvider::getTexture(parser, *n.get(),
                  "shadow-color-texture", "shadow-buffer", "shadow-color-attachment");
        }
        if(toggle) {
          useShadows = (shadowMap.get()!=NULL);
          toggle = false;
        }
        else if((shadowMap.get()!=NULL) != useShadows) {
          REGEN_WARN("Ignoring " << input.getDescription() << ".");
          continue;
        }

        // Each light pass can have a set of ShaderInput's
        const list< ref_ptr<SceneInputNode> > &childs = n->getChildren();
        for(list< ref_ptr<SceneInputNode> >::const_iterator
            it=childs.begin(); it!=childs.end(); ++it)
        {
          ref_ptr<SceneInputNode> m = *it;
          if(m->getCategory()=="input") {
            inputs.push_back(InputStateProvider::createShaderInput(parser,*m.get()));
          }
          else {
            REGEN_WARN("Unhandled node " << m->getDescription() << ".");
          }
        }

        x->addLight(light,shadowCamera,shadowMap,shadowColorMap,inputs);
      }

      StateConfigurer shaderConfigurer;
      shaderConfigurer.addNode(parent.get());
      x->createShader(shaderConfigurer.cfg());
    }
  };
}}

#endif /* REGEN_SCENE_LIGHT_PASS_H_ */
