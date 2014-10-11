/*
 * star-map-layer.h
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#ifndef STAR_MAP_LAYER_H_
#define STAR_MAP_LAYER_H_

#include <regen/sky/sky-layer.h>
#include <regen/sky/sky.h>
#include <regen/gl-types/fbo.h>

namespace regen {
  class StarMapLayer : public SkyLayer {
  public:
    StarMapLayer(const ref_ptr<Sky> &sky);

    void set_texture(const string &textureFile);

    void set_scattering(GLdouble scattering);

    const ref_ptr<ShaderInput1f>& scattering() const;

    void set_apparentMagnitude(GLdouble apparentMagnitude);

    const ref_ptr<ShaderInput1f>& apparentMagnitude() const;

    void set_colorRatio(GLdouble colorRatio);

    const ref_ptr<ShaderInput1f>& colorRatio() const;

    void set_color(const Vec3f &color);

    const ref_ptr<ShaderInput3f>& color() const;


    const GLdouble defaultScattering();

    const GLdouble defaultApparentMagnitude();

    const GLdouble defaultColorRatio();

    const Vec3f defaultColor();

    // Override
    virtual void updateSkyLayer(RenderState *rs, GLdouble dt);
    ref_ptr<Mesh> getMeshState();
    ref_ptr<HasShader> getShaderState();

  protected:
    ref_ptr<Mesh> meshState_;
    ref_ptr<HasShader> shaderState_;

    ref_ptr<TextureState> starMap_;

    ref_ptr<ShaderInput1f> scattering_;
    ref_ptr<ShaderInput1f> apparentMagnitude_;
    ref_ptr<ShaderInput1f> colorRatio_;
    ref_ptr<ShaderInput3f> color_;
    ref_ptr<ShaderInput1f> deltaM_;

    ref_ptr<FBO> fbo_;
    ref_ptr<Texture2D> cloudTexture_;
    ref_ptr<Texture3D> noise0_;
    ref_ptr<Texture3D> noise1_;
    ref_ptr<Texture3D> noise2_;
    ref_ptr<Texture3D> noise3_;
  };
}
#endif /* STAR_MAP_LAYER_H_ */
