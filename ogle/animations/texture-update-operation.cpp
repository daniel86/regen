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
#include <ogle/shader/shader-manager.h>
#include <ogle/gl-types/shader.h>
#include <ogle/gl-types/volume-texture.h>

class PrePostSwapOperation : public State {
public:
  PrePostSwapOperation(TextureUpdateOperation *op_) : State(), op(op_) {}
  virtual void enable(RenderState *rs) { op->outputBuffer()->swap(); }
  virtual void disable(RenderState *rs) { op->outputBuffer()->swap(); }
  TextureUpdateOperation *op;
};
class PostSwapOperation : public State {
public:
  PostSwapOperation(TextureUpdateOperation *op_) : State(), op(op_) {}
  virtual void disable(RenderState *rs) { op->outputBuffer()->swap(); }
  TextureUpdateOperation *op;
};

TextureUpdateOperation::TextureUpdateOperation(
    TextureBuffer *outputBuffer,
    MeshState *textureQuad,
    const map<string,string> &operationConfig,
    const map<string,string> &shaderConfig)
: State(),
  textureQuad_(textureQuad),
  blendMode_(BLEND_MODE_SRC),
  clear_(GL_FALSE),
  clearColor_(Vec4f(0.0f)),
  numIterations_(1),
  posLoc_(-1),
  outputBuffer_(outputBuffer),
  shaderConfig_(shaderConfig)
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

  string shaderHeader="";

  needle = operationConfig.find("versionGLSL");
  if(needle != operationConfig.end()) {
    shaderHeader = FORMAT_STRING("#version "<<needle->second<<"\n" << shaderHeader);
  } else {
    shaderHeader = FORMAT_STRING("#version 150\n" << shaderHeader);
  }

  list<string> defs;

  // configuration using macros
  for(map<string,string>::iterator
      it=shaderConfig_.begin(); it!=shaderConfig_.end(); ++it)
  {
    const string &name = it->first;
    const string &value = it->second;
    if(value=="1") {
      shaderHeader = FORMAT_STRING("#define "<<name<<"\n" << shaderHeader);
      defs.push_back(name);
    }
    else if(value=="0") {
      shaderHeader = FORMAT_STRING("// #undef "<<name<<"\n" << shaderHeader);
    }
    else {
      shaderHeader = FORMAT_STRING("#define "<<name<<" "<<value<<"\n" << shaderHeader);
      defs.push_back(name + "=" + value);
    }
  }

  // sort for signature
  defs.sort();

  string signature = "";
  static const GLenum stages[] = {
      GL_FRAGMENT_SHADER,
      GL_VERTEX_SHADER,
      GL_GEOMETRY_SHADER,
      GL_TESS_CONTROL_SHADER,
      GL_TESS_EVALUATION_SHADER};
  for(int i=0; i<sizeof(stages)/sizeof(GLenum); ++i)
  {
    GLenum stage = stages[i];
    if(shaderNames_.count(stage)>0 &&
        !boost::contains(shaderNames_[stage], "\n"))
    {
      signature += "&" + shaderNames_[stage];
    }
  }
  for(list<string>::iterator it=defs.begin(); it!=defs.end(); ++it)
  {
    signature += "&" + (*it);
  }

  ref_ptr<Shader> shader = ShaderManager::getShaderWithSignarure(signature);
  if(shader.get()!=NULL) {
    // use previously loaded shader
    shader_ = ref_ptr<Shader>::manage(new Shader(*(shader.get())));
    glUseProgram(shader_->id());
    shader_->setupInputLocations();
    posLoc_ = shader_->attributeLocation("pos");
  } else {
    list<string> effectNames;
    map<GLenum,string> stagesStr;

    for(map<GLenum,string>::iterator
        it=shaderNames_.begin(); it!=shaderNames_.end(); ++it)
    {
      if(!boost::contains(it->second, "\n")) {
        // try to load shader with specified name
        string shaderCode = ShaderManager::loadShaderFromKey(it->second);

        if(!shaderCode.empty()) {
          stringstream ss;
          ss << shaderHeader << endl;
          ss << shaderCode << endl;
          stagesStr[it->first] = ss.str();

          list<string> path;
          boost::split(path, it->second, boost::is_any_of("."));
          effectNames.push_back(*path.begin());

          continue;
        }
      }
      {
        // we expect shader code directly provided
        string shaderCode = ShaderManager::loadShaderCode(it->second);
        stringstream ss;
        ss << shaderHeader << endl;
        ss << shaderCode << endl;
        stagesStr[it->first] = ss.str();
      }
    }

    // if no vertex shader provided try to load default for effect
    if(stagesStr.count(GL_VERTEX_SHADER)==0) {
      for(list<string>::iterator it=effectNames.begin(); it!=effectNames.end(); ++it) {
        string defaultVSName = FORMAT_STRING((*it) << ".vs");
        string code = ShaderManager::loadShaderFromKey(defaultVSName);
        if(!code.empty()) {
          stringstream ss;
          ss << shaderHeader << endl;
          ss << code << endl;
          stagesStr[GL_VERTEX_SHADER] = ss.str();
          break;
        }
      }
    }

    shader_ = ref_ptr<Shader>::manage(new Shader(stagesStr));
    ShaderManager::setShaderWithSignarure(shader_,signature);
    if(shader_->compile() && shader_->link()) {
      glUseProgram(shader_->id());
      shader_->setupInputLocations();
      posLoc_ = shader_->attributeLocation("pos");
    } else {
      shader_ = ref_ptr<Shader>();
    }
  }
}

