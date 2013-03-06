/*
 * cone.h
 *
 *  Created on: 03.02.2013
 *      Author: daniel
 */

#ifndef CONE_H_
#define CONE_H_

#include <ogle/meshes/mesh-state.h>
#include <ogle/algebra/vector.h>

namespace ogle {

/**
 * A cone is an n-dimensional geometric shape that tapers smoothly from a base
 * (usually flat and circular) to a point called the apex or vertex.
 */
class OpenedCone : public MeshState
{
public:
  struct Config {
    // cosine of cone angle
    GLfloat cosAngle;
    // distance from apex to base
    GLfloat height;
    // generate normal attribute ?
    GLboolean isNormalRequired;
    // subdivisions = 4*pow(levelOfDetail,2)
    GLint levelOfDetail;
    Config();
  };

  OpenedCone(const Config &cfg=Config());
  void updateAttributes(const Config &cfg=Config());

  Config& meshCfg();

protected:
  Config coneCfg_;
  ref_ptr<ShaderInput> nor_;
  ref_ptr<ShaderInput> pos_;
};

class ClosedCone : public IndexedMeshState
{
public:
  static ref_ptr<ClosedCone> getBaseCone();

  struct Config {
    GLfloat radius;
    GLfloat height;
    GLboolean isNormalRequired;
    GLboolean isBaseRequired;
    GLint levelOfDetail;
    Config();
  };

  ClosedCone(const Config &cfg=Config());
  void updateAttributes(const Config &cfg=Config());

  Config& meshCfg();

protected:
  Config coneCfg_;
  ref_ptr<ShaderInput> nor_;
  ref_ptr<ShaderInput> pos_;
};

} // end ogle namespace

#endif /* CONE_H_ */
