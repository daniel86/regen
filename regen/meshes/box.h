/*
 * box.h
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#ifndef __BOX_H__
#define __BOX_H__

#include <regen/meshes/mesh-state.h>
#include <regen/algebra/vector.h>

namespace ogle {
/**
 * \brief Three-dimensional solid object bounded by six square faces,
 * facets or sides, with three meeting at each vertex - a cube ;)
 *
 * The cube is centered at (0,0,0).
 */
class Box : public Mesh
{
public:
  /**
   * A box with each side having a length of 2.
   * No tangents, normals and texture coordinates are generated for this cube.
   * @return the static unit cube (in range [-1,1]).
   */
  static ref_ptr<Box> getUnitCube();

  /**
   * Configures texture coordinates.
   */
  enum TexcoMode {
    TEXCO_MODE_NONE,   //!< do not generate texture coordinates
    TEXCO_MODE_UV,     //!< generate 2D uv coordinates
    TEXCO_MODE_CUBE_MAP//!< generate 3D coordinates for cube mapping
  };
  /**
   * Vertex data configuration.
   */
  struct Config {
    /** scaling for the position attribute. */
    Vec3f posScale;
    /** cube xyz rotation. */
    Vec3f rotation;
    /** scaling vector for TEXCO_MODE_UV. */
    Vec2f texcoScale;
    /** texture coordinate mode. */
    TexcoMode texcoMode;
    /** generate normal attribute ?. */
    GLboolean isNormalRequired;
    /** generate tangent attribute ?. */
    GLboolean isTangentRequired;
    Config();
  };

  /**
   * @param cfg the mesh configuration.
   */
  Box(const Config &cfg=Config());
  /**
   * Updates vertex data based on given configuration.
   * @param cfg vertex data configuration.
   */
  void updateAttributes(const Config &cfg=Config());
};
} // namespace

#endif /* __BOX_H__ */
