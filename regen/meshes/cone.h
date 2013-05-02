/*
 * cone.h
 *
 *  Created on: 03.02.2013
 *      Author: daniel
 */

#ifndef CONE_H_
#define CONE_H_

#include <regen/meshes/mesh-state.h>
#include <regen/math/vector.h>

namespace regen {

/**
 * \brief A cone is an n-dimensional geometric shape that tapers smoothly from a base
 * to a point called the apex.
 *
 * The base has a circle shape.
 * OpenedCone does not handle the base geometry, it is defined using GL_TRIANGLE_FAN.
 */
class ConeOpened : public Mesh
{
public:
  /**
   * Vertex data configuration.
   */
  struct Config {
    /** cosine of cone angle */
    GLfloat cosAngle;
    /** distance from apex to base */
    GLfloat height;
    /** generate normal attribute ? */
    GLboolean isNormalRequired;
    /** subdivisions = 4*levelOfDetail^2 */
    GLint levelOfDetail;
    VertexBufferObject::Usage usage;
    Config();
  };

  /**
   * @param cfg the mesh configuration.
   */
  ConeOpened(const Config &cfg=Config());
  /**
   * Updates vertex data based on given configuration.
   * @param cfg vertex data configuration.
   */
  void updateAttributes(const Config &cfg=Config());

protected:
  ref_ptr<ShaderInput> nor_;
  ref_ptr<ShaderInput> pos_;
};

/**
 * \brief A cone is an n-dimensional geometric shape that tapers smoothly from a base
 * to a point called the apex.
 *
 * The base has a circle shape.
 * ClosedCone does handle the base geometry, it is defined using GL_TRIANGLES.
 */
class ConeClosed : public Mesh
{
public:
  /**
   * The 'base' cone has apex=(0,0,0) and opens
   * in positive z direction. The base radius is 0.5 and the apex base
   * distance is 1.0.
   * @return the base cone.
   */
  static ref_ptr<ConeClosed> getBaseCone();

  /**
   * Vertex data configuration.
   */
  struct Config {
    /** the base radius */
    GLfloat radius;
    /** the base apex distance */
    GLfloat height;
    /** generate cone normals */
    GLboolean isNormalRequired;
    /** generate cone base geometry */
    GLboolean isBaseRequired;
    /** level of detail for base circle */
    GLint levelOfDetail;
    VertexBufferObject::Usage usage;
    Config();
  };

  /**
   * @param cfg the mesh configuration.
   */
  ConeClosed(const Config &cfg=Config());
  ConeClosed(const ref_ptr<ShaderInputContainer> &inputContainer);
  /**
   * Updates vertex data based on given configuration.
   * @param cfg vertex data configuration.
   */
  void updateAttributes(const Config &cfg=Config());

protected:
  ref_ptr<ShaderInput> nor_;
  ref_ptr<ShaderInput> pos_;
};

} // namespace

#endif /* CONE_H_ */
