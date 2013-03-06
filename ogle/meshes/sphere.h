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
 * Perfectly round geometrical object in
 * three-dimensional space - a sphere ;)
 * The sphere is centered at (0,0,0) and scaled by a user specified
 * factor.
 */
class Sphere : public MeshState
{
public:
  enum TexcoMode {
    // do not generate texture coordinates
    TEXCO_MODE_NONE,
    // generate 2D uv coordinates
    TEXCO_MODE_UV,
    // generate 3D coordinates for cube mapping
    TEXCO_MODE_SPHERE_MAP
  };
  struct Config {
    // scaling for the position attribute.
    // with vec3(1) a unit sphere is created
    Vec3f posScale;
    // scaling vector for TEXCO_MODE_UV
    Vec2f texcoScale;
    // number of surface divisions
    GLuint levelOfDetail;
    // texture coordinate mode
    TexcoMode texcoMode;
    // generate normal attribute ?
    GLboolean isNormalRequired;
    GLboolean isTangentRequired;
    Config();
  };

  Sphere(const Config &cfg=Config());
  void updateAttributes(const Config &cfg=Config());

protected:
  ref_ptr<PositionShaderInput> pos_;
  ref_ptr<NormalShaderInput> nor_;
  ref_ptr<TexcoShaderInput> texco_;
  ref_ptr<TangentShaderInput> tan_;
};

/**
 * A sprite that generates sphere normals and discards
 * all fragments outside the given radius.
 * It is not possible to define per vertex coordinates
 * for the sphere because each sphere is extruded from a single point
 * in space.
 */
class SpriteSphere : public MeshState
{
public:
  SpriteSphere(GLfloat *radius, Vec3f *position, GLuint sphereCount);
  void updateAttributes(GLfloat *radius, Vec3f *position, GLuint sphereCount);
};

} // end ogle namespace

#endif /* SPHERE_H_ */
