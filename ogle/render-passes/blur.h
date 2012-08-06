/*
 * blur.h
 *
 *  Created on: 31.07.2012
 *      Author: daniel
 */

#ifndef BLUR_H_
#define BLUR_H_

#include <render-pass.h>
#include <scene.h>

/**
 * Blurs a texture by down sampling it and
 * then horizontal, vertical blurring the down sampled
 * texture.
 */
class BlurSeparablePass : public UnitOrthoRenderPass
{
public:
  BlurSeparablePass(Scene *scene,
      const BlurConfig &blurCfg,
      const float &sizeScale,
      GLenum textureFormat=GL_RGBA16F);
  ~BlurSeparablePass();

  ref_ptr<Texture2D> blurTex() {
    return blurTex_;
  }

  void resize();
  virtual void render();
  virtual bool usesSceneBuffer() { return false; }
protected:
  ref_ptr<Texture2D> blurTex_;
  ref_ptr<FrameBufferObject> blurBuffer_;
  ref_ptr<Shader> downsampleShader_;
  ref_ptr<Shader> horizontalBlur_;
  ref_ptr<Shader> verticalBlur_;
  ref_ptr<EventCallable> projectionChangedCB_;
  GLuint texLocHorizontal_, texLocVertical_;
  Vec2f bufferSize_;
  float sizeScale_;

  void initDownsampleShader();
  void initBlurShader(
      const BlurConfig &blurCfg);
};

#endif /* BLUR_H_ */
