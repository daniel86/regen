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
class UnitQuad : public AttributeState
{
public:
  enum TexcoMode {
    // do not generate texture coordinates
    TEXCO_MODE_NONE,
    // generate 2D uv coordinates
    TEXCO_MODE_UV
  };
  struct Config {
    // number of surface divisions
    GLuint levelOfDetail;
    // scaling for the position attribute.
    // with vec3(1) a unit quad is created
    Vec3f posScale;
    // cube xyz rotation
    Vec3f rotation;
    // scaling vector for TEXCO_MODE_UV
    Vec2f texcoScale;
    // generate normal attribute ?
    GLboolean isNormalRequired;
    // generate texco attribute ?
    GLboolean isTexcoRequired;
    GLboolean isTangentRequired;
    GLboolean centerAtOrigin;
    Config();
  };

  UnitQuad(const Config &cfg=Config());
  void updateAttributes(Config cfg);
};

#endif /* QUAD_H_ */
