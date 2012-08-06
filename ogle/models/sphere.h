/*
 * sphere.h
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#ifndef SPHERE_H_
#define SPHERE_H_

#include <ogle/states/attribute-state.h>
#include <ogle/algebra/vector.h>

/**
 * A simple sphere mesh.
 */
class Sphere : public AttributeState
{
public:
  /**
   * Default constructor.
   */
  Sphere();
  /**
   * generate vertex data using some params.
   */
  void createVertexData(
      unsigned int levelOfDetail,
      const Vec3f &scale=(Vec3f){1.0,1.0,1.0},
      const Vec2f &uvScale=(Vec2f){1.0,1.0},
      int uvMode=0);
};

#endif /* SPHERE_H_ */
