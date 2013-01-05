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

class BlurNode : public StateNode
{
public:
  BlurNode(
      const ref_ptr<Texture> &input,
      const ref_ptr<MeshState> &orthoQuad,
      GLfloat sizeScale);

  void set_sigma(GLfloat sigma);
  const ref_ptr<ShaderInput1f>& sigma() const;

  void set_numPixels(GLfloat numPixels);
  const ref_ptr<ShaderInput1f>& numPixels() const;

  void resize();

  /**
   * Updates shader.
   */
  virtual void set_parent(StateNode *parent);

  const ref_ptr<Texture>& blurredTexture() const;

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
