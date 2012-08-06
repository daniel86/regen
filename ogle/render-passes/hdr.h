/*
 * glowing.h
 *
 *  Created on: 23.04.2012
 *      Author: daniel
 */

#ifndef __HDR_H_
#define __HDR_H_

#include <render-pass.h>
#include <scene.h>
#include <texture.h>
#include <shader-generator.h>

struct HDRConfig {
  /**
   * Blur pass config.
   */
  BlurConfig blurCfg;
  HDRConfig()
  {
  }
};

class HDRRenderer
{
public:
  HDRRenderer(
      ref_ptr<Scene> scene,
      const HDRConfig &cfg,
      GLenum format=GL_RGBA16F);
  ~HDRRenderer();

  void resize();
protected:
  ref_ptr<Scene> scene_;
  GLenum format_;

  ref_ptr<Texture2D> downsampledTex2_;
  ref_ptr<FrameBufferObject> downsampledBuffer2_;

  ref_ptr<Texture2D> downsampledTex4_;
  ref_ptr<FrameBufferObject> downsampledBuffer4_;

  ref_ptr<RenderPass> blurHorizontalPass_;
  ref_ptr<RenderPass> blurVerticalPass_;
  ref_ptr<RenderPass> tonemapPass_;
  ref_ptr<RenderPass> downsamplePass_;

  ref_ptr<EventCallable> projectionChangedCB_;

  HDRConfig cfg_;

  void makeDownsamplePass(
      ref_ptr<Texture2D> downsampledTex,
      ref_ptr<FrameBufferObject> downsampledBuffer);
  void makeBlurPass(
      bool isHorizontal, const BlurConfig &blurcfg);
  void makeTonemapPass();

  friend class BlurHDRPass;
  friend class BlurHDRHorizontalPass;
  friend class BlurHDRVerticalPass;
  friend class TonemapHDRPass;
  friend class DownsampleHDRPass;
};

#endif /* __HDR_H_ */
