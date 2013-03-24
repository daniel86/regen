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

namespace ogle {
/**
 * \brief State that updates each texel of the active render target.
 */
class FullscreenPass : public State
{
public:
  /**
   * @param shaderKey the key will be imported when createShader is called.
   */
  FullscreenPass(const string &shaderKey) : State()
  {
    shaderKey_ = shaderKey;
    shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
    joinStates(ref_ptr<State>::cast(shader_));

    joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
  }

  /**
   * @param cfg the shader config.
   */
  void createShader(ShaderState::Config &cfg)
  { shader_->createShader(cfg, shaderKey_); }

protected:
  ref_ptr<ShaderState> shader_;
  string shaderKey_;
};
} // namespace
#endif /* FULLSCREEN_PASS_H_ */
