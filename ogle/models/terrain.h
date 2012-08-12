/*
 * terrain.h
 *
 *  Created on: 11.04.2012
 *      Author: daniel
 */

#ifndef TERRAIN_H_
#define TERRAIN_H_

#include <string>
using namespace std;

#include <ogle/states/attribute-state.h>
#include <ogle/algebra/vector.h>

/**
 * A terrain mesh using quad patches and a height map for tesselation.
 * Gourad shading will not work because tessalation is done after vertex shader.
 */
class Terrain : public IndexedAttributeState
{
public:
  /**
   * @param size width,depth in world space
   * @param numPatches width,height of patch grid. width*height patches will be generated.
   */
  Terrain(const Vec2f &size, const Vec2i &numPatched);

  ref_ptr<Texture> loadNormalMap(const string &normalMapPath);
  ref_ptr<Texture> normalMap();

  ref_ptr<Texture> loadHeightMap(const string &heightMapPath);
  ref_ptr<Texture> heightMap();

  ref_ptr<Texture> loadColorMap(const string &colorMapPath);
  ref_ptr<Texture> colorMap();

protected:
  ref_ptr<Texture> colorMap_;
  ref_ptr<Texture> normalMap_;
  ref_ptr<Texture> heightMap_;
};

#endif /* TERRAIN_H_ */
