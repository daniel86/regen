/*
 * fluid-operation.cpp
 *
 *  Created on: 09.10.2012
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>

#include "fluid-operation.h"

#include <ogle/external/glsw/glsw.h>
#include <ogle/states/blend-state.h>
#include <ogle/utility/string-util.h>
#include <ogle/utility/gl-error.h>
#include <ogle/gl-types/shader.h>

class FluidOperationModify : public State {
public:
  FluidOperationModify(FluidBuffer *fluidBuffer_) : fluidBuffer(fluidBuffer_) {}
  virtual void enable(RenderState *rs) { fluidBuffer->swap(); }
  virtual void disable(RenderState *rs) { fluidBuffer->swap(); }
  FluidBuffer *fluidBuffer;
};
class FluidOperationNext : public State {
public:
  FluidOperationNext(FluidBuffer *fluidBuffer_) : fluidBuffer(fluidBuffer_) {}
  virtual void disable(RenderState *rs) { fluidBuffer->swap(); }
  FluidBuffer *fluidBuffer;
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
  if(code_c==NULL) { return ""; }
  string code(code_c);

  return getShader_(code);
}

FluidOperation::FluidOperation(
    const string &name,
    FluidBuffer *outputBuffer,
    GLboolean is2D,
    GLboolean useObstacles,
    GLboolean isLiquid,
    GLfloat timestep,
    MeshState *textureQuad)
: State(),
  name_(name),
  textureQuad_(textureQuad),
  blendMode_(BLEND_MODE_SRC),
  clear_(GL_FALSE),
  mode_(MODIFY_STATE),
  numIterations_(1),
  posLoc_(-1),
  outputBuffer_(outputBuffer)
{
  // TODO: shader sharing between operations

  outputTexture_ = outputBuffer_->fluidTexture().get();
  posInput_ = textureQuad_->getInputPtr("pos");
  set_mode(MODIFY_STATE);

  // try to load shader with specified name
  string fs = getShader(FORMAT_STRING("fluid." << name));
  if(!fs.empty()) {
    map<GLenum, string> stagesStr;

    // define header that is shared by all fluid operations
    fs = FORMAT_STRING(getShader("fluid.fs.header") << endl << fs);
    // configuration using macros
    if(is2D) {
      fs = FORMAT_STRING("#define IS_2D_SIMULATION\n" << fs);
    }
    if(useObstacles) {
      fs = FORMAT_STRING("#define USE_OBSTACLES\n" << fs);
    }
    if(isLiquid) {
      fs = FORMAT_STRING("#define IS_LIQUID\n" << fs);
    }
    fs = FORMAT_STRING("#define TIMESTEP " << timestep << endl << fs);
    fs = FORMAT_STRING("#version 150\n" << fs);
    stagesStr[GL_FRAGMENT_SHADER] = fs;

    // load vertex shader that is shared by all operations
    string vs = getShader("fluid.vs");
    if(is2D) {
      vs = FORMAT_STRING("#define IS_2D_SIMULATION\n" << vs);
    }
    vs = FORMAT_STRING("#version 150\n" << vs);
    stagesStr[GL_VERTEX_SHADER] = vs;

    if(!is2D) {
      // load geometry for instanced layer selection
      string gs = getShader("fluid.gs");
      gs = FORMAT_STRING("#version 150\n" << gs);
      stagesStr[GL_GEOMETRY_SHADER] = gs;
    }

    shader_ = ref_ptr<Shader>::manage(new Shader(stagesStr));
    if(shader_->compile()) {
      if(!shader_->link()) {
        shader_ = ref_ptr<Shader>();
      }
    } else {
      shader_ = ref_ptr<Shader>();
    }

    if(shader_.get()!=NULL) {
      // TODO: not needed
      glBindFragDataLocation(shader_->id(), 0, "output");
      glBindFragDataLocation(shader_->id(), 1, "output");
      glBindFragDataLocation(shader_->id(), 2, "output");

      posLoc_ = glGetAttribLocation(shader_->id(), "v_pos");
    }
  }
}

const string& FluidOperation::name() const
{
  return name_;
}

Shader* FluidOperation::shader()
{
  return shader_.get();
}

void FluidOperation::set_mode(Mode mode)
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
FluidOperation::Mode FluidOperation::mode() const
{
  return mode_;
}

void FluidOperation::set_blendMode(TextureBlendMode blendMode)
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
TextureBlendMode FluidOperation::blendMode() const
{
  return blendMode_;
}

void FluidOperation::set_clearColor(const Vec4f &clearColor)
{
  clear_ = GL_TRUE;
  clearColor_ = clearColor;
}
GLboolean FluidOperation::clear() const
{
  return clear_;
}

void FluidOperation::set_numIterations(GLuint numIterations)
{
  numIterations_ = numIterations;
}
GLuint FluidOperation::numIterations() const
{
  return numIterations_;
}

void FluidOperation::addInputBuffer(FluidBuffer *buffer, GLint loc)
{
  PositionedFluidBuffer b;
  b.buffer = buffer;
  b.loc = loc;
  inputBuffer_.push_back(b);
}

void FluidOperation::execute(RenderState *rs, GLint lastShaderID)
{
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
    // TODO: clear only first buffer ?
    outputBuffer_->clear(clearColor_);
  }

  for(register int i=0; i<numIterations_; ++i)
  {
    enable(rs);

    // setup render target
    GLint renderTarget = (outputTexture_->bufferIndex()+1) % outputTexture_->numBuffers();
    outputBuffer_->drawBuffer(GL_COLOR_ATTACHMENT0+renderTarget);

    // setup shader input textures
    // FIXME: this breaks not specifying unit !!!
    // maybe used fixed units ???
    GLuint textureChannel = 0;
    for(list<PositionedFluidBuffer>::iterator
        it=inputBuffer_.begin(); it!=inputBuffer_.end(); ++it)
    {
      glActiveTexture(GL_TEXTURE0 + textureChannel);
      it->buffer->fluidTexture()->bind();
      glUniform1i(it->loc, textureChannel);
      ++textureChannel;
    }

    textureQuad_->draw(1);
    disable(rs);
  }
}
