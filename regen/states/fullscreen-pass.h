/*
 * fullscreen-pass.h
 *
 *  Created on: 09.03.2013
 *      Author: daniel
 */

#ifndef FULLSCREEN_PASS_H_
#define FULLSCREEN_PASS_H_

#include <regen/states/state.h>
#include <regen/states/shader-state.h>
#include <regen/meshes/rectangle.h>
#include <regen/utility/interfaces.h>

namespace regen {
/**
 * \brief State that updates each texel of the active render target.
 */
class FullscreenPass : public State, public HasShader
{
public:
  /**
   * @param shaderKey the key will be imported when createShader is called.
   */
  FullscreenPass(const string &shaderKey) : State(), HasShader(shaderKey)
  {
    joinStates(ref_ptr<State>::cast(shaderState_));
    joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
  }
};
} // namespace
#endif /* FULLSCREEN_PASS_H_ */
