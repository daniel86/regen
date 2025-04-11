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

ref_ptr<PreProcessor> &Shader::defaultPreProcessor() {
	static ref_ptr<PreProcessor> defaultProcessor;
	if (!defaultProcessor.get()) {
		defaultProcessor = ref_ptr<PreProcessor>::alloc();
		defaultProcessor->addProcessor(ref_ptr<InputProviderProcessor>::alloc());
		defaultProcessor->addProcessor(ref_ptr<DirectiveProcessor>::alloc());
		defaultProcessor->addProcessor(ref_ptr<CommentProcessor>::alloc());
		defaultProcessor->addProcessor(ref_ptr<WhiteSpaceProcessor>::alloc());
		defaultProcessor->addProcessor(ref_ptr<IOProcessor>::alloc());
	}
	return defaultProcessor;
}

ref_ptr<PreProcessor> &Shader::singleStagePreProcessor() {
	static ref_ptr<PreProcessor> singleProcessor;
	if (!singleProcessor.get()) {
		singleProcessor = ref_ptr<PreProcessor>::alloc();
		singleProcessor->addProcessor(ref_ptr<InputProviderProcessor>::alloc());
		singleProcessor->addProcessor(ref_ptr<DirectiveProcessor>::alloc());
		singleProcessor->addProcessor(ref_ptr<CommentProcessor>::alloc());
		singleProcessor->addProcessor(ref_ptr<WhiteSpaceProcessor>::alloc());
	}
	return singleProcessor;
}

void Shader::preProcess(
		std::map<GLenum, std::string> &ret,
		const PreProcessorConfig &cfg,
		const ref_ptr<PreProcessor> &preProcessor) {
	// configure shader using macros
	std::stringstream header;
	header << "#version " << cfg.version << std::endl;
	for (const auto &define: cfg.defines) {
		const std::string &name = define.first;
		const std::string &value = define.second;
		if (value == "TRUE") {
			header << "#define " << name << std::endl;
		} else if (value == "FALSE") {
			header << "// #undef " << name << std::endl;
		} else {
			header << "#define " << name << " " << value << std::endl;
		}
	}
	header << "#include regen.defines.regen_InstanceID" << std::endl;
	for (const auto &path: cfg.includes) {
		header << "#include " << path << std::endl;
	}
	std::string headerStr = header.str();

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
		GLboolean success) {
	Logging::LogLevel logLevel;
	if (shaderCode != nullptr) {
		std::string shaderName = glenum::glslStageName(shaderType);

		if (success) {
			logLevel = Logging::INFO;
			//REGEN_LOG(logLevel, shaderName << " Shader compiled successfully!");
		} else {
			logLevel = Logging::ERROR;
			REGEN_LOG(logLevel, shaderName << " Shader failed to compile!");
		}
	} else {
		if (success) {
			logLevel = Logging::INFO;
			//REGEN_LOG(logLevel, "Shader linked successfully.");
		} else {
			logLevel = Logging::ERROR;
			REGEN_LOG(logLevel, "Shader failed to link.");
		}
	}

	if (!success && shaderCode != nullptr) {
		std::vector<std::string> codeLines;
		boost::split(codeLines, shaderCode, boost::is_any_of("\n"));
		for (GLuint i = 0; i < codeLines.size(); ++i) {
			REGEN_LOG(logLevel,
					  std::setw(3) << i << std::setw(0) << " " << codeLines[i]);
		}
	}

	int length;
	if (shaderType == GL_NONE) {
		glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &length);
	} else {
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
	}
	if (length > 1) {
		char *log = new char[length];
		std::string msgPrefix;
		if (shaderType == GL_NONE) {
			glGetProgramInfoLog(shader, length, nullptr, log);
			msgPrefix = "link info: ";
		} else {
			glGetShaderInfoLog(shader, length, nullptr, log);
			msgPrefix = "compile info: ";
		}

		std::vector<std::string> errorLines;
		boost::split(errorLines, log, boost::is_any_of("\n"));
		for (const auto &errorLine: errorLines) {
			if (!errorLine.empty()) REGEN_LOG(logLevel, msgPrefix << errorLine);
		}
		delete[]log;
	}
}

