/*
 * fluid-operation.cpp
 *
 *  Created on: 09.10.2012
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>

#include <ogle/meshes/rectangle.h>
#include <ogle/states/blend-state.h>
#include <ogle/utility/string-util.h>
#include <ogle/utility/gl-util.h>
#include <ogle/gl-types/shader.h>
#include <ogle/gl-types/gl-enum.h>

#include "texture-update-operation.h"
using namespace ogle;

class PrePostSwapOperation : public State {
public:
  PrePostSwapOperation(TextureUpdateOperation *op_) : State(), op(op_) {}
  void enable(RenderState *rs)
  {
    op->outputBuffer()->firstColorBuffer()->nextBuffer();
  }
  void disable(RenderState *rs)
  {
    op->outputBuffer()->firstColorBuffer()->nextBuffer();
  }
  TextureUpdateOperation *op;
};

class PostSwapOperation : public State {
public:
  PostSwapOperation(TextureUpdateOperation *op_) : State(), op(op_) {}
  void disable(RenderState *rs) {
    op->outputBuffer()->firstColorBuffer()->nextBuffer();
  }
  TextureUpdateOperation *op;
};

TextureUpdateOperation::TextureUpdateOperation(
    FrameBufferObject *outputBuffer,
    GLuint shaderVersion,
    const map<string,string> &operationConfig,
    const map<string,string> &shaderConfig)
: State(),
  shaderConfig_(shaderConfig),
  posLoc_(-1),
  blendMode_(BLEND_MODE_SRC),
  outputBuffer_(outputBuffer),
  clear_(GL_FALSE),
  clearColor_(Vec4f(0.0f)),
  numIterations_(1)
{
  map<string,string>::const_iterator needle;

  set_blendMode(BLEND_MODE_SRC);
  parseConfig(operationConfig);

  textureQuad_ = ref_ptr<Mesh>::cast(Rectangle::getUnitQuad());

  outputTexture_ = outputBuffer_->firstColorBuffer().get();
  posInput_ = textureQuad_->getInput("pos");

  Texture3D *tex3D = dynamic_cast<Texture3D*>(outputTexture_);
  if(tex3D!=NULL) {
    numInstances_ = tex3D->depth();
  } else {
    numInstances_ = 1;
  }

  map<string,string> functions;
  shader_ = Shader::create(shaderVersion, shaderConfig_, functions, shaderNames_);

  if(shader_.get()!=NULL && shader_->compile() && shader_->link()) {
    posLoc_ = shader_->attributeLocation("pos");
  } else {
    shader_ = ref_ptr<Shader>();
  }
}

void TextureUpdateOperation::parseConfig(const map<string,string> &cfg)
{
  map<string,string>::const_iterator needle;

  for(GLint i=0; i<glslStageCount(); ++i)
  {
    GLenum stage = glslStageEnums()[i];
    string stagePrefix = glslStagePrefix(stage);

    needle = cfg.find(stagePrefix);
    if(needle != cfg.end()) {
      shaderNames_[stage] = needle->second;
    }
  }

  needle = cfg.find("blend");
  if(needle != cfg.end()) {
    BlendMode blendMode;
    stringstream ss(needle->second);
    ss >> blendMode;
    set_blendMode(blendMode);
  }

  needle = cfg.find("clearColor");
  if(needle != cfg.end()) {
    Vec4f color(0.0f);
    stringstream ss(needle->second);
    ss >> color;
    set_clearColor(color);
  }

  needle = cfg.find("iterations");
  if(needle != cfg.end()) {
    GLuint count=numIterations_;
    stringstream ss(needle->second);
    ss >> count;
    set_numIterations(count);
  }
}

string TextureUpdateOperation::fsName()
{
  return shaderNames_[GL_FRAGMENT_SHADER];
}
map<GLenum,string>& TextureUpdateOperation::shaderNames()
{
  return shaderNames_;
}
map<string,string>& TextureUpdateOperation::shaderConfig()
{
  return shaderConfig_;
}

Shader* TextureUpdateOperation::shader()
{
  return shader_.get();
}

void TextureUpdateOperation::set_blendMode(BlendMode blendMode)
{
  blendMode_ = blendMode;

  ref_ptr<BlendState> blendState = ref_ptr<BlendState>::manage(new BlendState(blendMode));
  if(blendState_.get()!=NULL) {
    disjoinStates(blendState_);
  }
  if(swapState_.get()!=NULL) {
    disjoinStates(swapState_);
  }

  if(blendMode==BLEND_MODE_SRC) {
    // no blending
    swapState_ = ref_ptr<State>::manage(new PostSwapOperation(this));
    joinStates(swapState_);
    return;
  }

  swapState_ = ref_ptr<State>::manage(new PrePostSwapOperation(this));
  joinStates(swapState_);

  blendState_ = ref_ptr<State>::cast(blendState);
  joinStates(blendState_);
}
const BlendMode& TextureUpdateOperation::blendMode() const
{
  return blendMode_;
}

void TextureUpdateOperation::set_clearColor(const Vec4f &clearColor)
{
  clear_ = GL_TRUE;
  clearColor_ = clearColor;
}
GLboolean TextureUpdateOperation::clear() const
{
  return clear_;
}
const Vec4f& TextureUpdateOperation::clearColor() const
{
  return clearColor_;
}

void TextureUpdateOperation::set_numIterations(GLuint numIterations)
{
  numIterations_ = numIterations;
}
GLuint TextureUpdateOperation::numIterations() const
{
  return numIterations_;
}

void TextureUpdateOperation::addInputBuffer(FrameBufferObject *buffer, GLint loc, const string &nameInShader)
{
  PositionedTextureBuffer b;
  b.buffer = buffer;
  b.loc = loc;
  b.nameInShader = nameInShader;
  inputBuffer_.push_back(b);
}
list<TextureUpdateOperation::PositionedTextureBuffer>& TextureUpdateOperation::inputBuffer()
{
  return inputBuffer_;
}

void TextureUpdateOperation::set_outputBuffer(FrameBufferObject *outputBuffer)
{
  outputBuffer_ = outputBuffer;
  outputTexture_ = outputBuffer_->firstColorBuffer().get();
}
FrameBufferObject* TextureUpdateOperation::outputBuffer()
{
  return outputBuffer_;
}

void TextureUpdateOperation::updateTexture(RenderState *rs, GLint lastShaderID)
{
  rs->fbo().push(outputBuffer_);
  if(clear_==GL_TRUE) {
    outputTexture_->nextBuffer();
    outputBuffer_->drawBuffers();
    glClearColor(clearColor_.x, clearColor_.y, clearColor_.z, clearColor_.w);
    glClear(GL_COLOR_BUFFER_BIT);
    outputTexture_->nextBuffer();
  }

  rs->shader().push(shader_.get());
  {
    glBindBuffer(GL_ARRAY_BUFFER, posInput_->buffer());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, posInput_->buffer());
    posInput_->enable(posLoc_);
  }

  for(register unsigned int i=0u; i<numIterations_; ++i)
  {
    enable(rs);

    // setup render target
    GLint renderTarget = (outputTexture_->bufferIndex()+1) % outputTexture_->numBuffers();
    outputBuffer_->drawBuffer(GL_COLOR_ATTACHMENT0+renderTarget);

    // setup shader input textures
    for(list<PositionedTextureBuffer>::iterator
        it=inputBuffer_.begin(); it!=inputBuffer_.end(); ++it)
    {
      GLuint textureChannel = rs->reserveTextureChannel();
      glActiveTexture(GL_TEXTURE0 + textureChannel);
      it->buffer->firstColorBuffer()->bind();
      glUniform1i(it->loc, textureChannel);
    }

    textureQuad_->draw(numInstances_);

    for(list<PositionedTextureBuffer>::iterator
        it=inputBuffer_.begin(); it!=inputBuffer_.end(); ++it)
    {
      rs->releaseTextureChannel();
    }

    disable(rs);
  }

  rs->shader().pop();
  rs->fbo().pop();
}
