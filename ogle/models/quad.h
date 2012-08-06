/*
 * Quad.h
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#ifndef QUAD_H_
#define QUAD_H_

#include <ogle/states/attribute-state.h>

/**
 * A simple quad mesh.
 * Using 4 vertices.
 */
class Quad : public AttributeState
{
public:
  /**
   * Default constructor.
   */
  Quad();
  /**
   * generate vertex data using some params.
   */
  void createVertexData(
      const Vec3f &rotation=(Vec3f){0.0,0.0,0.0},
      const Vec3f &scale=(Vec3f){1.0,1.0,1.0},
      const Vec2f &uvScale=(Vec2f){1.0,1.0},
      unsigned int lod=0,
      bool generateUV=true,
      bool generateNormal=true,
      bool centerAtOrigin=true,
      bool isOrtho=false);
  void createOrthoVertexData(
      const Vec3f &scale=(Vec3f){1.0,1.0,1.0},
      bool generateTexco=false);
};

#endif /* QUAD_H_ */
