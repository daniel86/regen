/*
 * discard-rasterizer.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef __DISCARD_RASTERIZE_H_
#define __DISCARD_RASTERIZE_H_

#include <ogle/states/state.h>

class DiscardRasterizer : public State
{
public:
  DiscardRasterizer();
  // override
  virtual void enable(RenderState *state);
  virtual void disable(RenderState *state);
};

#endif /* __POLYGON_OFFSET_STATE_H_ */
