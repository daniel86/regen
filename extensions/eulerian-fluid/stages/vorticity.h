
#ifndef EULERIAN_EULERIAN_VORTICITY_H_
#define EULERIAN_EULERIAN_VORTICITY_H_

#include "primitive.h"
#include "helper.h"

/**
 * The motion of smoke, air and other low-viscosity fluids typically contains rotational flows at a variety of scales.
 * This rotational flow is vorticity. As Fedkiw et al. explained, numerical dissipation caused by simulation on
 * a coarse grid damps out these interesting features (Fedkiw et al. 2001).
 * Therefore, vorticity confinement is used to restore these fine-scale motions.
 */
class EulerianVorticity : public EulerianStage
{
public:
  static string vorticityTextureName_;

  EulerianVorticity(EulerianPrimitive*);

  void set_vorticityBuffer(const FluidBuffer &buffer);
  void set_velocityBuffer(const FluidBuffer &buffer);
  void set_obstaclesTexture(ref_ptr<Texture> tex);

  void set_vorticityConfinementScale(float scale) {
    vortConfinementScale_->set_value(scale);
  }
  float vorticityConfinementScale() const {
    return vortConfinementScale_->value();
  }

  virtual void update();

protected:
  ref_ptr<Shader> confinementShader_;
  ref_ptr<Shader> computeShader_;
  ref_ptr<UniformFloat> vortConfinementScale_;

  FluidBuffer vorticityBuffer_;
  FluidBuffer velocityBuffer_;
  ref_ptr<Texture> obstaclesTexture_;
};

#endif /* EULERIAN_EULERIAN_VORTICITY_H_ */
