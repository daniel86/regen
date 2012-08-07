
#ifndef EULERIAN_REDISTANCE_H_
#define EULERIAN_REDISTANCE_H_

#include "primitive.h"
#include "helper.h"

/**
 * For liquids, on the other hand, a change of volume is immediately apparent:
 * fluid appears to either pour out from nowhere or disappear entirely! Even something as simple as water
 * sitting in a tank can potentially be problematic if too few iterations are used to solve for pressure:
 * because information does not travel from the tank floor to the water surface, pressure from the floor
 * cannot counteract the force of gravity. As a result,
 * the water slowly sinks through the bottom of the tank
 */
class EulerianRedistance : public EulerianStage
{
public:
  EulerianRedistance(EulerianPrimitive*);

  void set_levelSetBuffer(const FluidBuffer &buffer);
  void set_initialLevelSetTexture(ref_ptr<Texture> tex);

  void set_numRedistanceIterations(GLuint numRedistanceIterations) {
    numRedistanceIterations_ = numRedistanceIterations;
  }
  GLuint numRedistanceIterations() const {
    return numRedistanceIterations_;
  }

  virtual void update();

protected:
  ref_ptr<Shader> redistanceShader_;
  GLuint levelSetCurrentLoc_;
  GLuint initialLevelSetLoc_;

  FluidBuffer levelSetBuffer_;
  ref_ptr<Texture> initialLevelSetTexture_;
  GLint initialLevelSetAttachment_;

  GLuint numRedistanceIterations_;

  FluidBuffer tmpBuffer_;
};

#endif /* EULERIAN_ADVECT_H_ */
