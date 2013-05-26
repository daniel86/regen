/*
 * glsl-io-processor.cpp
 *
 *  Created on: 29.10.2012
 *      Author: daniel
 */

#define NO_REGEX_MATCH boost::sregex_iterator()

#include <regen/utility/logging.h>
#include <regen/utility/string-util.h>
#include <regen/gl-types/gl-enum.h>
#include "io-processor.h"
using namespace regen;

///////////////////////

IOProcessor::InputOutput::InputOutput()
: layout(""),
  interpolation(""),
  ioType(""),
  dataType(""),
  name(""),
  numElements(""),
  value("")
{}
IOProcessor::InputOutput::InputOutput(const InputOutput &other)
: layout(other.layout),
  interpolation(""),
  ioType(other.ioType),
  dataType(other.dataType),
  name(other.name),
  numElements(other.numElements),
  value(other.value)
{}
string IOProcessor::InputOutput::declaration(GLenum stage)
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

IOProcessor::IOProcessor()
: GLSLProcessor(),
  wasEmpty_(GL_TRUE)
{
}

string IOProcessor::getNameWithoutPrefix(const string &name)
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

void IOProcessor::defineHandleIO(PreProcessorState &state)
{
  list<InputOutput> genOut, genIn;
  map<string,InputOutput> &nextInputs = inputs_[state.nextStage];
  map<string,InputOutput> &inputs = inputs_[state.currStage];
  map<string,InputOutput> &outputs = outputs_[state.currStage];

  // for each input of the next stage
  // make sure it is declared at least as output in this stage
  for(map<string,InputOutput>::const_iterator
      it=nextInputs.begin(); it!=nextInputs.end(); ++it)
  {
    const string &nameWithoutPrefix = it->first;
    const InputOutput &nextIn = it->second;

    if(outputs.count(nameWithoutPrefix)>0) { continue; }
    genOut.push_back(InputOutput(nextIn));
    genOut.back().name = "out_" + nameWithoutPrefix;
    genOut.back().ioType = "out";
    if(state.currStage==GL_GEOMETRY_SHADER) {
      genOut.back().numElements = "";
    }
    else if(state.currStage==GL_VERTEX_SHADER) {
      genOut.back().numElements = "";
    }
#ifdef GL_TESS_EVALUATION_SHADER
    else if(state.currStage==GL_TESS_EVALUATION_SHADER) {
      genOut.back().numElements = "";
    }
#endif
#ifdef GL_TESS_CONTROL_SHADER
    else if(state.currStage==GL_TESS_CONTROL_SHADER) {
      genOut.back().numElements = " ";
    }
#endif
    outputs.insert(make_pair(nameWithoutPrefix,genOut.back()));

    if(inputs.count(nameWithoutPrefix)>0) { continue; }
    genIn.push_back(InputOutput(nextIn));
    genIn.back().name = "in_" + nameWithoutPrefix;
    genIn.back().ioType = "in";
    if(state.currStage==GL_GEOMETRY_SHADER) {
      genIn.back().numElements = " ";
    }
    else if(state.currStage==GL_VERTEX_SHADER) {
      genIn.back().numElements = "";
    }
#ifdef GL_TESS_EVALUATION_SHADER
    else if(state.currStage==GL_TESS_EVALUATION_SHADER) {
      genIn.back().numElements = " ";
    }
#endif
#ifdef GL_TESS_CONTROL_SHADER
    else if(state.currStage==GL_TESS_CONTROL_SHADER) {
      genIn.back().numElements = " ";
    }
#endif
    inputs.insert(make_pair(nameWithoutPrefix,genIn.back()));
  }

  if(genOut.empty() && genIn.empty()) {
    lineQueue_.push_back("#define HANDLE_IO(i)");
    return;
  }

  // declare IO:
  //    * insert a redefinition of the IO name using the stage prefix
  //    * just insert the previous declaration again
  for(list<InputOutput>::iterator it=genIn.begin(); it!=genIn.end(); ++it) {
    lineQueue_.push_back("#define " + (*it).name + " " +
        glenum::glslStagePrefix(state.currStage) + "_" + getNameWithoutPrefix((*it).name));
    lineQueue_.push_back(it->declaration(state.currStage));
  }
  for(list<InputOutput>::iterator it=genOut.begin(); it!=genOut.end(); ++it) {
    lineQueue_.push_back("#define " + (*it).name + " " +
        glenum::glslStagePrefix(state.nextStage) + "_" + getNameWithoutPrefix((*it).name));
    lineQueue_.push_back(it->declaration(state.currStage));
  }

  // declare HANDLE_IO() function
  lineQueue_.push_back("void HANDLE_IO(int i) {");
  for(list<InputOutput>::iterator it=genOut.begin(); it!=genOut.end(); ++it) {
    InputOutput &io = *it;
    const string &outName = io.name;
    string inName = inputs[getNameWithoutPrefix(outName)].name;

    switch(state.currStage) {
    case GL_VERTEX_SHADER:
      lineQueue_.push_back(REGEN_STRING(
          "    " << outName << " = " << inName << ";"));
      break;
    case GL_TESS_CONTROL_SHADER:
      lineQueue_.push_back(REGEN_STRING(
          "    " << outName << "[ID] = " << inName << "[ID];"));
      break;
    case GL_TESS_EVALUATION_SHADER:
      lineQueue_.push_back(REGEN_STRING(
          "    " << outName << " = INTERPOLATE_VALUE(" << inName << ");"));
      break;
    case GL_GEOMETRY_SHADER:
      lineQueue_.push_back(REGEN_STRING(
          "    " << outName << " = " << inName << "[i];"));
      break;
    case GL_FRAGMENT_SHADER:
      break;
    }

  }
  lineQueue_.push_back("}");
}

