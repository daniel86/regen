/*
 * shader.cpp
 *
 *  Created on: 26.03.2011
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <regen/utility/logging.h>
#include <regen/utility/string-util.h>
#include <regen/gl-types/gl-util.h>
#include <regen/gl-types/gl-enum.h>
#include <regen/gl-types/glsl/directive-processor.h>
#include <regen/gl-types/glsl/io-processor.h>
#include <regen/gl-types/glsl/comment-processor.h>

#include "shader.h"
using namespace regen;

/////////////
/////////////

ref_ptr<PreProcessor>& Shader::defaultPreProcessor()
{
  static ref_ptr<PreProcessor> defaultProcessor;
  if(!defaultProcessor.get()) {
    defaultProcessor = ref_ptr<PreProcessor>::alloc();
    defaultProcessor->addProcessor(ref_ptr<InputProviderProcessor>::alloc());
    defaultProcessor->addProcessor(ref_ptr<DirectiveProcessor>::alloc());
    defaultProcessor->addProcessor(ref_ptr<CommentProcessor>::alloc());
    defaultProcessor->addProcessor(ref_ptr<WhiteSpaceProcessor>::alloc());
    defaultProcessor->addProcessor(ref_ptr<IOProcessor>::alloc());
  }
  return defaultProcessor;
}

ref_ptr<PreProcessor>& Shader::singleStagePreProcessor()
{
  static ref_ptr<PreProcessor> singleProcessor;
  if(!singleProcessor.get()) {
    singleProcessor = ref_ptr<PreProcessor>::alloc();
    singleProcessor->addProcessor(ref_ptr<InputProviderProcessor>::alloc());
    singleProcessor->addProcessor(ref_ptr<DirectiveProcessor>::alloc());
    singleProcessor->addProcessor(ref_ptr<CommentProcessor>::alloc());
    singleProcessor->addProcessor(ref_ptr<WhiteSpaceProcessor>::alloc());
  }
  return singleProcessor;
}

void Shader::preProcess(
    map<GLenum,string> &ret,
    const PreProcessorConfig &cfg,
    const ref_ptr<PreProcessor> &preProcessor)
{
  // configure shader using macros
  stringstream header;
  header << "#version " << cfg.version << endl;
  for(map<string,string>::const_iterator
      it=cfg.defines.begin(); it!=cfg.defines.end(); ++it)
  {
    const string &name = it->first;
    const string &value = it->second;
    if(value=="TRUE") {
      header << "#define " << name << endl;
    } else if(value=="FALSE") {
      header << "// #undef " << name << endl;
    } else {
      header << "#define " << name << " " << value << endl;
    }
  }
  string headerStr = header.str();

  // load the GLSL code.
  PreProcessorInput preProcessorInput(
      headerStr,
      cfg.unprocessed,
      cfg.externalFunctions,
      cfg.specifiedInput);
  ret = preProcessor->processStages(preProcessorInput);
}

/////////////
/////////////

void Shader::printLog(
    GLuint shader,
    GLenum shaderType,
    const char *shaderCode,
    GLboolean success)
{
  Logging::LogLevel logLevel;
  if(shaderCode != NULL) {
    string shaderName = glenum::glslStageName(shaderType);

    if(success) {
      logLevel = Logging::INFO;
      //REGEN_LOG(logLevel, shaderName << " Shader compiled successfully!");
    } else {
      logLevel = Logging::ERROR;
      REGEN_LOG(logLevel, shaderName << " Shader failed to compile!");
    }
  } else {
    if(success) {
      logLevel = Logging::INFO;
      //REGEN_LOG(logLevel, "Shader linked successfully.");
    } else {
      logLevel = Logging::ERROR;
      REGEN_LOG(logLevel, "Shader failed to link.");
    }
  }

  if(!success && shaderCode != NULL) {
    vector<string> codeLines;
    boost::split(codeLines, shaderCode, boost::is_any_of("\n"));
    for(GLuint i=0; i<codeLines.size(); ++i) {
      REGEN_LOG(logLevel,
          setw(3) << i << setw(0) << " " << codeLines[i]);
    }
  }

  int length;
  if(shaderType==GL_NONE) {
    glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &length);
  } else {
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
  }
  if(length>1) {
    char *log = new char[length];
    string msgPrefix;
    if(shaderType==GL_NONE) {
      glGetProgramInfoLog(shader, length, NULL, log);
      msgPrefix = "link info: ";
    } else {
      glGetShaderInfoLog(shader, length, NULL, log);
      msgPrefix = "compile info: ";
    }

    vector<string> errorLines;
    boost::split(errorLines, log, boost::is_any_of("\n"));
    for(GLuint i=0; i<errorLines.size(); ++i) {
      if(!errorLines[i].empty())
        REGEN_LOG(logLevel, msgPrefix << errorLines[i]);
    }
    delete []log;
  }
}

/////////////
/////////////

Shader::Shader(const Shader &other)
: id_(other.id_),
  shaderCodes_(other.shaderCodes_),
  shaders_(other.shaders_),
  feedbackLayout_(GL_SEPARATE_ATTRIBS)
{
}

Shader::Shader(const map<GLenum, string> &shaderCodes)
: shaderCodes_(shaderCodes),
  feedbackLayout_(GL_SEPARATE_ATTRIBS)
{
  id_ = ref_ptr<GLuint>::alloc();
  *(id_.get()) = glCreateProgram();
}

Shader::Shader(
    const map<GLenum, string> &shaderNames,
    const map<GLenum, ref_ptr<GLuint> > &shaderStages)
: shaderCodes_(shaderNames),
  shaders_(shaderStages),
  feedbackLayout_(GL_SEPARATE_ATTRIBS)
{
  id_ = ref_ptr<GLuint>::alloc();
  *(id_.get()) = glCreateProgram();

  for(map<GLenum, ref_ptr<GLuint> >::const_iterator
      it = shaders_.begin(); it != shaders_.end(); ++it)
  {
    glAttachShader(id(), *it->second.get());
  }
}

Shader::~Shader()
{
  for(map<GLenum, ref_ptr<GLuint> >::iterator
      it = shaders_.begin(); it != shaders_.end(); ++it)
  {
    ref_ptr<GLuint> &stage = it->second;
    glDetachShader(id(), *stage.get());
    if(*stage.refCount()==1) {
      glDeleteShader(*stage.get());
    }
  }

  if(*id_.refCount()==1) {
    glDeleteProgram(id());
  }
}

GLboolean Shader::hasStage(GLenum stage) const
{
  return shaders_.count(stage)>0;
}

const string& Shader::stageCode(GLenum stage) const
{
  map<GLenum, string>::const_iterator it = shaderCodes_.find(stage);
  if(it!=shaderCodes_.end()) {
    return it->second;
  } else {
    static const string empty = "";
    return empty;
  }
}

ref_ptr<GLuint> Shader::stage(GLenum s) const
{
  map<GLenum, ref_ptr<GLuint> >::const_iterator it = shaders_.find(s);
  if(it!=shaders_.end()) {
    return it->second;
  } else {
    return ref_ptr<GLuint>();
  }
}

const map<string, ref_ptr<ShaderInput> >& Shader::inputs() const
{ return inputs_; }
const map<GLint, ShaderTextureLocation>& Shader::textures() const
{ return textures_; }
const list<ShaderInputLocation>& Shader::attributes() const
{ return attributes_; }

GLboolean Shader::hasUniform(const string &name) const
{
  return inputs_.count(name)>0;
}
GLboolean Shader::hasSampler(const string &name) const
{
  map<string, GLint>::const_iterator it = samplerLocations_.find(name);
  if(it == samplerLocations_.end()) return GL_FALSE;
  return textures_.count(it->second)>0;
}

GLboolean Shader::hasUniformData(const string &name) const
{
  map<string, ref_ptr<ShaderInput> >::const_iterator it = inputs_.find(name);
  if(it==inputs_.end()) {
    return GL_FALSE;
  } else {
    return it->second->hasData();
  }
}

ref_ptr<ShaderInput> Shader::input(const string &name)
{
  return inputs_[name];
}

GLint Shader::samplerLocation(const string &name)
{
  map<string, GLint>::iterator it = samplerLocations_.find(name);
  return (it != samplerLocations_.end()) ? it->second :  -1;
}

GLint Shader::attributeLocation(const string &name)
{
  map<string, GLint>::iterator it = attributeLocations_.find(name);
  return (it != attributeLocations_.end()) ? it->second :  -1;
}

GLint Shader::uniformLocation(const string &name)
{
  map<string, GLint>::iterator it = uniformLocations_.find(name);
  return (it != uniformLocations_.end()) ? it->second :  -1;
}

GLint Shader::uniformBlockLocation(const string &name)
{
  map<string, GLint>::iterator it = uniformBlockLocations_.find(name);
  return (it != uniformBlockLocations_.end()) ? it->second :  -1;
}

GLint Shader::id() const
{
  return *(id_.get());
}

GLboolean Shader::compile()
{
  for(map<GLenum, string>::const_iterator
      it = shaderCodes_.begin(); it != shaderCodes_.end(); ++it)
  {
    const char* source = it->second.c_str();
    ref_ptr<GLuint> shaderStage = ref_ptr<GLuint>::alloc();
    GLuint &stage = *shaderStage.get();
    stage = glCreateShader(it->first);
    GLint length = -1;
    GLint status;

    glShaderSource(stage, 1, &source, &length);
    glCompileShader(stage);

    glGetShaderiv(stage, GL_COMPILE_STATUS, &status);
    printLog(stage, it->first, source, status!=0);
    if (!status) {
      glDeleteShader(stage);
      return GL_FALSE;
    }

    glAttachShader(id(), stage);
    shaders_[it->first] = shaderStage;
  }

  return GL_TRUE;
}

GLboolean Shader::link()
{
  if(!transformFeedback_.empty()) {
    vector<const char*> validNames(transformFeedback_.size());
    set<string> validNames_;
    int validCounter = 0;

    GLenum stage = feedbackStage_;
    GLuint i=0;
    for(; glenum::glslStages()[i]!=stage; ++i) {}

    // find next stage
    string nextStagePrefix = "out";
    for(GLint j=i+1; j<glenum::glslStageCount(); ++j)
    {
      GLenum nextStage = glenum::glslStages()[j];
      if(shaders_.count(nextStage)!=0) {
        nextStagePrefix = glenum::glslStagePrefix(nextStage);
        break;
      }
    }

    for(list<string>::const_iterator
        it=transformFeedback_.begin(); it!=transformFeedback_.end(); ++it)
    {
      string name = IOProcessor::getNameWithoutPrefix(*it);
      if(name == "Position") {
        name = "gl_" + name;
      } else {
        name = nextStagePrefix + "_" + name;
      }
      if(validNames_.count(name)>0) { continue; }
      validNames_.insert(name);
      validNames[validCounter] = name.c_str();
      ++validCounter;
    }

    glTransformFeedbackVaryings(id(),
        validCounter, validNames.data(), feedbackLayout_);
  }

  glLinkProgram(id());
  GLint status;
  glGetProgramiv(id(), GL_LINK_STATUS,  &status);
  GL_ERROR_LOG();
  if(status == GL_FALSE) {
    printLog(id(), GL_NONE, NULL, false);
    return GL_FALSE;
  } else {
    printLog(id(), GL_NONE, NULL, true);
    setupInputLocations();
    return GL_TRUE;
  }
}

GLboolean Shader::validate()
{
  glValidateProgram(id());
  GLint status;
  glGetProgramiv(id(), GL_VALIDATE_STATUS,  &status);
  if(status == GL_FALSE) {
    int length;
    glGetProgramiv(id(), GL_INFO_LOG_LENGTH, &length);
    char *log = new char[length];
    glGetProgramInfoLog(id(), length, NULL, log);
    REGEN_WARN("validation failed: " << log);
    delete []log;
    return GL_FALSE;
  }
  else {
    return GL_TRUE;
  }
}

void Shader::setupInputLocations()
{
  GLint count;
  GLint arraySize;
  GLenum type;
  char nameC[320];

  //inputs_.clear();
  samplerLocations_.clear();

  glGetProgramiv(id(), GL_ACTIVE_UNIFORMS, &count);
  for(GLint loc_=0; loc_<count; ++loc_)
  {
    glGetActiveUniform(id(), loc_, 320, NULL, &arraySize, &type, nameC);
    string uniformName(nameC);
    // for arrays..
    GLint loc = glGetUniformLocation(id(), nameC);

    // remember this uniform location
    string attName(nameC);
    if(hasPrefix(attName, "gl_")) { continue; }
    if(boost::ends_with(uniformName,"[0]")) {
      uniformName = uniformName.substr(0,uniformName.size()-3);
    }
    if (hasPrefix(uniformName, "u_")) {
      uniformName = truncPrefix(uniformName, "u_");
    } else if (hasPrefix(attName, "in_")) {
      uniformName = truncPrefix(uniformName, "in_");
    }
    uniformLocations_[string(nameC)] = loc;
    uniformLocations_[uniformName] = loc;
    uniformLocations_[REGEN_STRING("u_"<<uniformName)] = loc;
    uniformLocations_[REGEN_STRING("in_"<<uniformName)] = loc;

    // create ShaderInput without data allocated.
    // still setupInput must be called with this ShaderInput
    // for the shader to enable it with applyInputs()
    switch(type) {
    case GL_FLOAT:
    case GL_FLOAT_VEC2:
    case GL_FLOAT_VEC3:
    case GL_FLOAT_VEC4:
    case GL_BOOL:
    case GL_INT:
    case GL_BOOL_VEC2:
    case GL_INT_VEC2:
    case GL_BOOL_VEC3:
    case GL_INT_VEC3:
    case GL_BOOL_VEC4:
    case GL_INT_VEC4:
    case GL_FLOAT_MAT2:
    case GL_FLOAT_MAT3:
    case GL_FLOAT_MAT4:
      break;

    case GL_SAMPLER_BUFFER:
    case GL_SAMPLER_1D:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE:
    case GL_SAMPLER_2D_ARRAY:
    case GL_SAMPLER_1D_SHADOW:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_CUBE_SHADOW:
    case GL_SAMPLER_2D_ARRAY_SHADOW:
      samplerLocations_[uniformName] = loc;
      break;

    default:
      REGEN_WARN("unknown shader type for '" << uniformName << "'");
      break;

    }
  }

  glGetProgramiv(id(), GL_ACTIVE_UNIFORM_BLOCKS, &count);
  for(GLint loc_=0; loc_<count; ++loc_)
  {
    // Note: uniforms inside a uniform block do not have individual uniform locations
    glGetActiveUniformBlockName(id(), loc_, 320, &arraySize, nameC);
    string blockName(nameC);
    uniformBlockLocations_[blockName] = loc_;
  }

  glGetProgramiv(id(), GL_ACTIVE_ATTRIBUTES, &count);
  for(GLint loc_=0; loc_<count; ++loc_)
  {
    glGetActiveAttrib(id(), loc_, 320, NULL, &arraySize, &type, nameC);
    GLint loc = glGetAttribLocation(id(),nameC);

    // remember this attribute location
    string attName(nameC);
    if(hasPrefix(attName, "gl_")) { continue; }
    if(boost::ends_with(attName,"[0]")) {
      attName = attName.substr(0,attName.size()-3);
    }
    if (hasPrefix(attName, "a_")) {
      attName = truncPrefix(attName, "a_");
    } else if (hasPrefix(attName, "vs_")) {
      attName = truncPrefix(attName, "vs_");
    } else if (hasPrefix(attName, "in_")) {
      attName = truncPrefix(attName, "in_");
    }
    uniformLocations_[string(nameC)] = loc;
    attributeLocations_[attName] = loc;
    attributeLocations_[REGEN_STRING("a_"<<attName)] = loc;
    attributeLocations_[REGEN_STRING("in_"<<attName)] = loc;
    attributeLocations_[REGEN_STRING("vs_"<<attName)] = loc;
  }
}

ref_ptr<ShaderInput> Shader::createUniform(const string &name)
{
  GLint loc = uniformLocation(name);
  if(loc==-1) {
    REGEN_WARN("Is not an active uniform '" << name << "' shader=" << id());
    return ref_ptr<ShaderInput>();
  }

  GLint arraySize;
  GLenum type;
  char nameC[320];
  glGetActiveUniform(id(), loc, 320, NULL, &arraySize, &type, nameC);

  switch(type) {
  case GL_FLOAT:
    return ref_ptr<ShaderInput1f>::alloc(name,arraySize);
  case GL_FLOAT_VEC2:
    return ref_ptr<ShaderInput2f>::alloc(name,arraySize);
  case GL_FLOAT_VEC3:
    return ref_ptr<ShaderInput3f>::alloc(name,arraySize);
  case GL_FLOAT_VEC4:
    return ref_ptr<ShaderInput4f>::alloc(name,arraySize);
  case GL_BOOL:
  case GL_INT:
    return ref_ptr<ShaderInput1i>::alloc(name,arraySize);
  case GL_BOOL_VEC2:
  case GL_INT_VEC2:
    return ref_ptr<ShaderInput2i>::alloc(name,arraySize);
  case GL_BOOL_VEC3:
  case GL_INT_VEC3:
    return ref_ptr<ShaderInput3i>::alloc(name,arraySize);
  case GL_BOOL_VEC4:
  case GL_INT_VEC4:
    return ref_ptr<ShaderInput4i>::alloc(name,arraySize);
  case GL_FLOAT_MAT2:
    return ref_ptr<ShaderInput4f>::alloc(name,arraySize);
  case GL_FLOAT_MAT3:
    return ref_ptr<ShaderInputMat3>::alloc(name,arraySize);
  case GL_FLOAT_MAT4:
    return ref_ptr<ShaderInputMat4>::alloc(name,arraySize);
  default:
    REGEN_WARN("Not a known uniform type for '" << name << "' type=" << type);
    break;
  }
  return ref_ptr<ShaderInput>();
}

void Shader::setInput(const ref_ptr<ShaderInput> &in, const string &name)
{
  string inputName = (name.empty() ? in->name() : name);

  inputs_[inputName] = in;

  if(in->isVertexAttribute()) {
    map<string,GLint>::iterator needle = attributeLocations_.find(inputName);
    if(!in->hasData()) { return; }
    if(needle!=attributeLocations_.end()) {
      attributes_.push_back(ShaderInputLocation(in,needle->second));
    }
  }
  else if (!in->isConstant()) {
    map<string,GLint>::iterator needle = uniformLocations_.find(inputName);
    if(needle!=uniformLocations_.end()) {
      uniforms_.push_back(ShaderInputLocation(in,needle->second));
    }
  }
}
GLboolean Shader::setTexture(const ref_ptr<Texture> &tex, const string &name)
{
  map<string,GLint>::iterator needle = samplerLocations_.find(name);
  if(needle==samplerLocations_.end()) return GL_FALSE;
  if(tex.get()) {
    textures_[needle->second] = ShaderTextureLocation(name,tex,needle->second);
  }
  else {
    textures_.erase(needle->second);
  }
  return GL_TRUE;
}

void Shader::setInputs(const map<string, ref_ptr<ShaderInput> > &inputs)
{
  for(map<string, ref_ptr<ShaderInput> >::const_iterator
      it=inputs.begin(); it!=inputs.end(); ++it)
  {
    setInput(it->second, it->first);
  }
}

void Shader::setTransformFeedback(const list<string> &transformFeedback,
    GLenum attributeLayout, GLenum feedbackStage)
{
  feedbackLayout_ = attributeLayout;
  feedbackStage_ = feedbackStage;
  for(list<string>::const_iterator
      it=transformFeedback.begin(); it!=transformFeedback.end(); ++it)
  {
    transformFeedback_.push_back( *it );
  }
}

//////////////

void Shader::enable(RenderState *rs)
{
  for(list<ShaderInputLocation>::iterator
      it=uniforms_.begin(); it!=uniforms_.end(); ++it)
  {
    if(it->input->stamp() != it->uploadStamp && it->input->active()) {
      it->input->enableUniform(it->location);
      it->uploadStamp = it->input->stamp();
    }
  }
}
