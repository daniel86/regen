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
#include <ogle/gl-types/glsl-directive-processor.h>
#include <ogle/gl-types/glsl-io-processor.h>

#include <ogle/external/glsw/glsw.h>

#define NO_REGEX_MATCH boost::sregex_iterator()

/////////////
/////////////

string Shader::load(const string &shaderCode,
    const map<string,string> &functions)
{
  string code;
  if(GLSLDirectiveProcessor::canInclude(shaderCode)) {
    code = "\n#include " + shaderCode + "\n";
  } else {
    code = shaderCode;
  }

  stringstream in(code);

  GLSLDirectiveProcessor p(in, functions);
  stringstream out;
  p.preProcess(out);
  return out.str();
}

void Shader::load(
    const string &shaderHeader,
    map<GLenum,string> &shaderCode,
    const map<string,string> &functions,
    const map<string, ref_ptr<ShaderInput> > &specifiedInput)
{
  static const char* mainPattern = ".*void[ \n]+main\\([ \n]*|[ \n]*void[ \n]*\\)[ \n]*\\{.*\\}.*";
  static boost::regex mainRegex(mainPattern);

  list<string> effectNames;

  for(map<GLenum,string>::iterator
      it=shaderCode.begin(); it!=shaderCode.end(); ++it)
  {
    if(it->second.empty()) { continue; }

    stringstream ss;
    ss << "#define SHADER_STAGE " <<
        GLSLInputOutputProcessor::getPrefix(it->first) << endl;
    ss << shaderHeader << endl;

    if(GLSLDirectiveProcessor::canInclude(it->second)) {
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
      string code = GLSLDirectiveProcessor::include(defaultVSName);
      if(!code.empty()) {
        stringstream ss;
        ss << shaderHeader << endl;
        ss << code << endl;
        shaderCode[GL_VERTEX_SHADER] = ss.str();
        break;
      }
    }
  }

  {
    map<string,GLSLInputOutput> nextStageInputs;

    GLenum nextStage = GL_NONE;
    // reverse process stages, because stages must know inputs
    // of next stages.
    for(int i=GLSLInputOutputProcessor::pipelineSize-1; i>=0; --i)
    {
      GLenum stage = GLSLInputOutputProcessor::shaderPipeline[i];

      map<GLenum,string>::iterator it = shaderCode.find(stage);
      if(it==shaderCode.end()) { continue; }

      stringstream in(it->second);
      stringstream ioProcessed;
      stringstream directivesProcessed;
      string line;

      // in -> directivesProcessed
      GLSLDirectiveProcessor p0(in,functions);
      // directivesProcessed -> ioProcessed
      GLSLInputOutputProcessor p1(
          directivesProcessed,
          stage, nextStage,
          nextStageInputs,
          specifiedInput);

      // evaluate directives and modify IO afterwards
      while(p0.getline(line)) {
        directivesProcessed << line << endl;
        while(p1.getline(line)) {
          ioProcessed << line << endl;
        }
        // make the EOF disappear
        directivesProcessed.clear();
      }

      it->second = ioProcessed.str();
      // check if a main function is defined
      boost::sregex_iterator regexIt(it->second.begin(), it->second.end(), mainRegex);
      if(regexIt==NO_REGEX_MATCH) {
        shaderCode.erase(stage);
        continue;
      }

      nextStage = stage;
      nextStageInputs = p1.inputs();
    }
  }
}

ref_ptr<Shader> Shader::create(
    const map<string, string> &shaderConfig,
    const map<string,string> &functions,
    map<GLenum,string> &code)
{
  map<string, ref_ptr<ShaderInput> > specifiedInput;
  return create(shaderConfig,functions,specifiedInput,code);
}

