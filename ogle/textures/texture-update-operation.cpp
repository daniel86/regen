/*
 * fluid-operation.cpp
 *
 *  Created on: 09.10.2012
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>

#include <ogle/meshes/rectangle.h>
#include <ogle/states/blend-state.h>
#include <ogle/states/shader-configurer.h>
#include <ogle/utility/string-util.h>
#include <ogle/utility/gl-util.h>
#include <ogle/gl-types/shader.h>
#include <ogle/gl-types/gl-enum.h>

#include "texture-update-operation.h"
using namespace ogle;

class PostSwapOperation : public State {
public:
  PostSwapOperation(TextureUpdateOperation *op_) : State(), op(op_) {}
  void disable(RenderState *rs) {
    op->outputBuffer()->firstColorBuffer()->nextBuffer();
  }
  TextureUpdateOperation *op;
};

TextureUpdateOperation::TextureUpdateOperation(const ref_ptr<FrameBufferObject> &outputBuffer)
: State(), numIterations_(1)
{
  textureQuad_ = ref_ptr<Mesh>::cast(Rectangle::getUnitQuad());

  outputTexture_ = outputBuffer->firstColorBuffer();
  outputBuffer_ = ref_ptr<FBOState>::manage(new FBOState(outputBuffer));
  joinStates(ref_ptr<State>::cast(outputBuffer_));

  shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(shader_));

  Texture3D *tex3D = dynamic_cast<Texture3D*>(outputTexture_.get());
  numInstances_ = (tex3D==NULL ? 1 : tex3D->depth());

  set_blendMode(BLEND_MODE_SRC);
}

void TextureUpdateOperation::operator>>(rapidxml::xml_node<> *node)
{
  try {
    set_blendMode( XMLLoader::readAttribute<BlendMode>(node, "blend") );
  } catch(XMLLoader::Error &e) {}
  try {
    set_clearColor( XMLLoader::readAttribute<Vec4f>(node, "clearColor") );
  } catch(XMLLoader::Error &e) {}
  try {
    set_numIterations( XMLLoader::readAttribute<GLuint>(node, "iterations") );
  } catch(XMLLoader::Error &e) {}
}

void TextureUpdateOperation::createShader(const ShaderState::Config &cfg, const string &key)
{
  ShaderConfigurer cfg_(cfg);
  cfg_.addState(this);
  cfg_.addState(textureQuad_.get());
  shader_->createShader(cfg_.cfg(), key);

  for(list<TextureBuffer>::iterator it=inputBuffer_.begin(); it!=inputBuffer_.end(); ++it)
  { it->loc = shader_->shader()->samplerLocation(it->nameInShader); }
}

void TextureUpdateOperation::set_blendMode(BlendMode blendMode)
{
  blendMode_ = blendMode;

  if(blendState_.get()!=NULL) disjoinStates(blendState_);
  blendState_ = ref_ptr<State>::manage(new BlendState(blendMode));
  joinStates(blendState_);

  if(blendMode==BLEND_MODE_SRC) {
    swapState_ = ref_ptr<State>::manage(new PostSwapOperation(this));
  } else {
    swapState_ = ref_ptr<State>::manage(new TexturePingPong(outputTexture_));
  }
}

void TextureUpdateOperation::set_clearColor(const Vec4f &clearColor)
{
  ClearColorState::Data data;
  data.colorBuffers.push_back(GL_COLOR_ATTACHMENT0);
  data.clearColor = clearColor;
  outputBuffer_->setClearColor(data);
}

void TextureUpdateOperation::set_numIterations(GLuint numIterations)
{
  numIterations_ = numIterations;
}

void TextureUpdateOperation::addInputBuffer(const ref_ptr<FrameBufferObject> &buffer, const string &nameInShader)
{
  TextureBuffer b;
  b.buffer = buffer;
  b.loc = shader_->shader().get() ? shader_->shader()->samplerLocation(nameInShader) : 0;
  b.nameInShader = nameInShader;
  inputBuffer_.push_back(b);
}

void TextureUpdateOperation::executeOperation(RenderState *rs)
{
  list<TextureBuffer>::iterator it;

  enable(rs);
  // reserve input texture channels
  for(it=inputBuffer_.begin(); it!=inputBuffer_.end(); ++it)
  { it->channel = rs->reserveTextureChannel(); }

  for(register unsigned int i=0u; i<numIterations_; ++i)
  {
    swapState_->enable(rs);
    // setup render target
    glDrawBuffer(GL_COLOR_ATTACHMENT0 +
        (outputTexture_->bufferIndex()+1) % outputTexture_->numBuffers());
    // setup shader input textures
    for(it=inputBuffer_.begin(); it!=inputBuffer_.end(); ++it)
    {
      it->buffer->firstColorBuffer()->activate(it->channel);
      glUniform1i(it->loc, it->channel);
    }

    textureQuad_->draw(numInstances_);
    swapState_->disable(rs);
  }
  // release input texture channels
  for(it=inputBuffer_.begin(); it!=inputBuffer_.end(); ++it)
  { rs->releaseTextureChannel(); }

  disable(rs);
}


const ref_ptr<Shader>& TextureUpdateOperation::shader()
{ return shader_->shader(); }
const ref_ptr<FrameBufferObject>& TextureUpdateOperation::outputBuffer()
{ return outputBuffer_->fbo(); }
