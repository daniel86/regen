/*
 * spot-light.h
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#ifndef __SHADING_SPOT_LIGHT_H_
#define __SHADING_SPOT_LIGHT_H_

#include <ogle/shading/deferred-light.h>

namespace ogle {

/**
 * Renders diffuse and specular color for spot lights.
 */
class DeferredSpotLight : public DeferredLight
{
public:
  DeferredSpotLight();
  void createShader(const ShaderConfig &cfg);

  // override
  virtual void enable(RenderState *rs);

protected:
  GLint dirLoc_;
  GLint posLoc_;
  GLint radiusLoc_;
  GLint diffuseLoc_;
  GLint specularLoc_;
  GLint coneAnglesLoc_;
  GLint coneMatLoc_;
  GLint shadowMatLoc_;
  GLint shadowNearLoc_;
  GLint shadowFarLoc_;
};

} // end ogle namespace

#endif /* __SHADING_SPOT_LIGHT_H_ */
