/*
 * fluid-animation.h
 *
 *  Created on: 11.10.2012
 *      Author: daniel
 */

#ifndef FLUID_ANIMATION_H_
#define FLUID_ANIMATION_H_

#include <ogle/animations/animation.h>

#include "fluid.h"

/**
 * An animation that updates a fluid simulation.
 */
class FluidAnimation : public Animation
{
public:
  FluidAnimation(Fluid *fluid, GLuint framerate);

  virtual void animate(GLdouble dt);
  virtual void updateGraphics(GLdouble dt);

protected:
  Fluid *fluid_;
  GLdouble framerate_;
  GLdouble dt_;
};


#endif /* FLUID_ANIMATION_H_ */
