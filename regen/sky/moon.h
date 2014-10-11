/*
 * moon.h
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#ifndef MOON_H_
#define MOON_H_

#include <regen/sky/sky-layer.h>
#include <regen/sky/sky.h>
#include <regen/gl-types/fbo.h>

namespace regen {
  class MoonLayer : public SkyLayer {
  public:
    MoonLayer(const ref_ptr<Sky> &sky, const string &moonMapFile);

    void set_scale(GLdouble altitude);

    const ref_ptr<ShaderInput1f>& scale() const;

    void set_sunShineColor(const Vec3f &color);

    void set_sunShineIntensity(GLdouble sunShineIntensity);

    const ref_ptr<ShaderInput4f>& sunShine() const;

    void set_earthShineColor(const Vec3f &color);

    void set_earthShineIntensity(GLdouble sunShineIntensity);

    const ref_ptr<ShaderInput3f>& earthShine() const;

    GLdouble defaultScale();

    Vec3f defaultSunShineColor();

    GLdouble defaultSunShineIntensity();

    Vec3f defaultEarthShineColor();

    GLdouble defaultEarthShineIntensity();

    // Override
    virtual void updateSkyLayer(RenderState *rs, GLdouble dt);
    ref_ptr<Mesh> getMeshState();
    ref_ptr<HasShader> getShaderState();

  protected:
    ref_ptr<Mesh> meshState_;
    ref_ptr<HasShader> shaderState_;

    ref_ptr<ShaderInput1f> scale_;
    ref_ptr<ShaderInput4f> sunShine_;
    ref_ptr<ShaderInput3f> earthShine_;
    Vec3f earthShineColor_;
    GLdouble earthShineIntensity_;

    ref_ptr<ShaderInput4f> eclParams_;
    ref_ptr<ShaderInputMat4> moonOrientation_;

    ref_ptr<Texture1D> eclTex_;

    void setupMoonTextureCube(const string &cubeMapFilePath);
    void setupEclipseTexture();
  };
}
#endif /* MOON_H_ */
