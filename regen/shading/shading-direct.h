/*
 * direct.h
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#ifndef __SHADING_DIRECT_H_
#define __SHADING_DIRECT_H_

#include <regen/states/state.h>
#include <regen/shading/light-state.h>
#include <regen/shading/shadow-map.h>

namespace regen {
/**
 * \brief Implements deferred shading.
 *
 * Geometry processing and shading calculation is coupled in
 * direct shading.
 */
class DirectShading : public State
{
public:
  DirectShading();

  /**
   * @return the ambient light.
   */
  const ref_ptr<ShaderInput3f>& ambientLight() const;
  /**
   * @param l a light.
   */
  void addLight(const ref_ptr<Light> &l);
  /**
   * @param l a light.
   * @param sm a shadow map.
   * @param shadowFilter shadow filtering mode that should be used.
   */
  void addLight(
      const ref_ptr<Light> &l,
      const ref_ptr<ShadowMap> &sm,
      ShadowMap::FilterMode shadowFilter);
  /**
   * @param l remove previously added light.
   */
  void removeLight(const ref_ptr<Light> &l);

protected:
  GLint idCounter_;

  struct DirectLight {
    GLuint id_;
    ref_ptr<Light> light_;
    ref_ptr<ShadowMap> sm_;
    ref_ptr<TextureState> shadowMap_;
    ShadowMap::FilterMode shadowFilter_;
  };
  list<DirectLight> lights_;
  ref_ptr<ShaderInput3f> ambientLight_;

  void updateDefine(DirectLight &l, GLuint lightIndex);
};
} // namespace

#endif /* __SHADING_DIRECT_H_ */
