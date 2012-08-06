/*
 * volume.cpp
 *
 *  Created on: 08.03.2012
 *      Author: daniel
 */

#include "volume.h"

BoxVolume::BoxVolume()
: Volume(), Cube()
{
  bool generateNormal = false;
  bool generateUV = false;
  bool generateCubeMapUV = false;
  createVertexData(
      Vec3f(0.0, 0.0, 0.0), // rotate
      Vec3f(2.0, 2.0, 2.0), // scale
      Vec2f(1.0, 1.0), // uv scale
      generateNormal, generateUV, generateCubeMapUV
      );
}
