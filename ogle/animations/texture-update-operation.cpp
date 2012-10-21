/*
 * fluid-operation.cpp
 *
 *  Created on: 09.10.2012
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>

#include "texture-update-operation.h"

#include <ogle/external/glsw/glsw.h>
#include <ogle/states/blend-state.h>
#include <ogle/utility/string-util.h>
#include <ogle/utility/gl-error.h>
#include <ogle/gl-types/shader.h>
#include <ogle/gl-types/volume-texture.h>

class FluidOperationState : public State {
public:
  FluidOperationState(TextureBuffer *fluidBuffer_) : State(), textureBuffer(fluidBuffer_) {}
  TextureBuffer *textureBuffer;
};
class FluidOperationModify : public FluidOperationState {
public:
  FluidOperationModify(TextureBuffer *fluidBuffer_) : FluidOperationState(fluidBuffer_) {}
  virtual void enable(RenderState *rs) { textureBuffer->swap(); }
  virtual void disable(RenderState *rs) { textureBuffer->swap(); }
};
class FluidOperationNext : public FluidOperationState {
public:
  FluidOperationNext(TextureBuffer *fluidBuffer_) : FluidOperationState(fluidBuffer_) {}
  virtual void disable(RenderState *rs) { textureBuffer->swap(); }
};

static GLuint getNumLines(const string &s)
{
  GLuint numLines = 1;
  size_t pos = 0;
  while((pos = s.find_first_of('\n',pos+1)) != string::npos) {
    numLines += 1;
  }
  return numLines;
}
static GLuint getFirstLine(const string &s)
{
  static const int l = string("#line ").length();
  if(boost::starts_with(s, "#line ")) {
    char *pEnd;
    size_t pos = s.find_first_of('\n');
    if(pos==string::npos) {
      return 1;
    } else {
      return strtoul(s.substr(l,pos-l).c_str(), &pEnd, 0);
    }
  } else {
    return 1;
  }
}

static string getShader(const string &effectKey);
static string getShader_(const string &code)
{
  // find first #include directive
  // and split the code at this directive
  size_t pos0 = code.find("#include ");
  if(pos0==string::npos) {
    return code;
  }
  string codeHead = code.substr(0,pos0-1);
  string codeTail = code.substr(pos0);

  // add #line directives for shader compile errors
  // number of code lines (without #line) in code0
  GLuint numLines = getNumLines(codeHead);
  // code0 might start with a #line directive that tells
  // us the first line in the shader file.
  GLuint firstLine = getFirstLine(codeHead);
  string tailLineDirective = FORMAT_STRING(
      "#line " << (firstLine+numLines+1) << endl);

  size_t newLineNeedle = codeTail.find_first_of('\n');
  // parse included shader. the name is right to the #include directive.
  static const int l = string("#include ").length();
  string includedShader;
  if(newLineNeedle == string::npos) {
    includedShader = codeTail.substr(l);
    // no code left
    codeTail = "";
  } else {
    includedShader = codeTail.substr(l,newLineNeedle-l);
    // delete the #include directive line in code1
    codeTail = FORMAT_STRING(tailLineDirective <<
        codeTail.substr(newLineNeedle+1));
  }
  includedShader = getShader(includedShader);

  return FORMAT_STRING(codeHead << includedShader << getShader_(codeTail));
}
static string getShader(const string &effectKey)
{
  const char *code_c = glswGetShader(effectKey.c_str());
  if(code_c==NULL) {
    cerr << glswGetError() << endl;
    return "";
  }
  string code(code_c);

  return getShader_(code);
}

TextureUpdateOperation::TextureUpdateOperation(
    map<GLenum,string> shaderNames,
    TextureBuffer *outputBuffer,
    MeshState *textureQuad,
    map<string,string> &shaderConfig)
: State(),
  shaderNames_(shaderNames),
  textureQuad_(textureQuad),
  blendMode_(BLEND_MODE_SRC),
  clear_(GL_FALSE),
  mode_(MODIFY_STATE),
  numIterations_(1),
  posLoc_(-1),
  outputBuffer_(outputBuffer),
  shaderConfig_(shaderConfig)
{

  outputTexture_ = outputBuffer_->texture().get();
  posInput_ = textureQuad_->getInputPtr("pos");
  set_mode(MODIFY_STATE);

  Texture3D *tex3D = dynamic_cast<Texture3D*>(outputTexture_);
  if(tex3D!=NULL) {
    numInstances_ = tex3D->numTextures();
  } else {
    numInstances_ = 1;
  }

  // configuration using macros
  string shaderHeader = "#version 150\n"; // TODO: configurable
  for(map<string,string>::iterator
      it=shaderConfig_.begin(); it!=shaderConfig_.end(); ++it)
  {
    const string &name = it->first;
    const string &value = it->second;
    if(value=="1") {
      shaderHeader = FORMAT_STRING("#define "<<name<<"\n" << shaderHeader);
    }
    else if(value=="0") {
      shaderHeader = FORMAT_STRING("// #undef "<<name<<"\n" << shaderHeader);
    }
    else {
      shaderHeader = FORMAT_STRING("#define "<<name<<" "<<value<<"\n" << shaderHeader);
    }
  }

  list<string> effectNames;
  map<GLenum,string> stagesStr;

  for(map<GLenum,string>::iterator
      it=shaderNames_.begin(); it!=shaderNames_.end(); ++it)
  {
    // try to load shader with specified name
    string shaderCode = (it->second.size()<99 ? getShader(it->second) : "");
    if(!shaderCode.empty()) {
      stringstream ss;
      ss << shaderHeader << endl;
      ss << shaderCode << endl;
      stagesStr[it->first] = ss.str();

      list<string> path;
      boost::split(path, it->second, boost::is_any_of("."));
      effectNames.push_back(*path.begin());
    } else {
      // we expect shader code directly provided
      stagesStr[it->first] = it->second;
    }
  }

  // if no vertex shader provided try to load default for effect
  if(stagesStr.count(GL_VERTEX_SHADER)==0) {
    for(list<string>::iterator it=effectNames.begin(); it!=effectNames.end(); ++it) {
      string defaultVSName = FORMAT_STRING((*it) << ".vs");
      string code = getShader(defaultVSName);
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
  if(shader_->compile() && shader_->link()) {
    posLoc_ = glGetAttribLocation(shader_->id(), "v_pos");

    // TODO: do this everywhere!
    glUseProgram(shader_->id());
    shader_->setupUniforms();
  } else {
    shader_ = ref_ptr<Shader>();
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

void TextureUpdateOperation::set_mode(Mode mode)
{
  if(operationModeState_.get()!=NULL) {
    disjoinStates(operationModeState_);
  }
  mode_ = mode;
  if(mode_==MODIFY_STATE) {
    operationModeState_ = ref_ptr<State>::manage(
        new FluidOperationModify(outputBuffer_));
    joinStates(operationModeState_);
  } else {
    operationModeState_ = ref_ptr<State>::manage(
        new FluidOperationNext(outputBuffer_));
    joinStates(operationModeState_);
  }
}
TextureUpdateOperation::Mode TextureUpdateOperation::mode() const
{
  return mode_;
}

void TextureUpdateOperation::set_blendMode(TextureBlendMode blendMode)
{
  blendMode_ = blendMode;

  ref_ptr<BlendState> blendState =
      ref_ptr<BlendState>::manage(new BlendState(GL_ONE, GL_ZERO));

  switch(blendMode) {
  case BLEND_MODE_ALPHA:
    // c = c0*(1-c1.a) + c1*c1.a
    blendState->setBlendEquation(GL_FUNC_ADD);
    blendState->setBlendFunc(
        GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    break;
  case BLEND_MODE_MULTIPLY:
    // c = c0*c1, a=a0*a1
    blendState->setBlendEquation(GL_FUNC_ADD);
    blendState->setBlendFunc(GL_DST_COLOR, GL_ZERO);
    break;
  case BLEND_MODE_ADD:
    // c = c0+c1, a=a0+a1
    blendState->setBlendEquation(GL_FUNC_ADD);
    blendState->setBlendFunc(GL_ONE, GL_ONE);
    break;
  case BLEND_MODE_SMOOTH_ADD: // aka average
    // c = 0.5*c0 + 0.5*c1
    // a = a0 + a1
    blendState->setBlendEquation(GL_FUNC_ADD);
    blendState->setBlendFuncSeparate(
        GL_CONSTANT_ALPHA, GL_CONSTANT_ALPHA,
        GL_ONE, GL_ONE);
    blendState->setBlendColor(Vec4f(0.5f));
    break;
  case BLEND_MODE_SUBSTRACT:
    // c = c0-c1, a=a0-a1
    blendState->setBlendEquation(GL_FUNC_SUBTRACT);
    blendState->setBlendFunc(GL_ONE, GL_ONE);
    break;
  case BLEND_MODE_REVERSE_SUBSTRACT:
    // c = c1-c0, a=c1-c0
    blendState->setBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
    blendState->setBlendFunc(GL_ONE, GL_ONE);
    break;
  case BLEND_MODE_LIGHTEN:
    // c = max(c0,c1), a = max(a0,a1)
    blendState->setBlendEquation(GL_MAX);
    blendState->setBlendFunc(GL_ONE, GL_ONE);
    break;
  case BLEND_MODE_DARKEN:
    // c = min(c0,c1), a = min(a0,a1)
    blendState->setBlendEquation(GL_MIN);
    blendState->setBlendFunc(GL_ONE, GL_ONE);
    break;
  case BLEND_MODE_SCREEN:
    // c = c0 - c1*(1-c0), a = a0 - a1*(1-a0)
    blendState->setBlendEquation(GL_FUNC_SUBTRACT);
    blendState->setBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE);
    break;
  case BLEND_MODE_SRC_ALPHA:
    // c = c1*c1.a
    blendState->setBlendFunc(GL_SRC_ALPHA, GL_ZERO);
    break;
  case BLEND_MODE_SRC:
  default:
    // c = c1, a = a1
    blendState->setBlendFunc(GL_ONE, GL_ZERO);
    break;
  }

  if(blendState_.get()!=NULL) {
    disjoinStates(blendState_);
  }
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

void TextureUpdateOperation::addInputBuffer(TextureBuffer *buffer, GLint loc)
{
  PositionedTextureBuffer b;
  b.buffer = buffer;
  b.loc = loc;
  inputBuffer_.push_back(b);
}

void TextureUpdateOperation::set_outputBuffer(TextureBuffer *outputBuffer)
{
  outputBuffer_ = outputBuffer;
  outputTexture_ = outputBuffer_->texture().get();
  if(operationModeState_.get()!=NULL) {
    FluidOperationState *state = (FluidOperationState*)operationModeState_.get();
    state->textureBuffer = outputBuffer_;
  }
}
TextureBuffer* TextureUpdateOperation::outputBuffer()
{
  return outputBuffer_;
}

void TextureUpdateOperation::execute(RenderState *rs, GLint lastShaderID)
{
  // TODO TEXTURE UPDATE OPERATION: less glUniform calls
  //    * textures/uniforms/shaders/fbo may not need update
  //    * do not generate identical shaders

  GLuint shaderID = shader_->id();
  if(lastShaderID!=shaderID) {
    glUseProgram(shaderID);
    // setup pos attribute
    posInput_->enable(posLoc_);
  }
  shader_->applyInputs();

  outputBuffer_->bind();
  outputBuffer_->set_viewport();
  if(clear_==GL_TRUE) {
    outputBuffer_->swap();
    outputBuffer_->clear(clearColor_,1);
    outputBuffer_->swap();
  }

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
