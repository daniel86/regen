/*
 * texture-node.h
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#ifndef TEXTURE_NODE_H_
#define TEXTURE_NODE_H_

#include <ogle/states/state.h>
#include <ogle/gl-types/texture.h>
#include <ogle/textures/texel-transfer.h>

class TextureState : public State
{
public:
  TextureState(ref_ptr<Texture> &tex);

  virtual void enable(RenderState*);
  virtual void disable(RenderState*);
  virtual void configureShader(ShaderConfiguration*);

  void set_transfer(ref_ptr<TexelTransfer> transfer);
  ref_ptr<TexelTransfer> transfer();

  ref_ptr<Texture>& texture();
protected:
  ref_ptr<Texture> texture_;
  ref_ptr<TexelTransfer> transfer_;
  GLuint textureUnit_;
};

class TextureConstantUnitNode : public TextureState
{
public:
  TextureConstantUnitNode(ref_ptr<Texture> &tex, GLuint textureUnit);
  virtual void enable(RenderState*);

  GLuint textureUnit() const;
};

#endif /* TEXTURE_NODE_H_ */
