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
#include <regen/gl-types/texture.h>
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
: GLSLProcessor("InputOutput"),
  isInputSpecified_(GL_FALSE),
  currStage_(-1)
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

void IOProcessor::declareSpecifiedInput(PreProcessorState &state)
{
  const list<NamedShaderInput> &specifiedInput = state.in.specifiedInput;
  InputOutput io;
  io.layout = "";
  io.interpolation = "";
  lineQueue_.push_back("#define DECLARED_INPUT_FOO");

  for(list<NamedShaderInput>::const_iterator
      it=specifiedInput.begin(); it!=specifiedInput.end(); ++it)
  {
    ref_ptr<ShaderInput> in = it->in_;
    string nameWithoutPrefix = getNameWithoutPrefix(it->name_);
    if(inputNames_.count(nameWithoutPrefix)) continue;

    Texture *tex = dynamic_cast<Texture*>(in.get());
    if(tex==NULL) {
      io.dataType = glenum::glslDataType(in->dataType(), in->valsPerElement());
    } else {
      io.dataType = tex->samplerType();
    }
    io.numElements = (in->elementCount()>1 || in->forceArray()) ?
        REGEN_STRING(in->elementCount()) : "";
    io.name = "in_"+nameWithoutPrefix;

    if(in->isVertexAttribute()) {
      if(state.currStage != GL_VERTEX_SHADER) continue;
      io.ioType = "in";
      io.value = "";
      inputs_[state.currStage].insert(make_pair(nameWithoutPrefix,io));

      lineQueue_.push_back(REGEN_STRING("#define " << io.name << " " <<
          glenum::glslStagePrefix(state.currStage) << "_" << nameWithoutPrefix));
    }
    else if(in->isConstant()) {
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
      uniforms_[state.currStage].insert(make_pair(nameWithoutPrefix,io));
    }

    lineQueue_.push_back(io.declaration(state.currStage));
    inputNames_.insert(nameWithoutPrefix);
  }
}

void IOProcessor::parseValue(string &v, string &val)
{
  static const char* pattern_ = "[ ]*([^= ]+)[ ]*=[ ]*([^ ]+)[ ]*";
  static boost::regex regex_(pattern_);

  boost::sregex_iterator it(v.begin(), v.end(), regex_);
  if(it != NO_REGEX_MATCH) {
    val = (*it)[2];
    v = (*it)[1];
  }
}

void IOProcessor::parseArray(string &v, string &numElements)
{
  static const char* pattern_ = "([^\\[]+)\\[([^\\]]*)\\]";
  static boost::regex regex_(pattern_);

  boost::sregex_iterator it(v.begin(), v.end(), regex_);
  if(it != NO_REGEX_MATCH) {
    numElements = (*it)[2];
    v = (*it)[1];
  }
}

void IOProcessor::clear()
{
  inputs_.clear();
  outputs_.clear();
  uniforms_.clear();
  lineQueue_.clear();
  inputNames_.clear();
  isInputSpecified_ = GL_FALSE;
  currStage_ = -1;
}

bool IOProcessor::process(PreProcessorState &state, string &line)
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
  static const char* macroPattern_ = "^[ |\t]*#(.*)$";
  static boost::regex macroRegex_(macroPattern_);

  if(currStage_!=state.currStage) {
    inputNames_.clear();
    isInputSpecified_ = GL_FALSE;
    currStage_ = state.currStage;
  }

  // read a line from the queue
  if(!lineQueue_.empty())
  {
    line = lineQueue_.front();
    lineQueue_.pop_front();
    return true;
  }
  // read a line from the input stream
  if(!getlineParent(state, line))
  {
    return false;
  }
  boost::sregex_iterator it;

  // Insert declarations of specified inputs when the first
  // code line occurs.
  if(!isInputSpecified_) {
    it = boost::sregex_iterator(line.begin(), line.end(), macroRegex_);
    if(it==NO_REGEX_MATCH) {
      declareSpecifiedInput(state);
      isInputSpecified_ = GL_TRUE;
    }
  }

  // Processing of HANLDE_IO macro.
  // TODO: do not require this macro. do it automatically.
  it = boost::sregex_iterator(line.begin(), line.end(), handleIORegex_);
  if(it!=NO_REGEX_MATCH) {
    if(!isInputSpecified_) {
      declareSpecifiedInput(state);
      isInputSpecified_ = GL_TRUE;
    }
    defineHandleIO(state);
    return process(state,line);
  }

  // Parse input declaration.
  InputOutput io;
  it = boost::sregex_iterator(line.begin(), line.end(), interpolationRegex_);
  if(it==NO_REGEX_MATCH) {
    it = boost::sregex_iterator(line.begin(), line.end(), regex_);
  }
  else {
    // interpolation qualifier specified
    io.interpolation = (*it)[2];
    line = (*it)[3];
    it = boost::sregex_iterator(line.begin(), line.end(), regex_);
  }
  // No input declaration found.
  if(it==NO_REGEX_MATCH)
  {
    lineQueue_.push_back(line);
    return process(state,line);
  }

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
  if(io.ioType != "out") {
    // skip input if already defined
    if(inputNames_.count(nameWithoutPrefix)>0)
      return process(state,line);
    inputNames_.insert(nameWithoutPrefix);

    // Change IO type based on specified input.
    list<NamedShaderInput>::const_iterator needle;
    for(needle = state.in.specifiedInput.begin();
        needle!= state.in.specifiedInput.end(); ++needle)
    { if(needle->name_ == nameWithoutPrefix) {
      // Change declaration based on specified input
      const ref_ptr<ShaderInput> &in = needle->in_;
      if(in->isVertexAttribute()) {
        io.ioType = "in";
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
      break;
    }}
  }

  if(io.ioType == "in") {
    // define input name with matching prefix
    lineQueue_.push_back(REGEN_STRING("#define " << io.name << " " <<
        glenum::glslStagePrefix(state.currStage) << "_" << nameWithoutPrefix));
  }
  else if(io.ioType == "out" && state.nextStage != GL_NONE) {
    // define output name with matching prefix
    lineQueue_.push_back(REGEN_STRING("#define " << io.name << " " <<
        glenum::glslStagePrefix(state.nextStage) << "_" << nameWithoutPrefix));
  }
  line = io.declaration(state.currStage);

  if(io.ioType == "out") {
    outputs_[state.currStage].insert(make_pair(nameWithoutPrefix,io));
  }
  else if(io.ioType == "in") {
    inputs_[state.currStage].insert(make_pair(nameWithoutPrefix,io));
  }
  else if(io.ioType == "uniform") {
    uniforms_[state.currStage].insert(make_pair(nameWithoutPrefix,io));
  }

  lineQueue_.push_back(line);
  return process(state,line);
}
