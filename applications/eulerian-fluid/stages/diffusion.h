
#ifndef EULERIAN_DIFFUSION_H_
#define EULERIAN_DIFFUSION_H_

#include "primitive.h"
#include "helper.h"

typedef struct {
  FluidBuffer buffer;
  GLuint numIterations;
  float viscosity;
}DiffusionTarget;

class EulerianDiffusion : public EulerianStage
{
public:
  EulerianDiffusion(EulerianPrimitive*);

  void addDiffusionTarget(ref_ptr<DiffusionTarget> tex);
  void removeDiffusionTarget(ref_ptr<DiffusionTarget> tex);

  virtual void update();

protected:
  list< ref_ptr<DiffusionTarget> > diffusionTargets_;
  ref_ptr<Shader> diffusionShader_;
  GLuint initialTexLoc_;
  GLuint currentTexLoc_;
  GLuint viscosityLoc_;
  FluidBuffer tmpBuffer_;
};

#endif /* EULERIAN_ADVECT_H_ */
