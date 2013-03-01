/*
 * point-light.h
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#ifndef __SHADING_POINT_LIGHT_H_
#define __SHADING_POINT_LIGHT_H_

#include <ogle/shading/deferred-light.h>

/**
 * Renders diffuse and specular color for point lights.
 */
class DeferredPointLight : public DeferredLight
{
public:
  DeferredPointLight();
  void createShader(const ShaderConfig &cfg);

  // override
  virtual void enable(RenderState *rs);

protected:
  GLint posLoc_;
  GLint radiusLoc_;
  GLint diffuseLoc_;
  GLint specularLoc_;
  GLint shadowFarLoc_;
  GLint shadowNearLoc_;
};

#endif /* __SHADING_POINT_LIGHT_H_ */
