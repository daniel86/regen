/*
 * depth-screen.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef __DEPTH_STATE_H_
#define __DEPTH_STATE_H_

#include <regen/states/atomic-states.h>

namespace regen {
  /**
   * \brief Allows manipulating how the depth buffer is handled.
   */
  class DepthState : public ServerSideState
  {
  public:
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

  protected:
    ref_ptr<State> depthTestToggle_;
    ref_ptr<State> depthWriteToggle_;
    ref_ptr<State> depthRange_;
    ref_ptr<State> depthFunc_;
  };
} // namespace

#endif /* BLIT_TO_SCREEN_H_ */
