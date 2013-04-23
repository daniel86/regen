/*
 * sphere.h
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#ifndef SPHERE_H_
#define SPHERE_H_

#include <regen/meshes/mesh-state.h>
#include <regen/math/vector.h>
#include <regen/utility/interfaces.h>

namespace regen {

/**
 * \brief Round geometrical object in three-dimensional space - a sphere ;)
 *
 * The sphere is centered at (0,0,0).
 * A LoD factor can be used to configure sphere tesselation.
 * @note take a look at SphereSprite for an alternative.
 */
class Sphere : public Mesh
{
public:
  /**
   * Configures texture coordinates.
   */
  enum TexcoMode {
    /** do not generate texture coordinates */
    TEXCO_MODE_NONE,
    /** generate 2D uv coordinates */
    TEXCO_MODE_UV,
    /** generate 3D coordinates for cube mapping */
    TEXCO_MODE_SPHERE_MAP
  };
  /**
   * Vertex data configuration.
   */
  struct Config {
    /** scaling for the position attribute. */
    Vec3f posScale;
    /** scaling vector for TEXCO_MODE_UV */
    Vec2f texcoScale;
    /** number of surface divisions */
    GLuint levelOfDetail;
    /** texture coordinate mode */
    TexcoMode texcoMode;
    /** generate normal attribute */
    GLboolean isNormalRequired;
    /** generate tangent attribute */
    GLboolean isTangentRequired;
    Config();
  };

  /**
   * @param cfg the mesh configuration.
   */
  Sphere(const Config &cfg=Config());
  /**
   * Updates vertex data based on given configuration.
   * @param cfg vertex data configuration.
   */
  void updateAttributes(const Config &cfg=Config());

protected:
  ref_ptr<ShaderInput3f> pos_;
  ref_ptr<ShaderInput3f> nor_;
  ref_ptr<ShaderInput2f> texco_;
  ref_ptr<ShaderInput4f> tan_;

  struct SphereFace {
    SphereFace(const Vec3f &_p1, const Vec3f &_p2, const Vec3f &_p3)
    : p1(_p1), p2(_p2), p3(_p3) {}
    SphereFace() {}

    Vec3f p1;
    Vec3f p2;
    Vec3f p3;
  };
  static vector<SphereFace>* makeSphere(GLuint levelOfDetail);
};

/**
 * \brief A sprite that generates sphere normals.
 *
 * It is not possible to define per vertex attributes
 * for the sphere because each sphere is extruded from a single point
 * in space. You can only add per sphere attributes to this mesh.
 * This is a nice way to handle spheres because you get a perfectly round shape
 * on the render target you use and you can anti-alias edges in the fragment shader
 * easily.
 */
class SphereSprite : public Mesh, public HasShader
{
public:
  /**
   * Vertex data configuration.
   */
  struct Config {
    /** one radius for each sphere. */
    GLfloat *radius;
    /** one position for each sphere. */
    Vec3f *position;
    /** number of spheres. */
    GLuint sphereCount;
  };

  /**
   * @param cfg the mesh configuration.
   */
  SphereSprite(const Config &cfg);
  /**
   * Updates vertex data based on given configuration.
   * @param cfg vertex data configuration.
   */
  void updateAttributes(const Config &cfg);
};

} // namespace

#endif /* SPHERE_H_ */