ref_ptr<Shader> Shader::create(
    const map<string, string> &shaderConfig,
    const map<string,string> &functions,
    const map<string, ref_ptr<ShaderInput> > &specifiedInput,
    map<GLenum,string> &code)
{
  // configure shader using macros
  string header="";
  for(map<string,string>::const_iterator
      it=shaderConfig.begin(); it!=shaderConfig.end(); ++it)
  {
    const string &name = it->first;
    string value = it->second;

    //boost::algorithm::replace_all(value, "\n"," \\ \n");

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
  load(header, stages, functions, specifiedInput);

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
    default: shaderName = "Linking"; break;
    }

    if(success) {
      logLevel = Logging::INFO;
      LOG_MESSAGE(logLevel, shaderName << " Shader compiled successfully!");
    } else {
      logLevel = Logging::ERROR;
      LOG_MESSAGE(logLevel, shaderName << " Shader failed to compile!");
    }
  } else {
    if(success) {
      logLevel = Logging::INFO;
      LOG_MESSAGE(logLevel, "Shader linked successfully.");
    } else {
      logLevel = Logging::ERROR;
      LOG_MESSAGE(logLevel, "Shader failed to link.");
    }
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
  if(shaderType==GL_NONE) {
    glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &length);
  } else {
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
  }
  if(length>0) {
    char log[length];
    if(shaderType==GL_NONE) {
      glGetProgramInfoLog(shader, length, NULL, log);
      LOG_MESSAGE(logLevel, "link info:" << log);
    } else {
      glGetShaderInfoLog(shader, length, NULL, log);
      LOG_MESSAGE(logLevel, "compile info:" << log);
    }
  } else {
    LOG_MESSAGE(logLevel, "shader info: empty");
  }
}

/////////////
/////////////

Shader::Shader(Shader &other)
: id_(other.id_),
  numInstances_(0),
  shaderCodes_(other.shaderCodes_),
  shaders_(other.shaders_),
  feedbackLayout_(GL_SEPARATE_ATTRIBS)
{
}

Shader::Shader(const map<GLenum, string> &shaderCodes)
: numInstances_(0),
  shaderCodes_(shaderCodes),
  feedbackLayout_(GL_SEPARATE_ATTRIBS)
{
  id_ = ref_ptr<GLuint>::manage(new GLuint);
  *(id_.get()) = glCreateProgram();
}

Shader::Shader(
    const map<GLenum, string> &shaderNames,
    map<GLenum, ref_ptr<GLuint> > &shaderStages)
: numInstances_(0),
  shaderCodes_(shaderNames),
  shaders_(shaderStages),
  feedbackLayout_(GL_SEPARATE_ATTRIBS)
{
  id_ = ref_ptr<GLuint>::manage(new GLuint);
  *(id_.get()) = glCreateProgram();

  for(map<GLenum, ref_ptr<GLuint> >::const_iterator
      it = shaders_.begin(); it != shaders_.end(); ++it)
  {
    glAttachShader(id(), *it->second.get());
  }
}

