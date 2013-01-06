/*
 * blur-node.h
 *
 *  Created on: 12.11.2012
 *      Author: daniel
 */

#ifndef BLUR_NODE_H_
#define BLUR_NODE_H_

#include <ogle/render-tree/state-node.h>
#include <ogle/states/fbo-state.h>
#include <ogle/states/mesh-state.h>

/**
 * Implements separable blur.
 */
class BlurNode : public StateNode
{
public:
  BlurNode(
      const ref_ptr<Texture> &input,
      const ref_ptr<MeshState> &orthoQuad, // XXX orthoQuad
      GLfloat sizeScale);

  /**
   * Blurred result texture.
   */
  const ref_ptr<Texture>& blurredTexture() const;

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

  void resize();

  /**
   * Updates shader.
   */
  virtual void set_parent(StateNode *parent);

protected:
  ref_ptr<FBOState> framebuffer_;
  ref_ptr<Texture> blurredTexture_;
  ref_ptr<Texture> input_;
  GLfloat sizeScale_;

  ref_ptr<ShaderInput1f> sigma_;
  ref_ptr<ShaderInput1f> numPixels_;

  ref_ptr<ShaderState> blurVertical_;
  ref_ptr<ShaderState> blurHorizontal_;
  ref_ptr<ShaderState> downsample_;
  StateNode *blurVerticalNode_;
  StateNode *blurHorizontalNode_;
  StateNode *downsampleNode_;
};

#endif /* BLUR_NODE_H_ */
