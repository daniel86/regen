/*
 * direct.h
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#ifndef __SHADING_DIRECT_H_
#define __SHADING_DIRECT_H_

#include <ogle/states/state.h>
#include <ogle/shading/light-state.h>
#include <ogle/shading/shadow-map.h>

namespace ogle {

/**
 * Regular direct shading where the shading computation
 * is done in the same shader as the geometry is processed.
 */
class DirectShading : public State
{
public:
  DirectShading();

  void addLight(const ref_ptr<Light> &l);
  void addLight(
      const ref_ptr<Light> &l,
      const ref_ptr<ShadowMap> &sm,
      ShadowMap::FilterMode shadowFilter);

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

  void updateDefine(DirectLight &l, GLuint lightIndex);
};

} // end ogle namespace

#endif /* __SHADING_DIRECT_H_ */
