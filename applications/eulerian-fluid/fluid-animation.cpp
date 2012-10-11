/*
 * fluid-animation.cpp
 *
 *  Created on: 11.10.2012
 *      Author: daniel
 */

#include "fluid-animation.h"

FluidAnimation::FluidAnimation(Fluid *fluid, GLuint framerate)
: Animation(),
  fluid_(fluid),
  framerate_(1.0/(double)framerate),
  dt_(framerate_+1.0)
{
}

void FluidAnimation::animate(GLdouble dt)
{
}
void FluidAnimation::updateGraphics(GLdouble dt)
{
  dt_ += dt;
  if(dt_ > framerate_) {
    dt_ = 0.0;
    fluid_->executeOperations(fluid_->operations());
  }
}

