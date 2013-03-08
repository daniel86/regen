/*
 * sphere.h
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#ifndef SPHERE_H_
#define SPHERE_H_

#include <ogle/meshes/mesh-state.h>
#include <ogle/algebra/vector.h>

namespace ogle {

/**
 * \brief Round geometrical object in three-dimensional space - a sphere ;)
 *
 * The sphere is centered at (0,0,0).
 * A LoD factor can be used to configure sphere tesselation.
 */
class Sphere : public MeshState
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

  Sphere(const Config &cfg=Config());
  /**
   * Updates vertex data based on given configuration.
   * @param cfg vertex data configuration.
   */
  void updateAttributes(const Config &cfg=Config());

protected:
  ref_ptr<PositionShaderInput> pos_;
  ref_ptr<NormalShaderInput> nor_;
  ref_ptr<TexcoShaderInput> texco_;
  ref_ptr<TangentShaderInput> tan_;

  struct SphereFace {
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
class SphereSprite : public MeshState
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

  SphereSprite(const Config &cfg);
  /**
   * Updates vertex data based on given configuration.
   * @param cfg vertex data configuration.
   */
  void updateAttributes(const Config &cfg);
};

} // end ogle namespace

#endif /* SPHERE_H_ */
