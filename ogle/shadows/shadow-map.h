/*
 * shadow-map.h
 *
 *  Created on: 24.11.2012
 *      Author: daniel
 */

#ifndef SHADOW_MAP_H_
#define SHADOW_MAP_H_

#include <ogle/animations/animation.h>
#include <ogle/states/camera.h>
#include <ogle/render-tree/state-node.h>

class ShadowMap : public Animation
{
public:
  ShadowMap();

  void addCaster(ref_ptr<StateNode> &caster);
  void removeCaster(StateNode *caster);

  // override
  virtual void animate(GLdouble dt);

protected:
  list< ref_ptr<StateNode> > caster_;
};

#endif /* SHADOW_MAP_H_ */
