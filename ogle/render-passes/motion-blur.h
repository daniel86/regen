/*
 * motion-blur.h
 *
 *  Created on: 29.07.2012
 *      Author: daniel
 */

#ifndef MOTION_BLUR_H_
#define MOTION_BLUR_H_

#include <filter-shader.h>
#include <render-pass.h>
#include <scene.h>

class MotionBlurPass : public UnitOrthoRenderPass {
public:
  /**
   * Motion blur using a velocity texture.
   * All black parts of the texture are not blurred.
   */
  MotionBlurPass(
      Scene *scene,
      ref_ptr<Texture2D> &velocityTexture,
      ref_ptr<TexelTransfer> &velocityTransfer,
      GLuint numSamples=8);

  /**
   * Motion blur using viewProjection matrix on depth buffer.
   * Note that movement of meshes is not handled, only camera movement
   * results in motion blur.
   * @see GPUGems3 chapter 27
   */
  MotionBlurPass(
      Scene *scene,
      ref_ptr<TexelTransfer> &velocityTransfer,
      GLuint numSamples=8);

  virtual bool rendersOnTop() { return false; }
  virtual bool usesDepthTest() { return false; }
  virtual bool usesSceneBuffer() { return true; }
  virtual void render();
  GLint textureUniform_;
  ref_ptr<Texture> velocityTexture_;
  ref_ptr<UniformMat4> previousViewProjectionUniform_;
};

class MotionBlurShader : public TextureShader {
public:
  MotionBlurShader(
      const vector<string> &args,
      Texture &tex,
      ref_ptr<TexelTransfer> &velocityTransfer,
      GLuint numSamples,
      bool useVelocityTexture);
  virtual string code() const;
  bool useVelocityTexture_;
  ref_ptr<TexelTransfer> velocityTransfer_;
};

#endif /* MOTION_BLUR_H_ */
