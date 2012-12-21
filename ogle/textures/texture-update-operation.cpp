/*
 * fluid-operation.cpp
 *
 *  Created on: 09.10.2012
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>

#include "texture-update-operation.h"

#include <ogle/states/blend-state.h>
#include <ogle/utility/string-util.h>
#include <ogle/utility/gl-error.h>
#include <ogle/gl-types/shader.h>
#include <ogle/gl-types/volume-texture.h>

class PrePostSwapOperation : public State {
public:
  PrePostSwapOperation(TextureUpdateOperation *op_) : State(), op(op_) {}
  virtual void enable(RenderState *rs) { op->outputBuffer()->swap(); }
  virtual void disable(RenderState *rs) { op->outputBuffer()->swap(); }
  TextureUpdateOperation *op;
  virtual const string& name() const {
    static const string n = "PrePostSwapOperation";
    return n;
  }
};
class PostSwapOperation : public State {
public:
  PostSwapOperation(TextureUpdateOperation *op_) : State(), op(op_) {}
  virtual void disable(RenderState *rs) { op->outputBuffer()->swap(); }
  TextureUpdateOperation *op;
  virtual const string& name() const {
    static const string n = "PostSwapOperation";
    return n;
  }
};

TextureUpdateOperation::TextureUpdateOperation(
    TextureBuffer *outputBuffer,
    MeshState *textureQuad,
    const map<string,string> &operationConfig,
    const map<string,string> &shaderConfig)
: State(),
  textureQuad_(textureQuad),
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

  outputTexture_ = outputBuffer_->texture().get();
  posInput_ = textureQuad_->getInputPtr("pos");

  Texture3D *tex3D = dynamic_cast<Texture3D*>(outputTexture_);
  if(tex3D!=NULL) {
    numInstances_ = tex3D->numTextures();
  } else {
    numInstances_ = 1;
  }

  map<string,string> functions;
  shader_ = Shader::create(shaderConfig_, functions, shaderNames_);

  if(shader_.get()!=NULL && shader_->compile() && shader_->link()) {
    posLoc_ = shader_->attributeLocation("pos");
  } else {
    shader_ = ref_ptr<Shader>();
  }
}

void TextureUpdateOperation::parseConfig(const map<string,string> &cfg)
{
  map<string,string>::const_iterator needle;

  needle = cfg.find("fs");
  if(needle != cfg.end()) {
    shaderNames_[GL_FRAGMENT_SHADER] = needle->second;
  }
  needle = cfg.find("vs");
  if(needle != cfg.end()) {
    shaderNames_[GL_VERTEX_SHADER] = needle->second;
  }
  needle = cfg.find("gs");
  if(needle != cfg.end()) {
    shaderNames_[GL_GEOMETRY_SHADER] = needle->second;
  }
  needle = cfg.find("tes");
  if(needle != cfg.end()) {
    shaderNames_[GL_TESS_EVALUATION_SHADER] = needle->second;
  }
  needle = cfg.find("tcs");
  if(needle != cfg.end()) {
    shaderNames_[GL_TESS_CONTROL_SHADER] = needle->second;
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

void TextureUpdateOperation::addInputBuffer(TextureBuffer *buffer, GLint loc, const string &nameInShader)
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

void TextureUpdateOperation::set_outputBuffer(TextureBuffer *outputBuffer)
{
  outputBuffer_ = outputBuffer;
  outputTexture_ = outputBuffer_->texture().get();
}
TextureBuffer* TextureUpdateOperation::outputBuffer()
{
  return outputBuffer_;
}

const string& TextureUpdateOperation::name() const {
  static const string n = "TextureUpdateOperation";
  return n;
}

void TextureUpdateOperation::updateTexture(RenderState *rs, GLint lastShaderID)
{
  outputBuffer_->bind();
  outputBuffer_->set_viewport();
  if(clear_==GL_TRUE) {
    outputBuffer_->swap();
    outputBuffer_->clear(clearColor_,1);
    outputBuffer_->swap();
  }

  GLuint shaderID = shader_->id();
  if(lastShaderID!=(GLint)shaderID) {
    glUseProgram(shaderID);
    // setup pos attribute
    posInput_->enable(posLoc_);
  }
  shader_->uploadInputs();

  for(register unsigned int i=0u; i<numIterations_; ++i)
  {
    enable(rs);

    // setup render target
    GLint renderTarget = (outputTexture_->bufferIndex()+1) % outputTexture_->numBuffers();
    outputBuffer_->drawBuffer(GL_COLOR_ATTACHMENT0+renderTarget);

    // setup shader input textures
    GLuint textureChannel = 0;
    for(list<PositionedTextureBuffer>::iterator
        it=inputBuffer_.begin(); it!=inputBuffer_.end(); ++it)
    {
      glActiveTexture(GL_TEXTURE0 + textureChannel);
      it->buffer->texture()->bind();
      glUniform1i(it->loc, textureChannel);
      ++textureChannel;
    }

    textureQuad_->draw(numInstances_);
    disable(rs);
  }
}
