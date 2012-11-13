/*
 * glsl-io-processor.cpp
 *
 *  Created on: 29.10.2012
 *      Author: daniel
 */

#define NO_REGEX_MATCH boost::sregex_iterator()

#include <ogle/utility/string-util.h>
#include "glsl-io-processor.h"

const GLenum GLSLInputOutputProcessor::shaderPipeline[] = {
    GL_VERTEX_SHADER,
    GL_TESS_CONTROL_SHADER,
    GL_TESS_EVALUATION_SHADER,
    GL_GEOMETRY_SHADER,
    GL_FRAGMENT_SHADER
};
const string GLSLInputOutputProcessor::shaderPipelinePrefixes[] = {
    "vs",
    "tcs",
    "tes",
    "gs",
    "fs"
};
const GLint GLSLInputOutputProcessor::pipelineSize =
    sizeof(shaderPipeline)/sizeof(GLenum);

const string& GLSLInputOutputProcessor::getPrefix(GLenum stage)
{
  for(int i=0; i<GLSLInputOutputProcessor::pipelineSize; ++i) {
    if(GLSLInputOutputProcessor::shaderPipeline[i] == stage) {
      return GLSLInputOutputProcessor::shaderPipelinePrefixes[i];
    }
  }
  static const string unk="unknown";
  return unk;
}

///////////////////////

GLSLInputOutput::GLSLInputOutput()
: layout(""),
  ioType(""),
  dataType(""),
  name(""),
  numElements(""),
  value("")
{}
GLSLInputOutput::GLSLInputOutput(const GLSLInputOutput &other)
: layout(other.layout),
  ioType(other.ioType),
  dataType(other.dataType),
  name(other.name),
  numElements(other.numElements),
  value(other.value)
{}
string GLSLInputOutput::declaration(GLenum stage)
{
  stringstream ss;
  if(!layout.empty()) { ss << layout << " "; }
  ss << ioType << " " << dataType << " " << name;
  if(!numElements.empty()) { ss << "[" << numElements << "]"; }
  if(!value.empty()) { ss << " = " << value; }
  ss << ";";
  return ss.str();
}

//////////////////

GLSLInputOutputProcessor::GLSLInputOutputProcessor(
    istream &in,
    GLenum stage,
    GLenum nextStage,
    const map<string,GLSLInputOutput> &nextStageInputs,
    const map<string, ref_ptr<ShaderInput> > &specifiedInput)
: in_(in),
  stage_(stage),
  nextStage_(nextStage),
  nextStageInputs_(nextStageInputs),
  specifiedInput_(specifiedInput),
  wasEmpty_(GL_TRUE)
{
}

string GLSLInputOutputProcessor::getNameWithoutPrefix(const string &name)
{
  static const string prefixes[] = {"in_","out_","u_","c_","gs_","fs_","vs_","tes_","tcs_"};
  static const int numprefixes = sizeof(prefixes)/sizeof(string);
  for(int i=0; i<numprefixes; ++i) {
    if(hasPrefix(name, prefixes[i])) {
      return truncPrefix(name, prefixes[i]);
    }
  }
  return name;
}

map<string,GLSLInputOutput>& GLSLInputOutputProcessor::outputs()
{
  return outputs_;
}
map<string,GLSLInputOutput>& GLSLInputOutputProcessor::inputs()
{
  return inputs_;
}

