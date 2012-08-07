
#ifndef EULERIAN_ADVECT_H_
#define EULERIAN_ADVECT_H_

#include "primitive.h"
#include "helper.h"

typedef struct {
  FluidBuffer buffer;
  float decayAmount;
  float decayAmountUnnormalized;
  float quantityLoss;
  bool treatAsLiquid;
}AdvectionTarget;

/**
 * The velocity of a fluid causes the fluid to transport objects, densities, and other quantities along with the flow.
 * Imagine squirting dye into a moving fluid. The dye is transported, or advected, along the fluid's velocity field.
 * In fact, the velocity of a fluid carries itself along just as it carries the dye.
 *
 * Instead of moving the cell centers forward in time through the velocity field, we look for the particles which
 * end up exactly at the cell centers by tracing backwards in time from the cell centers.
 */
class EulerianAdvection : public EulerianStage
{
public:
  EulerianAdvection(EulerianPrimitive *primitive);

  bool useMacCormack() const {
    return useMacCormack_;
  }
  void set_useMacCormack(bool useMacCormack) {
    useMacCormack_ = useMacCormack;
  }

  void set_velocityTexture(ref_ptr<Texture> tex);
  void set_obstaclesTexture(ref_ptr<Texture> tex);
  void set_levelSetTexture(ref_ptr<Texture> tex);

  void addAdvectionTarget(ref_ptr<AdvectionTarget> tex);
  void removeAdvectionTarget(ref_ptr<AdvectionTarget> tex);

  virtual void update();

  void advect();
  void advectMacCormack();

protected:
  bool useMacCormack_;

  FluidBuffer tmpBuffer_;

  ref_ptr<Texture> velocityTexture_;
  ref_ptr<Texture> obstaclesTexture_;
  ref_ptr<Texture> levelSetTexture_;

  list< ref_ptr<AdvectionTarget> > advectionTagets_;

  ref_ptr<Shader> advectShader_;
  GLuint decayAmountLoc_;
  GLuint quantityLossLoc_;
  GLuint advectSourceLoc_;
  GLuint treatAsLiquidLoc_;

  ref_ptr<Shader> advectMacCormackShader_;
  GLuint decayAmountLoc2_;
  GLuint quantityLossLoc2_;
  GLuint advectSourceLoc2_;
  GLuint advectSourceHatLoc2_;
  GLuint treatAsLiquidLoc2_;
};

#endif /* EULERIAN_ADVECT_H_ */
