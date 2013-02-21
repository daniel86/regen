/*
 * blur-state.h
 *
 *  Created on: 13.02.2013
 *      Author: daniel
 */

#ifndef BLUR_STATE_H_
#define BLUR_STATE_H_

#include <ogle/states/state.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/fbo-state.h>

class BlurState : public State
{
public:
  BlurState(
      const ref_ptr<Texture> &inputTexture,
      const Vec2ui &textureSize);

  void createShader(ShaderConfig &cfg);

  const ref_ptr<Texture>& blurTexture() const;
  const ref_ptr<FBOState>& framebuffer() const;

  /**
   * The sigma value for the gaussian function: higher value means more blur.
   */
  void set_sigma(GLfloat sigma);
  /**
   * The sigma value for the gaussian function: higher value means more blur.
   */
  const ref_ptr<ShaderInput1f>& sigma() const;

  /**
   * Half number of texels to consider..
   */
  void set_numPixels(GLfloat numPixels);
  /**
   * Half number of texels to consider..
   */
  const ref_ptr<ShaderInput1f>& numPixels() const;

protected:
  ref_ptr<FBOState> framebuffer_;
  ref_ptr<Texture> blurTexture0_;
  ref_ptr<Texture> blurTexture1_;
  ref_ptr<Texture> input_;

  ref_ptr<ShaderState> downsampleShader_;
  ref_ptr<ShaderState> blurHorizontalShader_;
  ref_ptr<ShaderState> blurVerticalShader_;

  ref_ptr<ShaderInput1f> sigma_;
  ref_ptr<ShaderInput1f> numPixels_;

  void createCubeMapTextures();
  void create2DArrayTextures();
  void create2DTextures();
};

#endif /* BLUR_STATE_H_ */
