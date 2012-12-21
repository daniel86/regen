/*
 * rectangle.h
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#ifndef __RECTANGLE_H__
#define __RECTANGLE_H__

#include <ogle/states/mesh-state.h>

/**
 * A simple Rectangle mesh.
 */
class Rectangle : public MeshState
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
    // cube xyz translation
    Vec3f translation;
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

  Rectangle(const Config &cfg=Config());
  void updateAttributes(Config cfg);
};

#endif /* __RECTANGLE_H__ */
