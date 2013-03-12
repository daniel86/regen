/*
 * directional-light.h
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#ifndef __SHADING_DIRECTIONAL_LIGHT_H_
#define __SHADING_DIRECTIONAL_LIGHT_H_

#include <ogle/shading/deferred-light.h>

namespace ogle {

/**
 * Renders diffuse and specular color for directional lights.
 */
class DeferredDirLight : public DeferredLight
{
public:
  DeferredDirLight();
  void createShader(const ShaderConfig &cfg);

  GLuint numShadowLayer() const;
  void set_numShadowLayer(GLuint numLayer);

  // override
  virtual void enable(RenderState *rs);
  virtual void addLight(const ref_ptr<Light> &l, const ref_ptr<ShadowMap> &sm);

protected:
  GLuint numShadowLayer_;
  GLint dirLoc_;
  GLint diffuseLoc_;
  GLint specularLoc_;
  GLint shadowMatricesLoc_;
  GLint shadowFarLoc_;
};

} // end ogle namespace

#endif /* __SHADING_DIRECTIONAL_LIGHT_H_ */
