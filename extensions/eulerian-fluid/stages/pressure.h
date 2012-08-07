
#ifndef EULERIAN_PRESSURE_H_
#define EULERIAN_PRESSURE_H_

#include "primitive.h"
#include "helper.h"

/**
 * Because the molecules of a fluid can move around each other, they tend to "squish" and "slosh."
 * When force is applied to a fluid, it does not instantly propagate through the entire volume.
 * Instead, the molecules close to the force push on those farther away, and pressure builds up.
 * Because pressure is force per unit area, any pressure in the fluid naturally leads to acceleration.
 * (Think of Newton's second law, F=m*a.)
 *
 * Every velocity field is the sum of an incompressible field and a gradient field.
 * To obtain an incompressible field we simply subtract the gradient field
 * from current velocities.
 */
class EulerianPressure : public EulerianStage
{
public:
  EulerianPressure(EulerianPrimitive*);

  void set_velocityBuffer(const FluidBuffer &buffer);
  void set_divergenceBuffer(const FluidBuffer &buffer);
  void set_pressureBuffer(const FluidBuffer &buffer);
  void set_obstaclesTexture(ref_ptr<Texture> tex);
  void set_levelSetTexture(ref_ptr<Texture> tex);

  void set_alpha(float alpha) {
    this->alpha_->set_value( alpha );
  }

  /**
   * 1/4 for 2D simulations and 1/6 for 3D simulations widely used.
   */
  void set_inverseBeta(float inverseBeta) {
    this->inverseBeta_->set_value( inverseBeta );
  }

  void set_fluidDensity(float rho) {
    this->densityInverse_->set_value( 1.0f / rho );
  }

  void set_halfInverseCellSize(float size) {
    this->halfCell_->set_value( size );
  }

  void set_numPressureIterations(GLuint numPressureIterations) {
    numPressureIterations_ = numPressureIterations;
  }

  virtual void update();

protected:
  GLuint numPressureIterations_;

  ref_ptr<Shader> divergenceShader_;
  ref_ptr<Shader> pressureShader_;
  ref_ptr<Shader> subtractPressureGradientShader_;
  ref_ptr<UniformFloat> alpha_;
  ref_ptr<UniformFloat> inverseBeta_;
  ref_ptr<UniformFloat> halfCell_;
  ref_ptr<UniformFloat> densityInverse_;

  FluidBuffer velocityBuffer_;
  FluidBuffer pressureBuffer_;
  FluidBuffer divergenceBuffer_;
  ref_ptr<Texture> obstaclesTexture_;
  ref_ptr<Texture> levelSetTexture_;
};


#endif /* EULERIAN_PRESSURE_H_ */