/////////////
/////////////

Shader::Shader(const Shader &other)
		: id_(other.id_),
		  shaderCodes_(other.shaderCodes_),
		  shaders_(other.shaders_),
		  feedbackLayout_(GL_SEPARATE_ATTRIBS),
		  feedbackStage_(GL_VERTEX_SHADER) {
}

Shader::Shader(const std::map<GLenum, std::string> &shaderCodes)
		: shaderCodes_(shaderCodes),
		  feedbackLayout_(GL_SEPARATE_ATTRIBS),
		  feedbackStage_(GL_VERTEX_SHADER) {
	id_ = ref_ptr<GLuint>::alloc();
	*(id_.get()) = glCreateProgram();
}

Shader::Shader(
		const std::map<GLenum, std::string> &shaderNames,
		const std::map<GLenum, ref_ptr<GLuint> > &shaderStages)
		: shaderCodes_(shaderNames),
		  shaders_(shaderStages),
		  feedbackLayout_(GL_SEPARATE_ATTRIBS),
		  feedbackStage_(GL_VERTEX_SHADER) {
	id_ = ref_ptr<GLuint>::alloc();
	*(id_.get()) = glCreateProgram();

	for (auto &shader: shaders_) {
		glAttachShader(id(), *shader.second.get());
	}
}

Shader::~Shader() {
	for (auto &shader: shaders_) {
		ref_ptr<GLuint> &stage = shader.second;
		glDetachShader(id(), *stage.get());
		if (*stage.refCount() == 1) {
			glDeleteShader(*stage.get());
		}
	}

	if (*id_.refCount() == 1) {
		glDeleteProgram(id());
	}
}

bool Shader::hasStage(GLenum stage) const {
	return shaders_.count(stage) > 0;
}

const std::string &Shader::stageCode(GLenum stage) const {
	auto it = shaderCodes_.find(stage);
	if (it != shaderCodes_.end()) {
		return it->second;
	} else {
		static const std::string empty;
		return empty;
	}
}

ref_ptr<GLuint> Shader::stage(GLenum s) const {
	auto it = shaders_.find(s);
	if (it != shaders_.end()) {
		return it->second;
	} else {
		return {};
	}
}

bool Shader::hasUniform(const std::string &name) const {
	return inputNames_.count(name) > 0;
}

bool Shader::hasSampler(const std::string &name) const {
	auto it = samplerLocations_.find(name);
	if (it == samplerLocations_.end()) return false;
	return textures_.count(it->second) > 0;
}

bool Shader::hasUniformData(const std::string &name) const {
	auto it = inputNames_.find(name);
	if (it == inputNames_.end()) {
		return false;
	} else {
		return it->second->in_->hasData();
	}
}

ref_ptr<ShaderInput> Shader::input(const std::string &name) {
	auto it = inputNames_.find(name);
	if (it == inputNames_.end()) {
		return {};
	} else {
		return it->second->in_;
	}
}

GLint Shader::samplerLocation(const std::string &name) {
	auto it = samplerLocations_.find(name);
	return (it != samplerLocations_.end()) ? it->second : -1;
}

GLint Shader::attributeLocation(const std::string &name) {
	auto it = attributeLocations_.find(name);
	return (it != attributeLocations_.end()) ? it->second : -1;
}

GLint Shader::uniformLocation(const std::string &name) {
	auto it = uniformLocations_.find(name);
	return (it != uniformLocations_.end()) ? it->second : -1;
}

unsigned int Shader::id() const {
	return *(id_.get());
}

