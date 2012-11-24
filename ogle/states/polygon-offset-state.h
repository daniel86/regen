/*
 * polygon-offset-state.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef __POLYGON_OFFSET_STATE_H_
#define __POLYGON_OFFSET_STATE_H_

#include <ogle/states/state.h>

/**
 * set the scale and units used to calculate depth values.
 * factor specifies a scale factor that is used to create a variable
 * depth offset for each polygon. The initial value is 0.
 * units is multiplied by an implementation-specific value to
 * create a constant depth offset. The initial value is 0.
  */
class PolygonOffsetState : public State
{
public:
  PolygonOffsetState(GLfloat factor, GLfloat units);
  virtual void enable(RenderState *state);
  virtual void disable(RenderState *state);
protected:
  GLfloat factor_, units_;
};

#endif /* __POLYGON_OFFSET_STATE_H_ */
