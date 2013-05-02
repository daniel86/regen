/*
 * interfaces.h
 *
 *  Created on: 10.03.2013
 *      Author: daniel
 */

#ifndef INTERFACES_H_
#define INTERFACES_H_

#include <regen/states/shader-state.h>

namespace regen {
/**
 * \brief interface for resizable objects.
 */
class Resizable {
public:
  virtual ~Resizable() {}
  /**
   * Resize buffers / textures.
   */
  virtual void resize()=0;
};
} // namespace

namespace regen {
/**
 * \brief can be used to mix in a shader.
 */
class HasShader {
public:
  /**
   * @param shaderKey the shader include key
   */
  HasShader(const string &shaderKey)
  : shaderKey_(shaderKey)
  { shaderState_ = ref_ptr<ShaderState>::manage(new ShaderState); }

  /**
   * @param cfg the shader configuration.
   */
  virtual void createShader(const ShaderState::Config &cfg)
  { shaderState_->createShader(cfg,shaderKey_); }

  /**
   * @return the shader state.
   */
  const ref_ptr<ShaderState>& shaderState() const
  { return shaderState_; }
  /**
   * @return the shader include key.
   */
  const string& shaderKey() const
  { return shaderKey_; }

protected:
  ref_ptr<ShaderState> shaderState_;
  string shaderKey_;
};

} // namespace

#endif /* INTERFACES_H_ */
