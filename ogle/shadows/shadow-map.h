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

class ShadowMap : public Animation, public State
{
public:
  ShadowMap();

  /**
   * Offset the geometry slightly to prevent z-fighting
   * Note that this introduces some light-leakage artifacts.
   */
  void setPolygonOffset(GLfloat factor=1.1, GLfloat units=4096.0);
  /**
   * Moves shadow acne to back faces. But it results in light bleeding
   * artifacts for some models.
   */
  void setCullFrontFaces(GLboolean v=GL_TRUE);

  void addCaster(ref_ptr<StateNode> &caster);
  void removeCaster(StateNode *caster);

  /**
   * Traverse all added shadow caster.
   */
  void traverse();
  virtual void updateShadow()=0;

  // override
  virtual void animate(GLdouble dt);
  virtual void updateGraphics(GLdouble dt);

protected:
  list< ref_ptr<StateNode> > caster_;
  ref_ptr<State> cullState_;
  ref_ptr<State> polygonOffsetState_;
};

#endif /* SHADOW_MAP_H_ */
