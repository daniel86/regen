/*
 * stars.h
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#ifndef STARS_LAYER_H_
#define STARS_LAYER_H_

#include <regen/sky/sky-layer.h>
#include <regen/sky/sky.h>
#include <regen/gl-types/fbo.h>

namespace regen {
  class Stars : public SkyLayer {
  public:
    Stars(const ref_ptr<Sky> &sky);

    void set_brightStarsFile(const string &brightStars);

    void set_apparentMagnitude(const GLfloat vMag);

    const ref_ptr<ShaderInput1f>& apparentMagnitude() const;

    void set_color(const Vec3f color);

    const ref_ptr<ShaderInput3f>& color() const;

    void set_colorRatio(const GLfloat ratio);

    const ref_ptr<ShaderInput1f>& colorRatio() const;

    void set_glareIntensity(const GLfloat intensity);

    const ref_ptr<ShaderInput1f>& glareIntensity() const;

    void set_glareScale(const GLfloat scale);

    const ref_ptr<ShaderInput1f>& glareScale() const;

    void set_scintillation(const GLfloat scintillation);

    const ref_ptr<ShaderInput1f>& scintillation() const;

    void set_scattering(const GLfloat scattering);

    const ref_ptr<ShaderInput1f>& scattering() const;

    void set_scale(const GLfloat scale);

    const ref_ptr<ShaderInput1f>& scale() const;


    GLfloat defaultApparentMagnitude();

    Vec3f defaultColor();

    GLfloat defaultColorRatio();

    GLfloat defaultGlareScale();

    GLfloat defaultScintillation();

    GLfloat defaultScattering();

    // Override
    virtual void updateSkyLayer(RenderState *rs, GLdouble dt);
    ref_ptr<Mesh> getMeshState();
    ref_ptr<HasShader> getShaderState();

  protected:
    ref_ptr<Mesh> meshState_;
    ref_ptr<ShaderInput4f> pos_;
    ref_ptr<ShaderInput4f> col_;

    ref_ptr<HasShader> shaderState_;

    ref_ptr<ShaderInput1f> apparentMagnitude_;
    ref_ptr<ShaderInput3f> color_;
    ref_ptr<ShaderInput1f> colorRatio_;
    ref_ptr<ShaderInput1f> glareIntensity_;
    ref_ptr<ShaderInput1f> glareScale_;
    ref_ptr<ShaderInput1f> scintillation_;
    ref_ptr<ShaderInput1f> scattering_;
    ref_ptr<ShaderInput1f> scale_;
    ref_ptr<TextureState> noiseTexState_;
    ref_ptr<Texture1D> noiseTex_;

    ref_ptr<Texture> noise1_;

    void updateNoiseTexture();
  };
}
#endif /* STARS_LAYER_H_ */
