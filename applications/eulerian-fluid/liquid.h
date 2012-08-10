
#ifndef EULERIAN_GRID_LIQUID_H_
#define EULERIAN_GRID_LIQUID_H_

#include "fluid.h"
#include "stages.h"

/**
 * From GPU Gems3:
 * With fire or smoke, we are interested in visualizing a density defined throughout the entire volume,
 * but with water the visually interesting part is the interface between air and liquid.
 *
 * Therefore, we need some way of representing this interface and tracking how it deforms
 * as it is pushed around by the fluid velocity.
 * The level set method (Sethian 1999) is a popular representation of a liquid surface and is
 * particularly well suited to a GPU implementation because it requires only a scalar value
 * at each grid cell. In a level set, each cell records the shortest signed distance phi from the
 * cell center to the water surface. Cells in the grid are classified according to the value of
 * phi: if phi < 0, the cell contains water; otherwise, it contains air. Wherever phi equals zero is
 * exactly where the water meets the air (the zero set). Because advection will not preserve
 * the distance field property of a level set, it is common to periodically reinitialize the
 * level set. Reinitialization ensures that each cell does indeed store the shortest distance to
 * the zero set. However, this property isn't needed to simply define the surface, and for
 * real-time animation, it is possible to get decent results without reinitialization.
 * Figure 30-1, at the beginning of this chapter, shows the quality of the results.
 * Just as with color, temperature, and other attributes, the level set is advected by the fluid,
 * but it also affects the simulation dynamics.
 * In fact, the level set defines the fluid domain:
 * in simple models of water and air, we assume that the air has a negligible effect on the
 * liquid and do not perform simulation wherever phi >= 0. In practice, this means we set the
 * pressure outside of the liquid to zero before solving for pressure and modify the pressure
 * only in liquid cells. It also means that we do not apply external forces such as gravity
 * outside of the liquid. To make sure that only fluid cells are processed, we check the value
 * of the level set texture in the relevant shaders and mask computations at a cell if the
 * value of phi is above some threshold. Two alternatives that may be more efficient are to use
 * z-cull to eliminate cells (if the GPU does not support dynamic flow control) (Sander et
 * al. 2004) and to use a sparse data structure (Lefohn et al. 2004).
 *
 * To render a liquid surface, we also march through a volume, but this time we look at
 * values from the level set phi. Instead of integrating values as we march, we look for the
 * first place along the ray where phi = 0. Once this point is found, we shade it just as we
 * would shade any other surface fragment, using NABLA * phi at that point to approximate the
 * shading normal. For water, it is particularly important that we do not see artifacts of
 * the grid resolution, so we use tricubic interpolation to filter these values. Figure 30-1 at
 * the beginning of the chapter demonstrates the rendered results. See Sigg and Hadwiger
 * 2005 and Hadwiger et al. 2005 for details on how to quickly intersect and filter volume
 * isosurface data such as a level set on the GPU.
 *
 */
class EulerianLiquid : public EulerianFluid
{
public:
  static const string LEVEL_SET;

  EulerianLiquid(
      ref_ptr<EulerianPrimitive> primitive,
      ref_ptr<EulerianObstacles> obstacles);

  void set_liquidHeight(float height) {
    liquidHeight_->set_value(height);
  }
  float liquidHeight() const {
    return liquidHeight_->value();
  }

  /**
   * Overwrites default shader.
   */
  void set_distanceToLiquidHeightShader(ref_ptr<Shader> distanceToLiquidHeightShader) {
    distanceToLiquidHeightShader_ = distanceToLiquidHeightShader;
    distanceToLiquidHeightShader_->addUniform( liquidHeight_.get() );
    reset();
  }

  EulerianRedistance& redistancelevelSetStage() {
    return *redistancelevelSetStage_.get();
  }
  EulerianExtrapolateVelocity& extrapolateVelocityStage() {
    return *extrapolateVelocityStage_.get();
  }
  EulerianLiquidStream& liquidStreamStage() {
    return *liquidStreamStage_.get();
  }

  AdvectionTarget& levelSetAdvectionTarget() {
    return *levelSetAdvectionTarget_.get();
  }

  FluidBuffer levelSetBuffer() {
    return levelSetBuffer_;
  }

  virtual void reset();

protected:
  FluidBuffer levelSetBuffer_;
  ref_ptr<Texture> initialLevelSetTexture_;
  GLint initialLevelSetAttachment_;

  ref_ptr<AdvectionTarget> levelSetAdvectionTarget_;

  ref_ptr<Shader> distanceToLiquidHeightShader_;
  ref_ptr<UniformFloat> liquidHeight_;

  ref_ptr<EulerianRedistance> redistancelevelSetStage_;
  ref_ptr<EulerianExtrapolateVelocity> extrapolateVelocityStage_;
  ref_ptr<EulerianLiquidStream> liquidStreamStage_;
  ref_ptr<EulerianStage> injectLiquidStage_;


  void resetLevelSet();
  virtual void setDefaultStages();
};

#endif /* EULERIAN_GRID_LIQUID_H_ */
