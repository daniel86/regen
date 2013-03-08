/*
 * rectangle.h
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#ifndef __RECTANGLE_H__
#define __RECTANGLE_H__

#include <ogle/meshes/mesh-state.h>

namespace ogle {
/**
 * \brief A polygon with four edges, vertices and right angles.
 */
class Rectangle : public MeshState
{
public:
  /**
   * Configures texture coordinates.
   */
  enum TexcoMode {
    /** do not generate texture coordinates */
    TEXCO_MODE_NONE,
    /** generate 2D uv coordinates */
    TEXCO_MODE_UV
  };
  /**
   * Vertex data configuration.
   */
  struct Config {
    /** number of surface divisions. */
    GLuint levelOfDetail;
    /** scaling for the position attribute. */
    Vec3f posScale;
    /** cube xyz rotation. */
    Vec3f rotation;
    /** cube xyz translation. */
    Vec3f translation;
    /** scaling vector for TEXCO_MODE_UV. */
    Vec2f texcoScale;
    /** generate normal attribute */
    GLboolean isNormalRequired;
    /** generate texture coordinates */
    GLboolean isTexcoRequired;
    /** generate tangent attribute */
    GLboolean isTangentRequired;
    GLboolean centerAtOrigin;
    Config();
  };

  /**
   * The unit quad has side length 2.0 and is parallel to the xy plane
   * with z=0. No normals,tangents and texture coordinates are generated.
   * @return the unit quad.
   */
  static const ref_ptr<Rectangle>& getUnitQuad();

  Rectangle(const Config &cfg=Config());
  /**
   * Updates vertex data based on given configuration.
   * @param cfg vertex data configuration.
   */
  void updateAttributes(Config cfg);

protected:
  ref_ptr<PositionShaderInput> pos_;
  ref_ptr<ShaderInput> nor_;
  ref_ptr<ShaderInput> tan_;
  ref_ptr<ShaderInput> texco_;
};

} // end ogle namespace

#endif /* __RECTANGLE_H__ */
