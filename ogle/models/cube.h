/*
 * Cube.h
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#ifndef CUBE_H_
#define CUBE_H_

#include <ogle/states/mesh-state.h>
#include <ogle/algebra/vector.h>

/**
 * three-dimensional solid object bounded by six square faces,
 * facets or sides, with three meeting at each vertex - a cube ;)
 * The cube is centered at (0,0,0) and scaled by a user specified
 * factor.
 */
class UnitCube : public IndexedMeshState
{
public:
  enum TexcoMode {
    // do not generate texture coordinates
    TEXCO_MODE_NONE,
    // generate 2D uv coordinates
    TEXCO_MODE_UV,
    // generate 3D coordinates for cube mapping
    TEXCO_MODE_CUBE_MAP
  };
  struct Config {
    // scaling for the position attribute.
    // with vec3(1) a unit cube is created
    Vec3f posScale;
    // cube xyz rotation
    Vec3f rotation;
    // scaling vector for TEXCO_MODE_UV
    Vec2f texcoScale;
    // texture coordinate mode
    TexcoMode texcoMode;
    // generate normal attribute ?
    GLboolean isNormalRequired;
    GLboolean isTangentRequired;
    Config();
  };

  UnitCube(const Config &cfg=Config());
  void updateAttributes(const Config &cfg=Config());
};

#endif /* CUBE_H_ */
