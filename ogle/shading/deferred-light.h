/*
 * deferred-light.h
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#ifndef __SHADING_DEFERRED_LIGHT_H_
#define __SHADING_DEFERRED_LIGHT_H_

#include <ogle/shading/shadow-map.h>

#include <ogle/states/state.h>

namespace ogle {

/**
 * Base class for deferred lights.
 */
class DeferredLight : public State
{
public:
  DeferredLight();
  virtual void createShader(const ShaderState::Config &cfg) = 0;

  GLboolean empty() const;
  GLboolean hasLight(Light *l) const;

  void setShadowFiltering(ShadowMap::FilterMode mode);

  void addLight(const ref_ptr<Light> &l, const ref_ptr<ShadowMap> &sm);
  void removeLight(Light *l);

protected:
  ref_ptr<MeshState> mesh_;

  ref_ptr<ShaderState> shader_;
  GLint shadowMapLoc_;

  ShadowMap::FilterMode shadowFiltering_;

  struct DLight {
    DLight(
        const ref_ptr<Light> &light,
        const ref_ptr<ShadowMap> &shadowMap)
    : l(light), sm(shadowMap)
    {}
    ref_ptr<Light> l;
    ref_ptr<ShadowMap> sm;
  };
  list<DLight> lights_;
  map< Light*, list<DLight>::iterator > lightIterators_;


  void activateShadowMap(ShadowMap *sm, GLuint channel);
};

string glsl_shadowFilterMode(ShadowMap::FilterMode f);
GLboolean glsl_useShadowMoments(ShadowMap::FilterMode f);
GLboolean glsl_useShadowSampler(ShadowMap::FilterMode f);

} // end ogle namespace

#endif /* __SHADING_DEFERRED_LIGHT_H_ */
