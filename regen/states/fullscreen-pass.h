/*
 * fullscreen-pass.h
 *
 *  Created on: 09.03.2013
 *      Author: daniel
 */

#ifndef FULLSCREEN_PASS_H_
#define FULLSCREEN_PASS_H_

#include <regen/states/state.h>
#include <regen/states/state-node.h>
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
    fullscreenMesh_ = Rectangle::getUnitQuad();
    joinStates(shaderState_);
    joinStates(fullscreenMesh_);
  }
  /**
   * @param cfg the shader configuration.
   */
  void createShader(Config &cfg)
  {
    shaderState_->createShader(cfg,shaderKey_);
    fullscreenMesh_->updateVAO(RenderState::get(), cfg, shaderState_->shader());
  }
protected:
  ref_ptr<Mesh> fullscreenMesh_;
};
} // namespace
#endif /* FULLSCREEN_PASS_H_ */