ostream& operator<<(ostream &out, TextureUpdateOperation &v)
{
  out << "        <operation" << endl;

  // shader stages
  // TODO: can be specified in xml file!
  for(map<GLenum,string>::iterator
      it=v.shaderNames_.begin(); it!=v.shaderNames_.end(); ++it)
  {
    switch(it->first) {
    case GL_FRAGMENT_SHADER:
      out << "            fs=\"" << it->second << "\"" << endl;
      break;
    case GL_VERTEX_SHADER:
      out << "            vs=\"" << it->second << "\"" << endl;
      break;
    case GL_GEOMETRY_SHADER:
      out << "            gs=\"" << it->second << "\"" << endl;
      break;
    case GL_TESS_EVALUATION_SHADER:
      out << "            tes=\"" << it->second << "\"" << endl;
      break;
    case GL_TESS_CONTROL_SHADER:
      out << "            tcs=\"" << it->second << "\"" << endl;
      break;
    }
  }

  if(v.blendMode_!=BLEND_MODE_SRC) {
    out << "            blend=\"" << v.blendMode_ << "\"" << endl;
  }
  if(v.clear_==GL_TRUE) {
    out << "            clearColor=\"" << v.clearColor_ << "\"" << endl;
  }
  if(v.numIterations_>1) {
    out << "            iterations=\"" << v.numIterations_ << "\"" << endl;
  }

  out << "            MACROS=XXXXX" << endl;

  if(v.shader_.get()!=NULL) {
    const map<string, ref_ptr<ShaderInput> > &input = v.shader_->inputs();

    for(map<string, ref_ptr<ShaderInput> >::const_iterator
        it=input.begin(); it!=input.end(); ++it)
    {
      const ref_ptr<ShaderInput> &inRef = it->second;
      if(!inRef->hasData()) continue;
      const ShaderInput &in = *(inRef.get());
      out << "            in_" << inRef->name() << "=\"";
      in >> out;
      out << "\"" << endl;
    }
  }

  for(list<TextureUpdateOperation::PositionedTextureBuffer>::iterator
      it=v.inputBuffer_.begin(); it!=v.inputBuffer_.end(); ++it)
  {
    TextureUpdateOperation::PositionedTextureBuffer &buffer = *it;
    out << "            in_" << buffer.nameInShader << "=\"" << buffer.buffer->name() << "\"" << endl;
  }

  if(v.outputBuffer_!=NULL) {
    out << "            out=\"" << v.outputBuffer_->name() << "\"" << endl;
  }

  out << "        />" << endl;
  return out;
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
    TextureBlendMode blendMode;
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

Shader* TextureUpdateOperation::shader()
{
  return shader_.get();
}

void TextureUpdateOperation::set_blendMode(TextureBlendMode blendMode)
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
TextureBlendMode TextureUpdateOperation::blendMode() const
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

void TextureUpdateOperation::set_outputBuffer(TextureBuffer *outputBuffer)
{
  outputBuffer_ = outputBuffer;
  outputTexture_ = outputBuffer_->texture().get();
}
TextureBuffer* TextureUpdateOperation::outputBuffer()
{
  return outputBuffer_;
}

void TextureUpdateOperation::updateTexture(RenderState *rs, GLint lastShaderID)
{
  // TODO TEXTURE UPDATE OPERATION: less glUniform calls
  //    * use a tex channel free list to save some tex switches
  //    * integrate with RenderState ? (also lastShader)
  //    * evil slap buffers need multiple channels (maybe in different frames/iterations)
  //    * drop out longest unused tex if no channel left

  outputBuffer_->bind();
  outputBuffer_->set_viewport();
  if(clear_==GL_TRUE) {
    outputBuffer_->swap();
    outputBuffer_->clear(clearColor_,1);
    outputBuffer_->swap();
  }

  GLuint shaderID = shader_->id();
  if(lastShaderID!=shaderID) {
    glUseProgram(shaderID);
    // setup pos attribute
    posInput_->enable(posLoc_);
  }
  shader_->applyInputs();

  for(register int i=0; i<numIterations_; ++i)
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
