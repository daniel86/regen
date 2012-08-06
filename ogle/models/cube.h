/*
 * Cube.h
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#ifndef CUBE_H_
#define CUBE_H_

#include <ogle/states/attribute-state.h>
#include <ogle/algebra/vector.h>

/**
 * A simple cube mesh.
 * Using 8 vertices.
 */
class Cube : public AttributeState
{
public:
  /**
   * Default constructor.
   */
  Cube();
  /**
   * generate vertex data using some params.
   */
  void createVertexData(
      const Vec3f &rotation=(Vec3f){0.0,0.0,0.0},
      const Vec3f &scale=(Vec3f){1.0,1.0,1.0},
      const Vec2f &uvScale=(Vec2f){1.0,1.0},
      bool generateNormal=true,
      bool generateUV=true,
      bool generateCubeMapUV=false);
};

#endif /* CUBE_H_ */
