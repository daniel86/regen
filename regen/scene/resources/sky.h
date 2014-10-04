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
#include <regen/sky/high-clouds.h>

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

    ref_ptr<Atmosphere> createAtmosphereLayer(
        const ref_ptr<Sky> &sky,
        SceneParser *parser, SceneInputNode &input);
    ref_ptr<HighCloudLayer> createHighCloudLayer(
        const ref_ptr<Sky> &sky,
        SceneParser *parser, SceneInputNode &input);
  };
}}


#endif /* _XML_RESOURCE_STATE_H_ */
