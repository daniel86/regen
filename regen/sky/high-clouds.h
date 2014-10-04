/*
 * high-clouds.h
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#ifndef HIGH_CLOUDS_H_
#define HIGH_CLOUDS_H_

#include <regen/sky/sky-layer.h>
#include <regen/sky/sky.h>
#include <regen/gl-types/fbo.h>

namespace regen {
  /**
   * \brief High Cloud Layer.
   * @see https://code.google.com/p/osghimmel/
   */
  class HighCloudLayer : public SkyLayer {
  public:
    HighCloudLayer(const ref_ptr<Sky> &sky, GLuint textureSize=2048);

    void set_altitude(GLdouble altitude);

    const ref_ptr<ShaderInput1f>& altitude() const;

    const float defaultAltitude();

    void set_sharpness(GLdouble sharpness);

    const ref_ptr<ShaderInput1f>& sharpness() const;

    void set_coverage(GLdouble coverage);

    const ref_ptr<ShaderInput1f>& coverage() const;

    void set_scale(const Vec2f &scale);

    const ref_ptr<ShaderInput2f>& scale() const;

    const Vec2f defaultScale();

    void set_change(GLdouble change);

    const ref_ptr<ShaderInput1f>& change() const;

    GLdouble defaultChange();

    void set_wind(const Vec2f &wind);

    const ref_ptr<ShaderInput2f>& wind() const;

    void set_color(const Vec3f &color);

    const ref_ptr<ShaderInput3f>& color() const;

    // Override
    void updateSkyLayer(RenderState *rs, GLdouble dt);
    ref_ptr<Mesh> getMeshState();
    ref_ptr<HasShader> getShaderState();

  protected:
    ref_ptr<Mesh> meshState_;
    ref_ptr<HasShader> shaderState_;

    ref_ptr<State> updateState_;
    ref_ptr<ShaderState> updateShader_;

    ref_ptr<ShaderInput1f> coverage_;
    ref_ptr<ShaderInput1f> sharpness_;
    ref_ptr<ShaderInput1f> change_;
    ref_ptr<ShaderInput2f> wind_;
    ref_ptr<ShaderInput3f> color_;
    ref_ptr<ShaderInput1f> altitude_;
    ref_ptr<ShaderInput2f> scale_;

    ref_ptr<FBO> fbo_;
    ref_ptr<Texture2D> cloudTexture_;
    ref_ptr<Texture3D> noise0_;
    ref_ptr<Texture3D> noise1_;
    ref_ptr<Texture3D> noise2_;
    ref_ptr<Texture3D> noise3_;
  };
}
#endif /* HIGH_CLOUDS_H_ */
