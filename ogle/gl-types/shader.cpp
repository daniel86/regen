/*
 * shader.cpp
 *
 *  Created on: 26.03.2011
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>

#include "shader.h"
#include "uniform.h"
#include "texture.h"
#include <ogle/utility/string-util.h>

Shader::Shader()
{
  id_ = glCreateProgram();
}
Shader::~Shader()
{
  glDeleteProgram(id_);
}

GLint Shader::id() const
{
  return id_;
}

bool Shader::compile(const map<GLenum, string> &stages)
{
  for(map<GLenum, string>::const_iterator
      it = stages.begin(); it != stages.end(); ++it)
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
    if(Logging::verbosity() > Logging::_) {
      printLog(shaderStage, it->first, source, true);
    }

    glAttachShader(id_, shaderStage);
    glDeleteShader(shaderStage);
  }
  return true;
}

bool Shader::link()
{
  glLinkProgram(id_);
  GLint status;
  glGetProgramiv(id_, GL_LINK_STATUS,  &status);
  if(status == GL_FALSE) {
    printLog(id_, GL_VERTEX_SHADER, NULL, false);
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
}

void Shader::setupTransformFeedback(const list<string> &tfAtts)
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
        GL_SEPARATE_ATTRIBS);
  }
}

void Shader::setupLocations(
    const set<string> &attributeNames,
    const set<string> &uniformNames)
{
  for(set<string>::const_iterator
      it=attributeNames.begin(); it!=attributeNames.end(); ++it)
  {
    string attNameInShader;
    const string &attName = *it;
    if (boost::starts_with(attName, "v_")) {
      attNameInShader = attName;
    } else {
      attNameInShader = FORMAT_STRING("v_"<<attName.c_str());
    }
    attributeLocations_[attName] = glGetAttribLocation(id(), attName.c_str());
  }

  for(set<string>::const_iterator
      it=uniformNames.begin(); it!=uniformNames.end(); ++it)
  {
    uniformLocations_[*it] = glGetUniformLocation(id(), it->c_str());
  }
}

void Shader::applyTexture(const ShaderTexture &d)
{
  glUniform1i( uniformLocations_[d.tex->name()], d.texUnit );
}

void Shader::applyAttribute(const VertexAttribute *attribute)
{
  attribute->enable( attributeLocations_[attribute->name()] );
}

void Shader::applyUniform(const Uniform *uniform)
{
  uniform->apply( uniformLocations_[uniform->name()] );
}