void GLSLInputOutputProcessor::defineHandleIO()
{
  list<GLSLInputOutput> genOut, genIn;

  // for each input of the next stage
  // make sure it is declared at least as output in this stage
  for(map<string,GLSLInputOutput>::const_iterator
      it=nextStageInputs_.begin(); it!=nextStageInputs_.end(); ++it)
  {
    const string &nameWithoutPrefix = it->first;
    const GLSLInputOutput &nextIn = it->second;

    if(outputs_.count(nameWithoutPrefix)>0) { continue; }
    genOut.push_back(GLSLInputOutput(nextIn));
    genOut.back().name = "out_" + nameWithoutPrefix;
    genOut.back().ioType = "out";
    outputs_[nameWithoutPrefix] = genOut.back();
    if(stage_==GL_TESS_EVALUATION_SHADER) {
      genOut.back().numElements = "";
    } else if(stage_==GL_TESS_CONTROL_SHADER) {
      genOut.back().numElements = " ";
    } else if(stage_==GL_GEOMETRY_SHADER) {
      genOut.back().numElements = "";
    } else if(stage_==GL_VERTEX_SHADER) {
      genOut.back().numElements = "";
    }

    if(inputs_.count(nameWithoutPrefix)>0) { continue; }
    genIn.push_back(GLSLInputOutput(nextIn));
    genIn.back().name = "in_" + nameWithoutPrefix;
    genIn.back().ioType = "in";
    if(stage_==GL_TESS_EVALUATION_SHADER) {
      genIn.back().numElements = " ";
    } else if(stage_==GL_TESS_CONTROL_SHADER) {
      genIn.back().numElements = " ";
    } else if(stage_==GL_GEOMETRY_SHADER) {
      genIn.back().numElements = " ";
    } else if(stage_==GL_VERTEX_SHADER) {
      genIn.back().numElements = "";
    }
    inputs_[nameWithoutPrefix] = genIn.back();
  }

  if(genOut.empty() && genIn.empty()) {
    lineQueue_.push_back("#define HANDLE_IO(i)");
    return;
  }

  // declare IO:
  //    * insert a redefinition of the IO name using the stage prefix
  //    * just insert the previous declaration again
  for(list<GLSLInputOutput>::iterator it=genIn.begin(); it!=genIn.end(); ++it) {
    lineQueue_.push_back("#define " + (*it).name + " " +
        getPrefix(stage_) + "_" + getNameWithoutPrefix((*it).name));
    lineQueue_.push_back(it->declaration(stage_));
  }
  for(list<GLSLInputOutput>::iterator it=genOut.begin(); it!=genOut.end(); ++it) {
    lineQueue_.push_back("#define " + (*it).name + " " +
        getPrefix(nextStage_) + "_" + getNameWithoutPrefix((*it).name));
    lineQueue_.push_back(it->declaration(stage_));
  }

  // declare HANDLE_IO() function
  lineQueue_.push_back("void HANDLE_IO(int i) {");
  for(list<GLSLInputOutput>::iterator it=genOut.begin(); it!=genOut.end(); ++it) {
    GLSLInputOutput &io = *it;
    const string &outName = io.name;
    string inName = inputs_[getNameWithoutPrefix(outName)].name;

    switch(stage_) {
    case GL_VERTEX_SHADER:
      lineQueue_.push_back(FORMAT_STRING(
          "    " << outName << " = " << inName << ";"));
      break;
    case GL_TESS_CONTROL_SHADER:
      lineQueue_.push_back(FORMAT_STRING(
          "    " << outName << "[ID] = " << inName << "[ID];"));
      break;
    case GL_TESS_EVALUATION_SHADER:
      lineQueue_.push_back(FORMAT_STRING(
          "    " << outName << " = INTERPOLATE_VALUE(" << inName << ");"));
      break;
    case GL_GEOMETRY_SHADER:
      lineQueue_.push_back(FORMAT_STRING(
          "    " << outName << " = " << inName << "[index];"));
      break;
    case GL_FRAGMENT_SHADER:
      break;
    }

  }
  lineQueue_.push_back("}");
}

void GLSLInputOutputProcessor::parseValue(string &v, string &val)
{
  static const char* pattern_ = "[ ]*([^= ]+)[ ]*=[ ]*([^ ]+)[ ]*";
  static boost::regex regex_(pattern_);

  boost::sregex_iterator it(v.begin(), v.end(), regex_);
  if(it != NO_REGEX_MATCH) {
    v = (*it)[1];
    val = (*it)[2];
  }
}

