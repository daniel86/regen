/*
 * direct.h
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#ifndef __SHADING_DIRECT_H_
#define __SHADING_DIRECT_H_

#include <ogle/states/state.h>
#include <ogle/states/light-state.h>

/**
 * Regular direct shading where the shading computation
 * is done in the same shader as the geometry is processed.
 */
class DirectShading : public State
{
public:
  DirectShading();

  virtual void addLight(const ref_ptr<Light> &l);
  virtual void removeLight(const ref_ptr<Light> &l);

protected:
  list< ref_ptr<Light> > lights_;
};

#endif /* __SHADING_DIRECT_H_ */