void IOProcessor::parseValue(string &v, string &val)
{
  static const char* pattern_ = "[ ]*([^= ]+)[ ]*=[ ]*([^ ]+)[ ]*";
  static boost::regex regex_(pattern_);

  boost::sregex_iterator it(v.begin(), v.end(), regex_);
  if(it != NO_REGEX_MATCH) {
    v = (*it)[1];
    val = (*it)[2];
  }
}

void IOProcessor::parseArray(string &v, string &numElements)
{
  static const char* pattern_ = "([^\\[]+)\\[([^\\]]*)\\]";
  static boost::regex regex_(pattern_);

  boost::sregex_iterator it(v.begin(), v.end(), regex_);
  if(it != NO_REGEX_MATCH) {
    v = (*it)[1];
    numElements = (*it)[2];
  }
}

void IOProcessor::clear()
{
  inputs_.clear();
  outputs_.clear();
  lineQueue_.clear();
}

bool IOProcessor::getline(PreProcessorState &state, string &line)
{
  static const char* interpolationPattern_ =
      "^[ |\t|]*((flat|noperspective|smooth|centroid)[ |\t]+(.*))$";
  static boost::regex interpolationRegex_(interpolationPattern_);
  static const char* pattern_ =
      "^[ |\t|]*((in|uniform|const|out)[ |\t]+([^ ]*)[ |\t]+([^;]+);)$";
  static boost::regex regex_(pattern_);
  static const char* handleIOPattern_ =
      "^[ |\t]*#define[ |\t]+HANDLE_IO[ |\t]*";
  static boost::regex handleIORegex_(handleIOPattern_);

  // read a line from the queue
  if(!lineQueue_.empty())
  {
    line = lineQueue_.front();
    lineQueue_.pop_front();
    REGEN_DEBUG("IOProcessor::getline out '" << line << "'");
    return true;
  }
  // read a line from the input stream
  if(!getlineParent(state, line))
  {
    return false;
  }

  boost::sregex_iterator it;
  it = boost::sregex_iterator(line.begin(), line.end(), handleIORegex_);
  if(it!=NO_REGEX_MATCH) {
    defineHandleIO(state);
    return IOProcessor::getline(state,line);
  }

  GLboolean isEmpty = line.empty();
  if(isEmpty && wasEmpty_) {
    return IOProcessor::getline(state,line);
  }
  wasEmpty_ = isEmpty;
  REGEN_DEBUG("IOProcessor::getline in  '" << line << "'");

  InputOutput io;
  it = boost::sregex_iterator(line.begin(), line.end(), interpolationRegex_);
  if(it==NO_REGEX_MATCH) {
    it = boost::sregex_iterator(line.begin(), line.end(), regex_);
  }
  else {
    // interpolation qualifier specified
    io.interpolation = (*it)[2];
    string nextLine = (*it)[3];
    it = boost::sregex_iterator(nextLine.begin(), nextLine.end(), regex_);
  }
  if(it==NO_REGEX_MATCH)
  {
    REGEN_DEBUG("IOProcessor::getline out '" << line << "'");
    return true;
  }

  io.ioType = (*it)[2];
  io.dataType = (*it)[3];
  io.name = (*it)[4];
  io.numElements = "";
  io.value = "";
  io.layout = "";

  REGEN_DEBUG("    _numElements=" << io.numElements);
  REGEN_DEBUG("    _name=" << io.name);
  REGEN_DEBUG("    _dataType=" << io.dataType);
  parseArray(io.dataType,io.numElements);
  REGEN_DEBUG("    _numElements0=" << io.numElements);
  REGEN_DEBUG("    _name0=" << io.name);
  parseValue(io.name,io.value);
  REGEN_DEBUG("    _numElements1=" << io.numElements);
  REGEN_DEBUG("    _name1=" << io.name);
  parseArray(io.name,io.numElements);
  REGEN_DEBUG("    _numElements2=" << io.numElements);
  REGEN_DEBUG("    _name2=" << io.name);

  string nameWithoutPrefix = getNameWithoutPrefix(io.name);

  map<string, ref_ptr<ShaderInput> >::const_iterator needle =
      state.in.specifiedInput.find(nameWithoutPrefix);
  if(needle != state.in.specifiedInput.end()) {
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
    line = "#define " + io.name + " " +
        glenum::glslStagePrefix(state.currStage) + "_" + nameWithoutPrefix;
    lineQueue_.push_back(io.declaration(state.currStage));
  }
  else if(io.ioType == "out" && state.nextStage != GL_NONE) {
    // define output name with matching prefix
    line = "#define " + io.name + " " +
        glenum::glslStagePrefix(state.nextStage) + "_" + nameWithoutPrefix;
    lineQueue_.push_back(io.declaration(state.currStage));
  }
  else {
    line = io.declaration(state.currStage);
  }

  if(io.ioType == "out") {
    outputs_[state.currStage].insert(
        make_pair(nameWithoutPrefix,io));
  }
  else if(io.ioType == "in") {
    inputs_[state.currStage].insert(
        make_pair(nameWithoutPrefix,io));
  }

  REGEN_DEBUG("IOProcessor::getline out '" << line << "'");
  REGEN_DEBUG("    ioType=" << io.ioType);
  REGEN_DEBUG("    dataType=" << io.dataType);
  REGEN_DEBUG("    name=" << io.name);
  REGEN_DEBUG("    numElements=" << io.numElements);
  REGEN_DEBUG("    value=" << io.value);
  REGEN_DEBUG("    layout=" << io.layout);

  return true;
}