bool Shader::compile() {
	for (auto &shaderCode: shaderCodes_) {
		const char *source = shaderCode.second.c_str();
		ref_ptr<GLuint> shaderStage = ref_ptr<GLuint>::alloc();
		GLuint &stage = *shaderStage.get();
		stage = glCreateShader(shaderCode.first);
		GLint length = -1;
		GLint status;

		glShaderSource(stage, 1, &source, &length);
		glCompileShader(stage);

		glGetShaderiv(stage, GL_COMPILE_STATUS, &status);
		printLog(stage, shaderCode.first, source, status != 0);
		if (!status) {
			glDeleteShader(stage);
			return false;
		}

		glAttachShader(id(), stage);
		shaders_[shaderCode.first] = shaderStage;
	}

	return true;
}

bool Shader::link() {
	if (!transformFeedback_.empty()) {
		std::vector<const char *> validNames(transformFeedback_.size());
		std::set<std::string> validNames_;
		int validCounter = 0;

		GLenum stage = feedbackStage_;
		GLint i = 0;
		for (; glenum::glslStages()[i] != stage; ++i) {}

		// find next stage
		std::string nextStagePrefix = "out";
		for (GLint j = i + 1; j < glenum::glslStageCount(); ++j) {
			GLenum nextStage = glenum::glslStages()[j];
			if (shaders_.count(nextStage) != 0) {
				nextStagePrefix = glenum::glslStagePrefix(nextStage);
				break;
			}
		}

		for (auto &it: transformFeedback_) {
			std::string name = IOProcessor::getNameWithoutPrefix(it);
			if (name == "Position") {
				name = "gl_" + name;
			} else {
				name = nextStagePrefix + "_" + name;
			}
			if (validNames_.count(name) > 0) { continue; }
			auto needle = validNames_.insert(name);
			validNames[validCounter++] = needle.first->c_str();
		}

		glTransformFeedbackVaryings(id(),
									validCounter, validNames.data(), feedbackLayout_);
	}

	glLinkProgram(id());
	GLint status;
	glGetProgramiv(id(), GL_LINK_STATUS, &status);
	GL_ERROR_LOG();
	if (status == GL_FALSE) {
		for (auto &shaderCode: shaderCodes_) {
			const char *source = shaderCode.second.c_str();
			printLog(id(), shaderCode.first, source, false);
		}
		return false;
	} else {
		printLog(id(), GL_NONE, nullptr, true);
		setupInputLocations();
		return true;
	}
}

bool Shader::validate() {
	glValidateProgram(id());
	GLint status;
	glGetProgramiv(id(), GL_VALIDATE_STATUS, &status);
	if (status == GL_FALSE) {
		int length;
		glGetProgramiv(id(), GL_INFO_LOG_LENGTH, &length);
		char *log = new char[length];
		glGetProgramInfoLog(id(), length, nullptr, log);
		REGEN_WARN("validation failed: " << log);
		delete[]log;
		return false;
	} else {
		return true;
	}
}

