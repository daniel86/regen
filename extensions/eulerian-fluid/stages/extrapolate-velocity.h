
#ifndef EULERIAN_EXTRAPOLATE_VELOCITY_H_
#define EULERIAN_EXTRAPOLATE_VELOCITY_H_

#include "primitive.h"
#include "helper.h"

/**
 * The x-component of the velocity field u can be extrapolated into the air region, where phi > 0,
 * by solving the equation
 *      delta u / delta tau = - ( NABLA omega / | NABLA omega | ) * NABLA u
 * where tau is fictitious time.
 *
 * From: ENRIGHT, D., M ARSCHNER , S., AND F EDKIW, R. 2002.
 * Animation and rendering of complex water surfaces. In Proc. SIGGRAPH, 736-744.
 */
class EulerianExtrapolateVelocity : public EulerianStage
{
public:
  EulerianExtrapolateVelocity(EulerianPrimitive*);

  void set_velocityBuffer(const FluidBuffer &buffer);
  void set_levelSetTexture(ref_ptr<Texture> tex);
  void set_obstaclesTexture(ref_ptr<Texture> tex);

  GLint numOfExtrapolationIterations() const {
    return numOfExtrapolationIterations_;
  }
  void set_numOfExtrapolationIterations(GLuint numOfExtrapolationIterations) {
    numOfExtrapolationIterations_ = numOfExtrapolationIterations;
  }

  virtual void update();

protected:
  ref_ptr<Shader> extrapolateVelocityShader_;
  FluidBuffer velocityBuffer_;
  ref_ptr<Texture> obstaclesTexture_;
  ref_ptr<Texture> levelSetTexture_;
  GLuint numOfExtrapolationIterations_;
};

#endif /* EULERIAN_ADVECT_H_ */
