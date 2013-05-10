/*
 * rectangle.h
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#ifndef __RECTANGLE_H__
#define __RECTANGLE_H__

#include <regen/meshes/mesh-state.h>

namespace regen {
  /**
   * \brief A polygon with four edges, vertices and right angles.
   */
  class Rectangle : public Mesh
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
      /** flag indicating if the quad center should be translated to origin. */
      GLboolean centerAtOrigin;
      /** VBO usage hint. */
      VBO::Usage usage;
      Config();
    };

    /**
     * The unit quad has side length 2.0 and is parallel to the xy plane
     * with z=0. No normals,tangents and texture coordinates are generated.
     * @return the unit quad.
     */
    static ref_ptr<Rectangle> getUnitQuad();

    /**
     * @param cfg the mesh configuration.
     */
    Rectangle(const Config &cfg=Config());
    /**
     * @param inputContainer custom input container.
     */
    Rectangle(const ref_ptr<ShaderInputContainer> &inputContainer);
    /**
     * Updates vertex data based on given configuration.
     * @param cfg vertex data configuration.
     */
    void updateAttributes(Config cfg);

  protected:
    ref_ptr<ShaderInput> pos_;
    ref_ptr<ShaderInput> nor_;
    ref_ptr<ShaderInput> tan_;
    ref_ptr<ShaderInput> texco_;
  };
} // namespace

#endif /* __RECTANGLE_H__ */
