/*
 * pixel-velocity.h
 *
 *  Created on: 18.07.2012
 *      Author: daniel
 */

#ifndef PIXEL_VELOCITY_H_
#define PIXEL_VELOCITY_H_

#include <ogle/states/shader-state.h>

typedef enum {
  // .. transformed to global coordinate system
  WORLD_SPACE,
  // .. and transformed by camera matrix
  EYE_SPACE,
  // .. and transformed by projection matrix
  SCREEN_SPACE
}CoordinateSpace;

/**
 * Generates velocity texture for camera perspective.
 * The calculation is done using transform feedback
 * of the position attribute in a configurable space
 * (world/eye/screen).
 */
class PixelVelocity : public ShaderState
{
public:
  PixelVelocity(
      CoordinateSpace velocitySpace,
      GLboolean useDepthTestFS=false,
      GLfloat depthBias=0.01f);

  virtual string name();

protected:
  CoordinateSpace velocitySpace_;
};

#endif /* PIXEL_VELOCITY_H_ */
