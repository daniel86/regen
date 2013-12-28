/*
 * direct-shading.h
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#ifndef __SHADING_DIRECT_H_
#define __SHADING_DIRECT_H_

#include <regen/states/state.h>
#include <regen/states/light-state.h>
#include <regen/states/texture-state.h>
#include <regen/camera/light-camera.h>

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
     * @param light a light.
     */
    void addLight(const ref_ptr<Light> &light);
    /**
     * @param light a light.
     * @param lightCamera Light-perspective Camera or null reference.
     * @param shadow ShadowMap or null reference.
     * @param shadowColor Color-ShadowMap or null reference.
     * @param shadowFilter shadow filtering mode that should be used.
     */
    void addLight(
        const ref_ptr<Light> &light,
        const ref_ptr<LightCamera> &lightCamera,
        const ref_ptr<Texture> &shadow,
        const ref_ptr<Texture> &shadowColor,
        ShadowFilterMode shadowFilter);
    /**
     * @param l remove previously added light.
     */
    void removeLight(const ref_ptr<Light> &l);

  protected:
    GLint idCounter_;

    struct DirectLight {
      GLuint id_;
      ref_ptr<Light> light_;
      ref_ptr<LightCamera> camera_;
      ref_ptr<Texture> shadow_;
      ref_ptr<Texture> shadowColor_;
      ref_ptr<TextureState> shadowMap_;
      ref_ptr<TextureState> shadowColorMap_;
      ShadowFilterMode shadowFilter_;
    };
    list<DirectLight> lights_;
    ref_ptr<ShaderInput3f> ambientLight_;

    void updateDefine(DirectLight &l, GLuint lightIndex);
  };
} // namespace

#endif /* __SHADING_DIRECT_H_ */
