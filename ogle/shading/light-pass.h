/*
 * light-pass.h
 *
 *  Created on: 13.03.2013
 *      Author: daniel
 */

#ifndef __LIGHT_PASS_H_
#define __LIGHT_PASS_H_

#include <ogle/states/state.h>
#include <ogle/states/shader-state.h>
#include <ogle/meshes/mesh-state.h>
#include <ogle/shading/shadow-map.h>

namespace ogle {
/**
 * \brief Deferred shading pass.
 */
class LightPass : public State
{
public:
  /**
   * \brief defines the light type
   */
  enum LightType {
    DIRECTIONAL,//!< directional light
    SPOT,       //!< spot light
    POINT       //!< point light
  };

  /**
   * @param type the light type.
   * @param shaderKey the shader key to include.
   */
  LightPass(LightType type, const string &shaderKey);
  /**
   * @param cfg the shader configuration.
   */
  void createShader(const ShaderState::Config &cfg);

  /**
   * Adds a light to the rendering pass.
   * @param l the light.
   * @param sm shadow map associated to light.
   * @param inputs render pass inputs.
   */
  void addLight(
      const ref_ptr<Light> &l,
      const ref_ptr<ShadowMap> &sm,
      const list< ref_ptr<ShaderInput> > &inputs);
  /**
   * @param l a previously added light.
   */
  void removeLight(Light *l);
  /**
   * @return true if no light was added yet.
   */
  GLboolean empty() const;
  /**
   * @param l a light.
   * @return true if the light was previously added.
   */
  GLboolean hasLight(Light *l) const;

  /**
   * @param mode the shadow filtering mode.
   */
  void setShadowFiltering(ShadowMap::FilterMode mode);
  /**
   * @param numLayer number of shadow texture layers.
   */
  void setShadowLayer(GLuint numLayer);

  // override
  void enable(RenderState *rs);

protected:
  struct LightPassLight {
    ref_ptr<Light> light;
    ref_ptr<ShadowMap> sm;
    list< ref_ptr<ShaderInput> > inputs;
    list< ShaderInputLocation > inputLocations;
  };

  LightType lightType_;
  const string shaderKey_;

  ref_ptr<MeshState> mesh_;
  ref_ptr<ShaderState> shader_;

  list<LightPassLight> lights_;
  map< Light*, list<LightPassLight>::iterator > lightIterators_;

  GLint shadowMapLoc_;
  ShadowMap::FilterMode shadowFiltering_;
  GLuint numShadowLayer_;

  void addInputLocation(LightPassLight &l,
      const ref_ptr<ShaderInput> &in, const string &name);
};
} // namespace

#endif /* __LIGHT_PASS_H_ */
