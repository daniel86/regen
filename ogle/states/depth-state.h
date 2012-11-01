/*
 * depth-screen.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef __DEPTH_STATE_H_
#define __DEPTH_STATE_H_

#include <ogle/states/state.h>

class DepthState : public State
{
public:
  DepthState();

  /**
   * Enable or disable depth testing with this state.
   */
  void set_useDepthTest(GLboolean useDepthTest);
  /**
   * Enable or disable depth writing with this state.
   */
  void set_useDepthWrite(GLboolean useDepthTest);
  /**
   * Specifies the depth comparison function. Symbolic constants
   * GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER,
   * GL_NOTEQUAL, GL_GEQUAL, and GL_ALWAYS are accepted.
   * The initial value is GL_LESS.
   */
  void set_depthFunc(GLenum depthFunc=GL_LESS);
  /**
   * specify mapping of depth values from normalized device coordinates to window coordinates.
   * nearVal specifies the mapping of the near clipping plane to window coordinates. The initial value is 0.
   * farVal specifies the mapping of the far clipping plane to window coordinates. The initial value is 1.
   */
  void set_depthRange(GLdouble nearVal=0.0, GLdouble farVal=1.0);
  /**
   * set the scale and units used to calculate depth values.
   * factor specifies a scale factor that is used to create a variable
   * depth offset for each polygon. The initial value is 0.
   * units is multiplied by an implementation-specific value to
   * create a constant depth offset. The initial value is 0.
   */
  void set_polygonOffset(GLfloat factor=0.0f, GLfloat units=0.0f);
protected:
  ref_ptr<State> depthTestToggle_;
  ref_ptr<State> depthWriteToggle_;
  ref_ptr<State> polygonOffset_;
  ref_ptr<State> depthRange_;
  ref_ptr<State> depthFunc_;
};

#endif /* BLIT_TO_SCREEN_H_ */
