/*
 * shadow-map.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_RESOURCE_SHADOW_MAP_H_
#define REGEN_SCENE_RESOURCE_SHADOW_MAP_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/resources.h>

#include <regen/states/shadow-map.h>

namespace regen {
namespace scene {
  /**
   * Provides ShadowMap instances from SceneInputNode data.
   */
  class ShadowMapResource : public ResourceProvider<ShadowMap> {
  public:
    ShadowMapResource();
    // Override
    ref_ptr<ShadowMap> createResource(
        SceneParser *parser, SceneInputNode &input);
  };
}}

#endif /* REGEN_SCENE_RESOURCE_SHADOW_MAP_H_ */