void Shader::setupInputLocations() {
	GLint count;
	GLint arraySize;
	GLenum type;
	char nameC[320];

	//inputs_.clear();
	samplerLocations_.clear();

	glGetProgramiv(id(), GL_ACTIVE_UNIFORMS, &count);
	for (GLint loc_ = 0; loc_ < count; ++loc_) {
		glGetActiveUniform(id(), loc_, 320, nullptr, &arraySize, &type, nameC);
		std::string uniformName(nameC);
		// for arrays..
		GLint loc = glGetUniformLocation(id(), nameC);

		// remember this uniform location
		std::string attName(nameC);
		if (hasPrefix(attName, "gl_")) { continue; }
		if (boost::ends_with(uniformName, "[0]")) {
			uniformName = uniformName.substr(0, uniformName.size() - 3);
		}
		if (hasPrefix(uniformName, "u_")) {
			uniformName = truncPrefix(uniformName, "u_");
		} else if (hasPrefix(attName, "in_")) {
			uniformName = truncPrefix(uniformName, "in_");
		}
		if (hasPrefix(uniformName, "instances_")) {
			uniformName = truncPrefix(uniformName, "instances_");
		} else if (hasPrefix(uniformName, "tbo_")) {
			uniformName = truncPrefix(uniformName, "tbo_");
		}
		uniformLocations_[std::string(nameC)] = loc;
		uniformLocations_[uniformName] = loc;
		uniformLocations_[REGEN_STRING("u_" << uniformName)] = loc;
		uniformLocations_[REGEN_STRING("in_" << uniformName)] = loc;

		// create ShaderInput without data allocated.
		// still setupInput must be called with this ShaderInput
		// for the shader to enable it with applyInputs()
		switch (type) {
			case GL_FLOAT:
			case GL_FLOAT_VEC2:
			case GL_FLOAT_VEC3:
			case GL_FLOAT_VEC4:
			case GL_BOOL:
			case GL_UNSIGNED_INT:
			case GL_UNSIGNED_INT_ATOMIC_COUNTER:
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

	// uniform blocks
	glGetProgramiv(id(), GL_ACTIVE_UNIFORM_BLOCKS, &count);
	GLint bindingPoint = 0;
	for (GLint blockIndex = 0; blockIndex < count; ++blockIndex) {
		// Note: uniforms inside a uniform block do not have individual uniform locations,
		// but each block has an index within the shader program.
		glGetActiveUniformBlockName(id(), blockIndex, 320, &arraySize, nameC);
		std::string blockName(truncPrefix(nameC, "in_"));
		uniformLocations_[blockName] = bindingPoint;
		uniformLocations_[REGEN_STRING("u_" << blockName)] = bindingPoint;
		uniformLocations_[REGEN_STRING("in_" << blockName)] = bindingPoint;
		// To bind data to a uniform block, we need to bind the uniform buffer object to a "binding point".
		// We can choose which one but must make sure that the uniform buffer object is bound to the same binding point
		// when the shader is used.
		glUniformBlockBinding(id(), blockIndex, bindingPoint);
		++bindingPoint;
	}

	// shader storage blocks
	glGetProgramInterfaceiv(id(),
			GL_SHADER_STORAGE_BLOCK,
			GL_ACTIVE_RESOURCES, &count);
	for (GLint blockIndex = 0; blockIndex < count; ++blockIndex) {
		glGetProgramResourceName(id(),
				GL_SHADER_STORAGE_BLOCK,
				blockIndex, 320, &arraySize, nameC);
		std::string blockName(truncPrefix(nameC, "in_"));
		uniformLocations_[blockName] = bindingPoint;
		uniformLocations_[REGEN_STRING("in_" << blockName)] = bindingPoint;
		glShaderStorageBlockBinding(id(), blockIndex, bindingPoint);
		++bindingPoint;
	}

	glGetProgramiv(id(), GL_ACTIVE_ATTRIBUTES, &count);
	for (GLint loc_ = 0; loc_ < count; ++loc_) {
		glGetActiveAttrib(id(), loc_, 320, nullptr, &arraySize, &type, nameC);
		GLint loc = glGetAttribLocation(id(), nameC);

		// remember this attribute location
		std::string attName(nameC);
		if (hasPrefix(attName, "gl_")) { continue; }
		if (boost::ends_with(attName, "[0]")) {
			attName = attName.substr(0, attName.size() - 3);
		}
		if (hasPrefix(attName, "a_")) {
			attName = truncPrefix(attName, "a_");
		} else if (hasPrefix(attName, "vs_")) {
			attName = truncPrefix(attName, "vs_");
		} else if (hasPrefix(attName, "in_")) {
			attName = truncPrefix(attName, "in_");
		}
		uniformLocations_[std::string(nameC)] = loc;
		attributeLocations_[attName] = loc;
		attributeLocations_[REGEN_STRING("a_" << attName)] = loc;
		attributeLocations_[REGEN_STRING("in_" << attName)] = loc;
		attributeLocations_[REGEN_STRING("vs_" << attName)] = loc;
	}
}

ref_ptr<ShaderInput> Shader::createUniform(const std::string &name) {
	GLint loc = uniformLocation(name);
	if (loc == -1) {
		REGEN_WARN("Is not an active uniform '" << name << "' shader=" << id());
		return {};
	}

	GLint arraySize;
	GLenum type;
	char nameC[320];
	glGetActiveUniform(id(), loc, 320, nullptr, &arraySize, &type, nameC);

	switch (type) {
		case GL_FLOAT:
			return ref_ptr<ShaderInput1f>::alloc(name, arraySize);
		case GL_FLOAT_VEC2:
			return ref_ptr<ShaderInput2f>::alloc(name, arraySize);
		case GL_FLOAT_VEC3:
			return ref_ptr<ShaderInput3f>::alloc(name, arraySize);
		case GL_FLOAT_VEC4:
			return ref_ptr<ShaderInput4f>::alloc(name, arraySize);
		case GL_BOOL:
		case GL_INT:
			return ref_ptr<ShaderInput1i>::alloc(name, arraySize);
		case GL_BOOL_VEC2:
		case GL_INT_VEC2:
			return ref_ptr<ShaderInput2i>::alloc(name, arraySize);
		case GL_BOOL_VEC3:
		case GL_INT_VEC3:
			return ref_ptr<ShaderInput3i>::alloc(name, arraySize);
		case GL_BOOL_VEC4:
		case GL_INT_VEC4:
			return ref_ptr<ShaderInput4i>::alloc(name, arraySize);
		case GL_FLOAT_MAT2:
			return ref_ptr<ShaderInput4f>::alloc(name, arraySize);
		case GL_FLOAT_MAT3:
			return ref_ptr<ShaderInputMat3>::alloc(name, arraySize);
		case GL_FLOAT_MAT4:
			return ref_ptr<ShaderInputMat4>::alloc(name, arraySize);
		default:
			REGEN_WARN("Not a known uniform type for '" << name << "' type=" << type);
			break;
	}
	return {};
}

void Shader::setInput(const ref_ptr<ShaderInput> &in, const std::string &name) {
	std::string inputName = (name.empty() ? in->name() : name);
	{
		auto needle = inputNames_.find(inputName);
		if (needle == inputNames_.end()) {
			inputs_.emplace_back(in, inputName);
			auto it = inputs_.end();
			--it;
			inputNames_[inputName] = it;
		} else {
			*needle->second = NamedShaderInput(in, inputName);
		}
	}
	if (in->isVertexAttribute()) {
		auto needle = attributeLocations_.find(inputName);
		if (!in->hasData()) { return; }
		if (needle != attributeLocations_.end()) {
			attributes_.emplace_back(in, needle->second);
		}
	} else if (!in->isConstant()) {
		auto needle = uniformLocations_.find(inputName);
		if (needle != uniformLocations_.end()) {
			uniforms_.emplace_back(in, needle->second);
		}
	}
}

bool Shader::setTexture(const ref_ptr<Texture> &tex, const std::string &name) {
	auto needle = samplerLocations_.find(name);
	if (needle == samplerLocations_.end()) return false;
	if (tex.get()) {
		textures_[needle->second] = TextureLocation(name, tex, needle->second);
	} else {
		textures_.erase(needle->second);
	}
	return true;
}

void Shader::setInputs(const std::list<NamedShaderInput> &inputs) {
	for (const auto &input: inputs) { setInput(input.in_, input.name_); }
}

void Shader::setTransformFeedback(const std::list<std::string> &transformFeedback,
								  GLenum attributeLayout, GLenum feedbackStage) {
	feedbackLayout_ = attributeLayout;
	feedbackStage_ = feedbackStage;
	for (const auto &it: transformFeedback) {
		transformFeedback_.push_back(it);
	}
}

//////////////

void Shader::enable(RenderState *) {
	for (auto &uniform: uniforms_) {
		if (!uniform.input->active()) { continue; }
		if (uniform.input->isBufferBlock()) {
			// enable buffer block
			uniform.input->enableUniform(uniform.location);
		} else if (uniform.input->stamp() != uniform.uploadStamp) {
			// enable uniform
			uniform.input->enableUniform(uniform.location);
			uniform.uploadStamp = uniform.input->stamp();
		}
	}
}