Shader::~Shader()
{
  glUseProgram(0);

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

GLuint Shader::numInstances() const
{
  return numInstances_;
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
    GLuint *shaderStage = new GLuint;
    *shaderStage = glCreateShader(it->first);
    GLint length = -1;
    GLint status;

    glShaderSource(*shaderStage, 1, &source, &length);
    glCompileShader(*shaderStage);

    glGetShaderiv(*shaderStage, GL_COMPILE_STATUS, &status);
    if (!status) {
      printLog(*shaderStage, it->first, source, false);
      glDeleteShader(*shaderStage);
      return GL_FALSE;
    }
    //if(Logging::verbosity() > Logging::_) {
    //  printLog(*shaderStage, it->first, source, GL_FALSE);
    //}

    glAttachShader(id(), *shaderStage);
    shaders_[it->first] = ref_ptr<GLuint>::manage(shaderStage);
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
    int i=0;
    for(; GLSLInputOutputProcessor::shaderPipeline[i]!=stage; ++i) {}

    // find next stage
    string nextStagePrefix = "out";
    for(int j=i+1; j<GLSLInputOutputProcessor::pipelineSize; ++j) {
      GLenum nextStage = GLSLInputOutputProcessor::shaderPipeline[j];
      if(shaders_.count(nextStage)!=0) {
        nextStagePrefix = GLSLInputOutputProcessor::shaderPipelinePrefixes[j];
        break;
      }
    }

    for(list<string>::const_iterator
        it=transformFeedback_.begin(); it!=transformFeedback_.end(); ++it)
    {
      string name = GLSLInputOutputProcessor::getNameWithoutPrefix(*it);
      if(name == "Position") {
        name = "gl_" + name;
      } else {
        name = nextStagePrefix + "_" + name;
      }
      if(validNames_.count(name)>0) { continue; }
      validNames_.insert(name);
      validNames[validCounter] = name.c_str();

      INFO_LOG("using '" << validNames[validCounter] << "' for transform feedback.");
      ++validCounter;
    }

    glTransformFeedbackVaryings(id(),
        validCounter, validNames.data(), feedbackLayout_);
  }

  glLinkProgram(id());
  GLint status;
  glGetProgramiv(id(), GL_LINK_STATUS,  &status);
  if(status == GL_FALSE) {
    printLog(id(), GL_NONE, NULL, false);
    handleGLError("after glLinkProgram");
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
    char log[length];
    glGetProgramInfoLog(id(), length, NULL, log);
    WARN_LOG("validation failed: " << log);
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

    case GL_FLOAT_MAT2:
      inputs_[uniformName] = ref_ptr<ShaderInput>::manage(
          new ShaderInput4f(uniformName,arraySize));
      break;
    case GL_FLOAT_MAT3:
      inputs_[uniformName] = ref_ptr<ShaderInput>::manage(
          new ShaderInputMat3(uniformName,arraySize));
      break;
    case GL_FLOAT_MAT4:
      inputs_[uniformName] = ref_ptr<ShaderInput>::manage(
          new ShaderInputMat4(uniformName,arraySize));
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
      WARN_LOG("unknown shader type for '" << uniformName << "'");
      break;

    }
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
    attributeLocations_[FORMAT_STRING("a_"<<attName)] = loc;
    attributeLocations_[FORMAT_STRING("in_"<<attName)] = loc;
    attributeLocations_[FORMAT_STRING("vs_"<<attName)] = loc;
  }
}

void Shader::setInput(const ref_ptr<ShaderInput> &in)
{
  inputs_[in->name()] = in;

  if(!in->hasData()) { return; }

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
      uniforms_.push_back(ShaderInputLocation(in,needle->second));
    } else {
    }
  }
}
void Shader::setTexture(GLint *channel, const string &name)
{
  map<string,GLint>::iterator needle = samplerLocations_.find(name);
  if(needle!=samplerLocations_.end())
  {
    for(list<ShaderTextureLocation>::iterator
        it=textures_.begin(); it!=textures_.end(); ++it)
    {
      ShaderTextureLocation &texLoc = *it;
      if(texLoc.location == needle->second) {
        textures_.erase(it);
        break;
      }
    }
    if(channel!=NULL) {
      textures_.push_back(ShaderTextureLocation(channel,needle->second));
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

void Shader::uploadInputs()
{
  GLuint lastVBO = 0;
  for(list<ShaderInputLocation>::iterator
      it=attributes_.begin(); it!=attributes_.end(); ++it)
  {
    if(it->input->buffer() != lastVBO) {
      lastVBO = it->input->buffer();
      glBindBuffer(GL_ARRAY_BUFFER, lastVBO);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lastVBO);
    }
    it->input->enableAttribute( it->location );
  }
  for(list<ShaderInputLocation>::iterator
      it=uniforms_.begin(); it!=uniforms_.end(); ++it)
  {
    it->input->enableUniform( it->location );
  }
  for(list<ShaderTextureLocation>::iterator
      it=textures_.begin(); it!=textures_.end(); ++it)
  {
    glUniform1i( it->location, *(it->channel) );
  }
}

void Shader::uploadTexture(GLint channel, const string &name)
{
  map<string,GLint>::iterator needle = uniformLocations_.find(name);
  if(needle!=uniformLocations_.end()) {
    glUniform1i( needle->second, channel );
  }
}

void Shader::uploadAttribute(const ShaderInput *input)
{
  map<string,GLint>::iterator needle = attributeLocations_.find(input->name());
  if(needle!=attributeLocations_.end()) {
    glBindBuffer(GL_ARRAY_BUFFER, input->buffer());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, input->buffer());
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
