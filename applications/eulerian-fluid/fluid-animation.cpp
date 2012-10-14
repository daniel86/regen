/*
 * fluid-animation.cpp
 *
 *  Created on: 11.10.2012
 *      Author: daniel
 */

#include "fluid-animation.h"

FluidAnimation::FluidAnimation(Fluid *fluid)
: Animation(),
  fluid_(fluid),
  dt_(0.0)
{
}


void FluidAnimation::set_fluid(Fluid *fluid)
{
  fluid_ = fluid;
  dt_ = 0.0;
}

void FluidAnimation::animate(GLdouble dt)
{
}
void FluidAnimation::updateGraphics(GLdouble dt)
{
  GLdouble framerate = 1000.0/(double)fluid_->framerate();
  dt_ += dt;
  if(dt_ > framerate) {
    dt_ = 0.0;
    fluid_->executeOperations(fluid_->operations());
  }
}

