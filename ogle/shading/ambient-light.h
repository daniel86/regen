/*
 * ambient-light.h
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#ifndef __SHADING_AMBIENT_LIGHT_H_
#define __SHADING_AMBIENT_LIGHT_H_

#include <ogle/states/state.h>
#include <ogle/states/shader-state.h>

namespace ogle {

/**
 * Renders ambient light for use with deferred shading.
 */
class DeferredAmbientLight : public State
{
public:
  DeferredAmbientLight();
  void createShader(ShaderConfig &cfg);

  /**
   * The ambient light that is multiplied with GBuffer diffuse color.
   */
  const ref_ptr<ShaderInput3f>& ambientLight() const;

protected:
  ref_ptr<ShaderState> shader_;
  ref_ptr<ShaderInput3f> ambientLight_;
};

} // end ogle namespace

#endif /* __SHADING_AMBIENT_LIGHT_H_ */
