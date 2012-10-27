/*
 * shader.cpp
 *
 *  Created on: 26.03.2011
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <ogle/utility/string-util.h>
#include <ogle/utility/gl-error.h>
#include <ogle/gl-types/shader.h>

#include <ogle/external/glsw/glsw.h>

const GLenum Shader::shaderPipeline[] = {
    GL_VERTEX_SHADER,
    GL_TESS_CONTROL_SHADER,
    GL_TESS_EVALUATION_SHADER,
    GL_GEOMETRY_SHADER,
    GL_FRAGMENT_SHADER
};
const string Shader::shaderPipelinePrefixes[] = {
    "vs",
    "tcs",
    "tes",
    "gs",
    "fs"
};
const GLint Shader::pipelineSize =
    sizeof(shaderPipeline)/sizeof(GLenum);

struct ShaderIO {
  string declaration;
  string ioType;
  string dataType;
  string name;
  string nameWithoutPrefix;
  string numElements;
  string value;
};

/////////////
/////////////

static string resolveIncludes(const string &code)
{
  // FIXME: something wrong with #line
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
    codeTail = FORMAT_STRING( //tailLineDirective <<
        codeTail.substr(newLineNeedle+1));
  }

  return FORMAT_STRING(
      codeHead << endl <<
      resolveIncludes(Shader::loadFromKey(includedShader) + codeTail));
}

static string getNameWithoutPrefix(const string &name)
{
  if(hasPrefix(name, "in_")) {
    return truncPrefix(name, "in_");
  } else if(hasPrefix(name, "out_")) {
    return truncPrefix(name, "out_");
  } else if(hasPrefix(name, "u_")) {
    return truncPrefix(name, "u_");
  } else if(hasPrefix(name, "c_")) {
    return truncPrefix(name, "c_");
  } else {
    for(int i=0; i<Shader::pipelineSize; ++i) {
      const string &prefix = Shader::shaderPipelinePrefixes[i];
      if(hasPrefix(name, prefix+"_")) {
        return truncPrefix(name, prefix+"_");
      }
    }
  }
  return name;
}

static string getDeclaration(GLenum stage, const ShaderIO &in)
{
  stringstream ss;

  ss << in.ioType << " " << in.dataType << " " << in.name;

  if(!in.numElements.empty()) {
    ss << "[" << in.numElements << "]";
  }

  switch(stage) {
  case GL_VERTEX_SHADER:
    break;
  case GL_TESS_CONTROL_SHADER:
    if(in.ioType == "in" || in.ioType == "out") {
      ss << "[]";
    }
    break;
  case GL_TESS_EVALUATION_SHADER:
    if(in.ioType == "in") {
      ss << "[]";
    }
    break;
  case GL_GEOMETRY_SHADER:
    if(in.ioType == "in") {
      ss << "[GS_MAX_VERTICES]";
    }
    break;
  case GL_FRAGMENT_SHADER:
    break;
  }

  if(!in.value.empty() && in.ioType!="in" && in.ioType!="out") {
    ss << " = " << in.value;
  }
  return ss.str() + ";";
}

static void collectShaderIO(
    map<GLenum,string> &stages,
    map<GLenum, map<string,ShaderIO> > &inputs,
    map<GLenum, map<string,ShaderIO> > &outputs)
{
  static const char* ioPattern =
      "\n[ |\t]*((in|uniform|const|out)[ |\t]+([^ ]*)[ |\t]+([^;]+);)";
  static const char* valuePattern = "([^=]+)=([^;]+);";
  static const char* arrayPattern = "([^[]+)\\[([^\\]]+)\\]";
  boost::regex io_regex(ioPattern);
  boost::regex value_regex(valuePattern);
  boost::regex array_regex(arrayPattern);
  boost::sregex_iterator end;

  for(map<GLenum,string>::iterator
      it=stages.begin(); it!=stages.end(); ++it)
  {
    string &code = it->second;
    map<string,ShaderIO> &inputMap = inputs[it->first];
    map<string,ShaderIO> &outputMap = outputs[it->first];

    boost::sregex_iterator ioIt(code.begin(), code.end(), io_regex);
    for (; ioIt != end; ++ioIt) {
      ShaderIO io;
      io.declaration = (*ioIt)[1];
      io.ioType = (*ioIt)[2];
      io.dataType = (*ioIt)[3];
      io.name = (*ioIt)[4];
      io.value = "";
      io.numElements = "";

      // check for arrays and default values
      boost::sregex_iterator valueIt(io.name.begin(), io.name.end(), value_regex);
      if(valueIt != end) {
        io.name = (*valueIt)[1];
        io.value = (*valueIt)[2];
        boost::sregex_iterator arrayIt(io.name.begin(), io.name.end(), array_regex);
        if(arrayIt != end) {
          io.name = (*arrayIt)[1];
          io.numElements = (*arrayIt)[2];
        }
      }
      else {
        boost::sregex_iterator arrayIt(io.name.begin(), io.name.end(), array_regex);
        if(arrayIt != end) {
          io.name = (*arrayIt)[1];
          io.numElements = (*arrayIt)[2];
        }
      }
      if(io.ioType == "in" || io.ioType == "out") {
        io.numElements = "";
        io.value = "";
      }

      io.nameWithoutPrefix = getNameWithoutPrefix(io.name);
      if(io.ioType == "out") {
        outputMap[io.nameWithoutPrefix] = io;
      } else {
        inputMap[io.nameWithoutPrefix] = io;
      }
    }
  }
}

static void changeIOType(
    const ref_ptr<ShaderInput> &specifiedInput,
    const string &desiredInputType,
    const string &desiredOutputType,
    map<GLenum, map<string,ShaderIO> > &inputs,
    map<GLenum, map<string,ShaderIO> > &outputs,
    map<GLenum, string> &code)
{
  string inputName = getNameWithoutPrefix(specifiedInput->name());

  for(map<GLenum, map<string,ShaderIO> >::iterator
      it=inputs.begin(); it!=inputs.end(); ++it)
  {
    map<string,ShaderIO> &inputMap = it->second;
    map<string,ShaderIO> &outputMap = outputs[it->first];
    string &stageCode = code[it->first];

    map<string,ShaderIO>::iterator outputIt = outputMap.find(inputName);
    if(outputIt != outputMap.end() && desiredOutputType.empty()) {
      // var defined as 'out' but we are having constant or uniform specified.
      // TODO: use constant/uniform as specified until first stage defines an output
      //        starting with this stage treat it as varying
      WARN_LOG("Unable to change varying to uniform/constant " <<
          "because a stage declares custom output for this varying.");
      continue;
    }

    map<string,ShaderIO>::iterator inputIt = inputMap.find(inputName);
    if(inputIt == inputMap.end()) { continue; }

    // var defined as 'in','const' or 'uniform'
    ShaderIO &in = inputIt->second;
    if(in.ioType == desiredInputType) {
      // nothing to do
      continue;
    }
    string ioTypeOld = in.ioType;
    in.ioType = desiredInputType;

    if(in.ioType=="in") {
      // attributes can not have a default value
      in.value = "";
      if(!in.numElements.empty()) {
        // attributes can not be arrays
        WARN_LOG("No support for array attibute with name '" << inputName << "'");
        in.numElements = "";
      }
    } else {
      if(specifiedInput->forceArray() || specifiedInput->elementCount()>1) {
        in.numElements = FORMAT_STRING(specifiedInput->elementCount());
      }
      if(specifiedInput->hasData() && in.ioType=="const") {
        // set a default value
        stringstream val;
        if(in.numElements.empty()) {
          val << in.dataType << "(";
          (*specifiedInput.get()) >> val;
          val << ")";
          in.value = val.str();
        } else {
          // TODO: array initialization
          in.value = "";
        }
      }
    }

    // get new declaration
    string newDeclaration = getDeclaration(it->first, in);
    boost::algorithm::replace_all(stageCode, in.declaration, newDeclaration);
    in.declaration = newDeclaration;
  }
}

const string& Shader::stagePrefix(GLenum stage)
{
  for(int i=0; i<Shader::pipelineSize; ++i) {
    if(Shader::shaderPipeline[i] == stage) {
      return Shader::shaderPipelinePrefixes[i];
    }
  }
  static const string unk="unknown";
  return unk;
}

string Shader::loadFromKey(const string &effectKey)
{
  const char *code_c = glswGetShader(effectKey.c_str());
  if(code_c==NULL) {
    WARN_LOG(glswGetError());
    return "";
  }
  return string(code_c);
}
GLboolean Shader::isShaderKey(const string &s)
{
  if(boost::contains(s, "\n")) {
    return GL_FALSE;
  }
  if(boost::contains(s, "#")) {
    return GL_FALSE;
  }
  if(!boost::contains(s, ".")) {
    return GL_FALSE;
  }
  return GL_TRUE;
}

string Shader::load(const string &shaderCode)
{
  if(isShaderKey(shaderCode)) {
    return resolveIncludes("#include " + shaderCode);
  } else {
    return resolveIncludes(shaderCode);
  }
}
void Shader::load(
    const string &shaderHeader,
    map<GLenum,string> &shaderCode)
{
  list<string> effectNames;

  for(map<GLenum,string>::iterator
      it=shaderCode.begin(); it!=shaderCode.end(); ++it)
  {
    stringstream ss;
    ss << shaderHeader << endl;

    if(isShaderKey(it->second)) {
      list<string> path;
      boost::split(path, it->second, boost::is_any_of("."));
      effectNames.push_back(*path.begin());

      ss << "#include " << it->second << endl;
      shaderCode[it->first] = ss.str();
      continue;
    }
    {
      // we expect shader code directly provided
      ss << it->second << endl;
      it->second = ss.str();
    }
  }

  // if no vertex shader provided try to load default for effect
  if(shaderCode.count(GL_VERTEX_SHADER)==0) {
    for(list<string>::iterator it=effectNames.begin(); it!=effectNames.end(); ++it) {
      string defaultVSName = FORMAT_STRING((*it) << ".vs");
      string code = Shader::loadFromKey(defaultVSName);
      if(!code.empty()) {
        stringstream ss;
        ss << shaderHeader << endl;
        ss << code << endl;
        shaderCode[GL_VERTEX_SHADER] = ss.str();
        break;
      }
    }
  }

  // resolve include directives
  for(map<GLenum,string>::iterator
      it=shaderCode.begin(); it!=shaderCode.end(); ++it)
  {
    it->second = resolveIncludes(it->second);
  }
}

void Shader::preProcessCode(
    map<GLenum,string> &stages,
    const map<string, ref_ptr<ShaderInput> > &specifiedInput)
{
  // We want only collect actually used inputs and outputs,
  // so we have to evaluate macros here and throw out undefined code.
  // Would be nicer if this would not be necessary but i currently see no way out.
  // At least the code is more reader friendly afterwards.
  for(map<GLenum,string>::iterator
      it=stages.begin(); it!=stages.end(); ++it)
  {
    it->second = evaluateMacros(it->second);
  }

  map<GLenum, map<string,ShaderIO> > inputs;
  map<GLenum, map<string,ShaderIO> > outputs;
  collectShaderIO(stages, inputs, outputs);

  // Change code based on specified inputs
  //     * 'const TYPE NAME = VAL;' if input is a constant
  //     * 'uniform TYPE NAME;' if input is a uniform
  //     * 'in TYPE NAME;' if input is an attribute
  for(map<string, ref_ptr<ShaderInput> >::const_iterator
      it=specifiedInput.begin(); it!=specifiedInput.end(); ++it)
  {
    const ref_ptr<ShaderInput> &in = it->second;
    if(in->isVertexAttribute()) {
      changeIOType(in, "in", "out", inputs, outputs, stages);
    } else if (in->isConstant()) {
      changeIOType(in, "const", "", inputs, outputs, stages);
    } else {
      changeIOType(in, "uniform", "", inputs, outputs, stages);
    }
  }

  // Generate IO code
  //     * for each stage collect input from back to front
  //     * for each input make sure it is declared as input and output in previous stage
  map<GLenum, list<ShaderIO*> > genOutputs;
  map<GLenum, list<ShaderIO*> > genInputs;
  for(int i=Shader::pipelineSize-1; i>0; --i) {
    GLenum stage = Shader::shaderPipeline[i];
    GLenum previousStage = Shader::shaderPipeline[i-1];

    map<string,ShaderIO> &stageInputs = inputs[stage];
    map<string,ShaderIO> &previousInputs = inputs[previousStage];
    map<string,ShaderIO> &previousOutputs = outputs[previousStage];

    for(map<string,ShaderIO>::iterator it=stageInputs.begin(); it!=stageInputs.end(); ++it) {
      ShaderIO &io = it->second;
      // nothing to do for uniforms and constants
      if(io.ioType != "in") { continue; }

      // nothing to do if previous stage declares output
      if(previousOutputs.count(io.nameWithoutPrefix)>0) { continue; }
      // generate output in previous stage
      ShaderIO &genOutput = previousOutputs[io.nameWithoutPrefix];
      genOutput.nameWithoutPrefix = io.nameWithoutPrefix;
      genOutput.name = FORMAT_STRING("out_" << genOutput.nameWithoutPrefix);
      genOutput.ioType = "out";
      genOutput.dataType = io.dataType;
      genOutput.numElements = "";
      genOutput.value = "";
      genOutput.declaration = getDeclaration(stage, genOutput);
      genOutputs[previousStage].push_back(&genOutput);

      // nothing more to do if previous stage declares input
      if(previousInputs.count(io.nameWithoutPrefix)>0) { continue; }
      ShaderIO &genInput = previousInputs[io.nameWithoutPrefix];
      genInput.nameWithoutPrefix = io.nameWithoutPrefix;
      genInput.name = FORMAT_STRING("in_" << genOutput.nameWithoutPrefix);
      genInput.ioType = "in";
      genInput.dataType = io.dataType;
      genInput.numElements = "";
      genInput.value = "";
      genInput.declaration = getDeclaration(stage, genInput);
      genInputs[previousStage].push_back(&genInput);
    }
  }
  for(int i=0; i<Shader::pipelineSize; ++i) {
    GLenum stage = Shader::shaderPipeline[i];
    if(stages.count(stage)==0) { continue; }

    list<ShaderIO*> &genIn = genInputs[stage];
    list<ShaderIO*> &genOut = genOutputs[stage];
    map<string,ShaderIO> &stageInputs = inputs[stage];
    map<string,ShaderIO> &stageOutputs = outputs[stage];

    stringstream genHeader, handleIO;
    for(list<ShaderIO*>::iterator it=genIn.begin(); it!=genIn.end(); ++it) {
      genHeader << getDeclaration(stage,(**it)) << endl;
    }
    if(genOut.empty()) {
      handleIO << "#define HANDLE_IO(i)" << endl;
    } else {
      handleIO << "void HANDLE_IO(int index) {" << endl;
      for(list<ShaderIO*>::iterator it=genOut.begin(); it!=genOut.end(); ++it) {
        ShaderIO *io = *it;
        // declare output in header
        genHeader << getDeclaration(stage,*io) << endl;

        // pass to next stage in HANDLE_IO()
        string outName = stageOutputs[io->nameWithoutPrefix].name;
        string inName = stageInputs[io->nameWithoutPrefix].name;
        switch(stage) {
        case GL_VERTEX_SHADER:
          handleIO << "    " << outName << " = " << inName << ";" << endl;
          break;
        case GL_TESS_CONTROL_SHADER:
          handleIO << "    " << outName << "[index] = " << inName << "[index];" << endl;
          break;
        case GL_TESS_EVALUATION_SHADER:
          handleIO << "    " << outName << " = interpolate(" << inName << ");" << endl;
          break;
        case GL_GEOMETRY_SHADER:
          handleIO << "    " << outName << " = " << inName << "[index];" << endl;
          break;
        case GL_FRAGMENT_SHADER:
          break;
        }

      }
      handleIO << "}" << endl;
    }
    // insert declaration and HANDLE_IO()
    boost::replace_last(stages[stage],
        "void main()",
        FORMAT_STRING(handleIO.str() << endl << "void main()"));
    stages[stage] = FORMAT_STRING(
        genHeader.str() << endl <<
        stages[stage]
    );
  }

  // Make IO prefixes match each other
  //      * Change 'in_' prefix for attributes to stage prefix
  //      * Change 'out_' prefix to next stage prefix
  for(int i=0; i<Shader::pipelineSize; ++i) {
    GLenum stage = Shader::shaderPipeline[i];
    if(stages.count(stage)==0) { continue; }
    string &stageCode = stages[stage];

    const string &stagePrefix = Shader::shaderPipelinePrefixes[i];
    map<string,ShaderIO> &stageInputs = inputs[stage];
    // rename in varyings with stage prefix
    for(map<string,ShaderIO>::iterator
        it=stageInputs.begin(); it!=stageInputs.end(); ++it)
    {
      ShaderIO &io = it->second;
      if(io.ioType != "in") { continue; }
      replaceVariable(
          io.name,
          FORMAT_STRING(stagePrefix<<"_"<<io.nameWithoutPrefix),
          &stageCode);
    }

    // find next stage
    string nextStagePrefix = "";
    for(int j=i+1; j<Shader::pipelineSize; ++j) {
      GLenum nextStage = Shader::shaderPipeline[j];
      if(stages.count(nextStage)==0) { continue; }
      nextStagePrefix = Shader::shaderPipelinePrefixes[j];
    }
    if(nextStagePrefix.empty()) { continue; }

    // rename out varyings with next stage prefix
    map<string,ShaderIO> &stageOutputs = outputs[stage];
    for(map<string,ShaderIO>::iterator
        it=stageOutputs.begin(); it!=stageOutputs.end(); ++it)
    {
      ShaderIO &io = it->second;
      if(io.ioType != "out") { continue; }
      replaceVariable(
          io.name,
          FORMAT_STRING(nextStagePrefix<<"_"<<io.nameWithoutPrefix),
          &stageCode);
    }
  }
}

ref_ptr<Shader> Shader::create(
    const map<string, string> &shaderConfig,
    map<GLenum,string> &code)
{
  map<string, ref_ptr<ShaderInput> > specifiedInput;
  create(shaderConfig,specifiedInput,code);
}
ref_ptr<Shader> Shader::create(
    const map<string, string> &shaderConfig,
    const map<string, ref_ptr<ShaderInput> > &specifiedInput,
    map<GLenum,string> &code)
{
  // configure shader using macros
  string header="";
  for(map<string,string>::const_iterator
      it=shaderConfig.begin(); it!=shaderConfig.end(); ++it)
  {
    const string &name = it->first;
    const string &value = it->second;
    if(name=="GLSL_VERSION") {
      header = FORMAT_STRING("#version "<<value<<"\n" << header);
    } else if(value=="TRUE") {
      header = FORMAT_STRING("#define "<<name<<"\n" << header);
    } else if(value=="FALSE") {
      header = FORMAT_STRING("// #undef "<<name<<"\n" << header);
    } else {
      header = FORMAT_STRING("#define "<<name<<" "<<value<<"\n" << header);
    }
  }

  // load the GLSL code.
  // shader names can be keys that will be loaded from file.
  // it is allowed to include files using the include directive
  map<GLenum,string> stages(code);
  load(header, stages);

  preProcessCode(stages, specifiedInput);

  return ref_ptr<Shader>::manage(new Shader(stages));
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

/////////////
/////////////

Shader::Shader(Shader &other)
: shaderCodes_(other.shaderCodes_),
  shaders_(other.shaders_),
  id_(other.id_),
  numInstances_(0),
  transformfeedbackLayout_(GL_SEPARATE_ATTRIBS)
{
}
Shader::Shader(const map<GLenum, string> &shaderCodes)
: shaderCodes_(shaderCodes),
  numInstances_(0),
  transformfeedbackLayout_(GL_SEPARATE_ATTRIBS)
{
  id_ = ref_ptr<GLuint>::manage(new GLuint);
  *(id_.get()) = glCreateProgram();
}
Shader::Shader(
    const map<GLenum, string> &shaderNames,
    const map<GLenum, GLuint> &shaderStages)
: shaderCodes_(shaderNames),
  shaders_(shaderStages),
  numInstances_(0),
  transformfeedbackLayout_(GL_SEPARATE_ATTRIBS)
{
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

GLuint Shader::numInstances() const
{
  return numInstances_;
}

bool Shader::hasStage(GLenum stage) const
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

GLuint Shader::stage(GLenum s) const
{
  map<GLenum, GLuint>::const_iterator it = shaders_.find(s);
  if(it!=shaders_.end()) {
    return it->second;
  } else {
    return 0;
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

GLboolean Shader::compile()
{
  numInstances_ = 0;

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
      return GL_FALSE;
    }
    //if(Logging::verbosity() > Logging::_) {
    //  printLog(shaderStage, it->first, source, true);
    //}

    glAttachShader(id(), shaderStage);
    shaders_[it->first] = shaderStage;
  }

  return GL_TRUE;
}

GLboolean Shader::link()
{
  if(!transformFeedback_.empty()>0) {
    // specify the transform feedback output names
    static const string prefixes[] = {"gl", "fs", "tcs", "tes", "gs"};
    vector<const char*> validNames(transformFeedback_.size());
    int validCounter = 0;

    map<GLenum, map<string,ShaderIO> > inputs;
    map<GLenum, map<string,ShaderIO> > outputs;
    collectShaderIO(shaderCodes_, inputs, outputs);

    for(list<string>::const_iterator
        it=transformFeedback_.begin(); it!=transformFeedback_.end(); ++it)
    {
      string name = getNameWithoutPrefix(*it);

      for(int i=0; i<sizeof(prefixes)/sizeof(string); ++i) {
        string nameWithPrefix = prefixes[i]+name;
        GLboolean found = GL_FALSE;

        for(int j=0; j<pipelineSize; ++j) {
          GLenum stage = shaderPipeline[j];
          if(inputs.count(stage)==0) { continue; }
          map<string,ShaderIO> &in = inputs[stage];
          if(in.count(name)==0) { continue; }

          if(in[name].name == nameWithPrefix) {
            validNames[validCounter++] = nameWithPrefix.c_str();
            found = GL_TRUE;
            break;
          }
        }
        if(found) {
          break;
        }
      }
    }

    glTransformFeedbackVaryings(id(),
        validCounter, validNames.data(), transformfeedbackLayout_);
  }

  if(outputs_.empty()) {
    glBindFragDataLocation(id(), 0, "output");
  } else {
    for(list<ShaderOutput>::const_iterator it=outputs_.begin(); it!=outputs_.end(); ++it) {
      glBindFragDataLocation(
          id(),
          it->colorAttachment-GL_COLOR_ATTACHMENT0,
          it->name.c_str());
    }
  }

  glLinkProgram(id());
  GLint status;
  glGetProgramiv(id(), GL_LINK_STATUS,  &status);
  if(status == GL_FALSE) {
    printLog(id(), GL_VERTEX_SHADER, NULL, false);
    handleGLError("after glLinkProgram");
    return GL_FALSE;
  } else {
    setupInputLocations();
    return GL_TRUE;
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

    cout << "ACTIVE UNI " << uniformName << endl;

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
    attributeLocations_[FORMAT_STRING("a_"<<attName)] = loc;
    attributeLocations_[FORMAT_STRING("in_"<<attName)] = loc;
    attributeLocations_[FORMAT_STRING("vs_"<<attName)] = loc;
  }
}

void Shader::setInput(const ref_ptr<ShaderInput> &in)
{
  if(in->isVertexAttribute()) {
    if(in->numInstances()>1) {
      numInstances_ = in->numInstances();
    }

    map<string,GLint>::iterator needle = attributeLocations_.find(in->name());
    if(needle!=attributeLocations_.end()) {
      attributes_.push_back(ShaderInputLocation(in,needle->second));
    }
  }
  else if (!in->isConstant()) {
    map<string,GLint>::iterator needle = uniformLocations_.find(in->name());
    if(needle!=uniformLocations_.end()) {

      cout << "SET INPUT " << in->name() << endl;
      uniforms_.push_back(ShaderInputLocation(in,needle->second));
    } else {

      cout << "NO INPUT " << in->name() << endl;
    }
  }
}

void Shader::setInputs(const map<string, ref_ptr<ShaderInput> > &inputs)
{
  for(map<string, ref_ptr<ShaderInput> >::const_iterator
      it=inputs.begin(); it!=inputs.end(); ++it)
  {
    const ref_ptr<ShaderInput> &in = it->second;
    setInput(in);
  }
}

void Shader::setOutput(const ShaderOutput &out)
{
  outputs_.push_back(out);
}
void Shader::setOutputs(const list<ShaderOutput> &outputs)
{
  for(list<ShaderOutput>::const_iterator it=outputs.begin(); it!=outputs.end(); ++it)
  {
    setOutput(*it);
  }
}

void Shader::setTransformFeedback(
    const list<string> &transformFeedback,
    GLenum attributeLayout)
{
  transformfeedbackLayout_ = attributeLayout;
  for(list<string>::const_iterator
      it=transformFeedback.begin(); it!=transformFeedback.end(); ++it)
  {
    transformFeedback_.push_back( *it );
  }
}

//////////////

void Shader::uploadInputs()
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

void Shader::uploadTexture(const ShaderTexture &d)
{
  map<string,GLint>::iterator needle = uniformLocations_.find(d.tex->name());
  if(needle!=uniformLocations_.end()) {
    glUniform1i( needle->second, d.texUnit );
  }
}

void Shader::uploadAttribute(const ShaderInput *input)
{
  map<string,GLint>::iterator needle = attributeLocations_.find(input->name());
  if(needle!=attributeLocations_.end()) {
    input->enableAttribute( needle->second );
  }
}

void Shader::uploadUniform(const ShaderInput *input)
{
  map<string,GLint>::iterator needle = uniformLocations_.find(input->name());
  if(needle!=uniformLocations_.end()) {
    input->enableUniform( needle->second );
  }
}
