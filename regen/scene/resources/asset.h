/*
 * asset.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_RESOURCE_ASSET_H_
#define REGEN_SCENE_RESOURCE_ASSET_H_

#include <regen/scene/resources.h>
#include <regen/scene/scene-parser.h>

#include <regen/meshes/assimp-importer.h>

namespace regen {
namespace scene {
  /**
   * Provides AssetImporter instances from SceneInputNode data.
   */
  class AssetResource : public ResourceProvider<AssetImporter> {
  public:
    AssetResource();
    // Override
    ref_ptr<AssetImporter> createResource(
        SceneParser *parser, SceneInputNode &input);
  };
}}

#endif /* ASSET_H_ */
