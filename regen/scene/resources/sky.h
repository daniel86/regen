/*
 * sky.h
 *
 *  Created on: Jan 5, 2014
 *      Author: daniel
 */

#ifndef _XML_RESOURCE_STATE_H_
#define _XML_RESOURCE_STATE_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/resources.h>

#include <regen/sky/sky.h>
#include <regen/sky/atmosphere.h>
#include <regen/sky/cloud-layer.h>
#include <regen/sky/star-map.h>
#include <regen/sky/stars.h>
#include <regen/sky/moon.h>

namespace regen {
namespace scene {
  /**
   * Provides MeshVector instances from SceneInputNode data.
   */
  class SkyResource : public ResourceProvider<Sky> {
  public:
    SkyResource();

    // Override
    ref_ptr<Sky> createResource(
        SceneParser *parser, SceneInputNode &input);

  protected:

    ref_ptr<StarMapLayer> createStarMapLayer(
        const ref_ptr<Sky> &sky,
        SceneParser *parser, SceneInputNode &input);
    ref_ptr<StarsLayer> createStarsLayer(
        const ref_ptr<Sky> &sky,
        SceneParser *parser, SceneInputNode &input);
    ref_ptr<Atmosphere> createAtmosphereLayer(
        const ref_ptr<Sky> &sky,
        SceneParser *parser, SceneInputNode &input);
    ref_ptr<CloudLayer> createCloudLayer(
        const ref_ptr<Sky> &sky,
        SceneParser *parser, SceneInputNode &input);
    ref_ptr<MoonLayer> createMoonLayer(
        const ref_ptr<Sky> &sky,
        SceneParser *parser, SceneInputNode &input);
  };
}}


#endif /* _XML_RESOURCE_STATE_H_ */