void GLSLInputOutputProcessor::parseArray(string &v, string &numElements)
{
  static const char* pattern_ = "([^[]+)\\[([^\\]]*)\\]";
  static boost::regex regex_(pattern_);

  boost::sregex_iterator it(v.begin(), v.end(), regex_);
  if(it != NO_REGEX_MATCH) {
    v = (*it)[1];
    numElements = (*it)[2];
  }
}

bool GLSLInputOutputProcessor::getline(string &line)
{
  static const char* pattern_ =
      "^[ |\t|]*((in|uniform|const|out)[ |\t]+([^ ]*)[ |\t]+([^;]+);)$";
  static boost::regex regex_(pattern_);
  static const char* handleIOPattern_ =
      "^[ |\t]*#define[ |\t]+HANDLE_IO[ |\t]*";
  static boost::regex handleIORegex_(handleIOPattern_);

  if(!lineQueue_.empty()) {
    line = lineQueue_.front();
    lineQueue_.pop_front();
    return true;
  }

  // read a line from the input stream
  if(!std::getline(in_, line)) { return false; }

  boost::sregex_iterator it;
  it = boost::sregex_iterator(line.begin(), line.end(), handleIORegex_);
  if(it!=NO_REGEX_MATCH) {
    defineHandleIO();
    return GLSLInputOutputProcessor::getline(line);
  }

  GLboolean isEmpty = line.empty();
  if(isEmpty && wasEmpty_) {
    return GLSLInputOutputProcessor::getline(line);
  }
  wasEmpty_ = isEmpty;

  it = boost::sregex_iterator(line.begin(), line.end(), regex_);
  if(it==NO_REGEX_MATCH) {
    return true;
  }

  GLSLInputOutput io;
  io.ioType = (*it)[2];
  io.dataType = (*it)[3];
  io.name = (*it)[4];
  io.numElements = "";
  io.value = "";
  io.layout = "";

  parseArray(io.dataType,io.numElements);
  parseValue(io.name,io.value);
  parseArray(io.name,io.numElements);

  string nameWithoutPrefix = getNameWithoutPrefix(io.name);

  map<string, ref_ptr<ShaderInput> >::const_iterator needle = specifiedInput_.find(nameWithoutPrefix);
  if(needle != specifiedInput_.end()) {
    // change declaration based on specified input
    const ref_ptr<ShaderInput> &in = needle->second;
    if(in->isVertexAttribute()) {
      if(io.ioType != "out") {
        io.ioType = "in";
      }
      io.value = "";
    }
    else if (in->isConstant()) {
      io.ioType = "const";

      stringstream val;
      val << io.dataType << "(";
      (*in.get()) >> val;
      val << ")";
      io.value = val.str();
    }
    else {
      io.ioType = "uniform";
      io.value = "";
    }
  }

  if(io.ioType == "in") {
    // define input name with matching prefix
    line = "#define " + io.name + " " + getPrefix(stage_) + "_" + nameWithoutPrefix;
    lineQueue_.push_back(io.declaration(stage_));
  }
  else if(io.ioType == "out" && nextStage_ != GL_NONE) {
    // define output name with matching prefix
    line = "#define " + io.name + " " + getPrefix(nextStage_) + "_" + nameWithoutPrefix;
    lineQueue_.push_back(io.declaration(stage_));
  }
  else {
    line = io.declaration(stage_);
  }

  if(io.ioType == "out") {
    outputs_[nameWithoutPrefix] = io;
  }
  else if(io.ioType == "in") {
    inputs_[nameWithoutPrefix] = io;
  }

  return true;
}

void GLSLInputOutputProcessor::preProcess(ostream &out)
{
  string line;
  while(GLSLInputOutputProcessor::getline(line)) { out << line << endl; }
}
