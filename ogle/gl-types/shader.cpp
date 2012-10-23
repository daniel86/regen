/*
 * shader.cpp
 *
 *  Created on: 26.03.2011
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>

#include "shader.h"
#include "texture.h"
#include <ogle/utility/string-util.h>
#include <ogle/utility/gl-error.h>

Shader::Shader(Shader &other)
: numInstances_(other.numInstances_),
  shaderCodes_(other.shaderCodes_),
  shaders_(other.shaders_),
  isLineShader_(other.isLineShader_),
  isPointShader_(other.isPointShader_),
  id_(other.id_)
{
}
Shader::Shader(const map<GLenum, string> &shaderCodes)
: numInstances_(1),
  shaderCodes_(shaderCodes),
  isLineShader_(GL_FALSE),
  isPointShader_(GL_FALSE)
{
  id_ = ref_ptr<GLuint>::manage(new GLuint);
  *(id_.get()) = glCreateProgram();
}
Shader::~Shader()
{
  if(*id_.refCount()==1) {
    for(map<GLenum, GLuint>::const_iterator
        it = shaders_.begin(); it != shaders_.end(); ++it)
    {
      glDeleteShader(it->second);
    }
    glDeleteProgram(*(id_.get()));
  }
}

GLboolean Shader::isPointShader() const
{
  return isPointShader_;
}
void Shader::set_isPointShader(GLboolean isPointShader)
{
  isPointShader_ = isPointShader;
}

GLboolean Shader::isLineShader() const
{
  return isLineShader_;
}
void Shader::set_isLineShader(GLboolean isLineShader)
{
  isLineShader_ = isLineShader;
}

bool Shader::hasShader(GLenum stage) const
{
  return shaders_.count(stage)>0;
}
const string& Shader::shaderCode(GLenum stage) const
{
  map<GLenum, string>::const_iterator it = shaderCodes_.find(stage);
  if(it!=shaderCodes_.end()) {
    return it->second;
  } else {
    static const string empty = "";
    return empty;
  }
}

const GLuint& Shader::shader(GLenum stage) const
{
  map<GLenum, GLuint>::const_iterator it = shaders_.find(stage);
  if(it!=shaders_.end()) {
    return it->second;
  } else {
    static const GLuint empty = 0;
    return empty;
  }
}
void Shader::setShaders(const map<GLenum, GLuint> &shaders)
{
  for(map<GLenum, GLuint>::const_iterator
      it = shaders_.begin(); it != shaders_.end(); ++it)
  {
    glAttachShader(id(), 0);
    glDeleteShader(it->second);
  }
  shaders_.clear();
  for(map<GLenum, GLuint>::const_iterator
      it = shaders.begin(); it != shaders.end(); ++it)
  {
    glAttachShader(id(), it->second);
    shaders_[it->first] = it->second;
  }
}

const map<string, ref_ptr<ShaderInput> >& Shader::inputs() const
{
  return inputs_;
}
GLboolean Shader::isUniform(const string &name) const
{
  return inputs_.count(name)>0;
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
void Shader::set_input(const string &name, ref_ptr<ShaderInput> &in)
{
  if(uniformLocations_.count(name)>0) {
    GLint loc = uniformLocations_[name];
    uniforms_.push_back(ShaderInputLocation(in,loc));
    inputs_[name] = in;
  }
}

GLboolean Shader::isSampler(const string &name) const
{
  return samplerLocations_.count(name)>0;
}

GLint Shader::samplerLocation(const string &name)
{
  return samplerLocations_[name];
}
GLint Shader::attributeLocation(const string &name)
{
  return attributeLocations_[name];
}

GLint Shader::id() const
{
  return *(id_.get());
}

bool Shader::compile()
{
  for(map<GLenum, string>::const_iterator
      it = shaderCodes_.begin(); it != shaderCodes_.end(); ++it)
  {
    const char* source = it->second.c_str();
    GLuint shaderStage = glCreateShader(it->first);
    GLint length = -1;
    GLint status;

    glShaderSource(shaderStage, 1, &source, &length);
    glCompileShader(shaderStage);

    glGetShaderiv(shaderStage, GL_COMPILE_STATUS, &status);
    if (!status) {
      printLog(shaderStage, it->first, source, false);
      glDeleteShader(shaderStage);
      return false;
    }
    //if(Logging::verbosity() > Logging::_) {
    //  printLog(shaderStage, it->first, source, true);
    //}

    glAttachShader(id(), shaderStage);
    shaders_[it->first] = shaderStage;
  }
  return true;
}

bool Shader::link()
{
  glLinkProgram(id());
  GLint status;
  glGetProgramiv(id(), GL_LINK_STATUS,  &status);
  if(status == GL_FALSE) {
    printLog(id(), GL_VERTEX_SHADER, NULL, false);
    handleGLError("after glLinkProgram");
    return false;
  } else {
    return true;
  }
}

void Shader::printLog(
    GLuint shader,
    GLenum shaderType,
    const char *shaderCode,
    GLboolean success)
{
  Logging::LogLevel logLevel;
  if(shaderCode != NULL) {
    const char* shaderName;
    switch(shaderType) {
    case GL_VERTEX_SHADER:   shaderName = "Vertex"; break;
    case GL_GEOMETRY_SHADER: shaderName = "Geometry"; break;
    case GL_FRAGMENT_SHADER: shaderName = "Fragment"; break;
    case GL_TESS_CONTROL_SHADER: shaderName = "TessControl"; break;
    case GL_TESS_EVALUATION_SHADER: shaderName = "TessEval"; break;
    }

    if(success) {
      logLevel = Logging::INFO;
      LOG_MESSAGE(logLevel, shaderName << " Shader compiled successfully!");
    } else {
      logLevel = Logging::ERROR;
      LOG_MESSAGE(logLevel, shaderName << " Shader failed to compile!");
    }
  } else {
    logLevel = Logging::ERROR;
    LOG_MESSAGE(logLevel, "Shader failed to link!");
  }

  if(shaderCode != NULL) {
    vector<string> codeLines;
    boost::split(codeLines, shaderCode, boost::is_any_of("\n"));
    for(GLuint i=0; i<codeLines.size(); ++i) {
      LOG_MESSAGE(logLevel,
          setw(3) << i << setw(0) << " " << codeLines[i]);
    }
  }

  int length;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
  if(length>0) {
    char log[length];
    glGetShaderInfoLog(shader, length, NULL, log);
    LOG_MESSAGE(logLevel, "shader info: " << log);
  } else {
    LOG_MESSAGE(logLevel, "shader info: empty");
  }

  if(length > 0) {
  }
}

void Shader::setupOutputs(
    const list<ShaderOutput> &outputs)
{
  for(list<ShaderOutput>::const_iterator
      it=outputs.begin(); it!=outputs.end(); ++it)
  {
    glBindFragDataLocation(
        id(),
        it->colorAttachment-GL_COLOR_ATTACHMENT0,
        it->name.c_str());
  }
  if(outputs.empty()) {
    glBindFragDataLocation(
        id(), 0, "defaultColorOutput");
  }
}

void Shader::setupTransformFeedback(
    const list<string> &tfAtts,
    GLenum attributeLayout)
{
  if(tfAtts.size()>0) {
    // specify the transform feedback output names
    vector<const char*> names(tfAtts.size());
    GLuint i=0;
    for(list<string>::const_iterator
        it=tfAtts.begin(); it!=tfAtts.end(); ++it)
    {
      names[i++] = it->c_str();
    }
    glTransformFeedbackVaryings(
        id(),
        tfAtts.size(),
        names.data(),
        attributeLayout);
  }
}

void Shader::setupInputLocations()
{
  GLint count;

  //inputs_.clear();
  samplerLocations_.clear();

  glGetProgramiv(id(), GL_ACTIVE_UNIFORMS, &count);
  for(GLint loc_=0; loc_<count; ++loc_)
  {
    GLint arraySize;
    GLenum type;
    char nameC[320];
    glGetActiveUniform(id(), loc_, 320, NULL, &arraySize, &type, nameC);
    string uniformName(nameC);
    // for arrays..
    GLint loc = glGetUniformLocation(id(), nameC);

    // remember this uniform location
    string attName(nameC);
    if (hasPrefix(attName, "gl_")) {
      attributeLocations_[attName] = loc;
      continue;
    }
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
    uniformLocations_[FORMAT_STRING("u_"<<uniformName)] = loc;
    uniformLocations_[FORMAT_STRING("in_"<<uniformName)] = loc;

    // create ShaderInput without data allocated.
    // still setupInput must be called with this ShaderInput
    // for the shader to enable it with applyInputs()
    switch(type) {
    case GL_FLOAT:
      inputs_[uniformName] = ref_ptr<ShaderInput>::manage(
          new ShaderInput1f(uniformName,arraySize));
      break;
    case GL_FLOAT_VEC2:
      inputs_[uniformName] = ref_ptr<ShaderInput>::manage(
          new ShaderInput2f(uniformName,arraySize));
      break;
    case GL_FLOAT_VEC3:
      inputs_[uniformName] = ref_ptr<ShaderInput>::manage(
          new ShaderInput3f(uniformName,arraySize));
      break;
    case GL_FLOAT_MAT2:
    case GL_FLOAT_VEC4:
      inputs_[uniformName] = ref_ptr<ShaderInput>::manage(
          new ShaderInput4f(uniformName,arraySize));
      break;
    case GL_BOOL:
    case GL_INT:
      inputs_[uniformName] = ref_ptr<ShaderInput>::manage(
          new ShaderInput1i(uniformName,arraySize));
      break;
    case GL_BOOL_VEC2:
    case GL_INT_VEC2:
      inputs_[uniformName] = ref_ptr<ShaderInput>::manage(
          new ShaderInput2i(uniformName,arraySize));
      break;
    case GL_BOOL_VEC3:
    case GL_INT_VEC3:
      inputs_[uniformName] = ref_ptr<ShaderInput>::manage(
          new ShaderInput3i(uniformName,arraySize));
      break;
    case GL_BOOL_VEC4:
    case GL_INT_VEC4:
      inputs_[uniformName] = ref_ptr<ShaderInput>::manage(
          new ShaderInput4i(uniformName,arraySize));
      break;

    case GL_FLOAT_MAT3:
      inputs_[uniformName] = ref_ptr<ShaderInput>::manage(
          new ShaderInputMat3(uniformName,arraySize));
      break;
    case GL_FLOAT_MAT4:
      inputs_[uniformName] = ref_ptr<ShaderInput>::manage(
          new ShaderInputMat4(uniformName,arraySize));
      break;

    case GL_SAMPLER_1D:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE:
      samplerLocations_[uniformName] = loc;
      break;

    default:
      break;

    }
  }

  glGetProgramiv(id(), GL_ACTIVE_ATTRIBUTES, &count);
  for(GLint loc_=0; loc_<count; ++loc_)
  {
    GLint arraySize;
    GLenum type;
    char nameC[320];

    glGetActiveAttrib(id(), loc_, 320, NULL, &arraySize, &type, nameC);
    GLint loc = glGetAttribLocation(id(),nameC);

    // remember this attribute location
    string attName(nameC);
    if (hasPrefix(attName, "gl_")) {
      attributeLocations_[attName] = loc;
      continue;
    }
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
    attributeLocations_[FORMAT_STRING("a_"<<attName)] = loc;
    attributeLocations_[FORMAT_STRING("in_"<<attName)] = loc;
    attributeLocations_[FORMAT_STRING("vs_"<<attName)] = loc;
  }
}

void Shader::setupInput(const ref_ptr<ShaderInput> &in)
{
  if(in->isVertexAttribute())
  {
    if(in->numInstances()>1)
    {
      if(numInstances_==1)
      {
        numInstances_ = in->numInstances();
      }
      else if(numInstances_ != in->numInstances())
      {
        WARN_LOG("incompatible number of instance for " << in->name() << "."
            << " Excpected is '" << numInstances_ << "' but actual value is '"
            << in->numInstances() << "'.")
      }
    }

    map<string,GLint>::iterator needle = attributeLocations_.find(in->name());
    if(needle!=attributeLocations_.end()) {
      attributes_.push_back(ShaderInputLocation(in,needle->second));
    }
  }
  else if (!in->isConstant()) {
    map<string,GLint>::iterator needle = uniformLocations_.find(in->name());
    if(needle!=uniformLocations_.end()) {
      uniforms_.push_back(ShaderInputLocation(in,needle->second));
    }
  }
}

void Shader::applyInputs()
{
  for(list<ShaderInputLocation>::iterator
      it=attributes_.begin(); it!=attributes_.end(); ++it)
  {
    it->input->enableAttribute( it->location );
  }
  for(list<ShaderInputLocation>::iterator
      it=uniforms_.begin(); it!=uniforms_.end(); ++it)
  {
    it->input->enableUniform( it->location );
  }
}

//////////////

GLuint Shader::numInstances() const
{
  return numInstances_;
}

void Shader::setupInputs(
    const map<string, ref_ptr<ShaderInput> > &inputs)
{
  numInstances_ = 1;
  for(map<string, ref_ptr<ShaderInput> >::const_iterator
      it=inputs.begin(); it!=inputs.end(); ++it)
  {
    const ref_ptr<ShaderInput> &in = it->second;
    setupInput(in);
  }
  inputs_ = inputs;
}

void Shader::applyTexture(const ShaderTexture &d)
{
  map<string,GLint>::iterator needle = uniformLocations_.find(d.tex->name());
  if(needle!=uniformLocations_.end()) {
    glUniform1i( needle->second, d.texUnit );
  }
}

void Shader::applyAttribute(const ShaderInput *input)
{
  map<string,GLint>::iterator needle = attributeLocations_.find(input->name());
  if(needle!=attributeLocations_.end()) {
    input->enableAttribute( needle->second );
  }
}

void Shader::applyUniform(const ShaderInput *input)
{
  map<string,GLint>::iterator needle = uniformLocations_.find(input->name());
  if(needle!=uniformLocations_.end()) {
    input->enableUniform( needle->second );
  }
}
