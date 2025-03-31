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
#include <regen/textures/texture.h>
#include "io-processor.h"
#include "regen/gl-types/ubo.h"

using namespace regen;
using namespace std;

///////////////////////

IOProcessor::InputOutput::InputOutput()
		: layout(""),
		  interpolation(""),
		  ioType(""),
		  dataType(""),
		  name(""),
		  numElements(""),
		  value("") {}

IOProcessor::InputOutput::InputOutput(const InputOutput &other)
		: layout(other.layout),
		  interpolation(other.interpolation),
		  ioType(other.ioType),
		  dataType(other.dataType),
		  name(other.name),
		  numElements(other.numElements),
		  value(other.value),
		  block(other.block),
		  requiresArrayElements(other.requiresArrayElements) {}

string IOProcessor::InputOutput::declaration() {
	stringstream ss;
	if (!layout.empty()) { ss << layout << " "; }
	if (!interpolation.empty()) { ss << interpolation << " "; }
	ss << ioType << " " << dataType << " " << name;
	if (!numElements.empty()) {
		if (requiresArrayElements) {
			ss << "[" << numElements << "]";
		} else {
			ss << "[]";
		}
	}
	if (!value.empty()) { ss << " = " << value; }
	if (!block.empty()) {
		ss << "{" << endl;
		for (auto & it : block) {
			ss << '\t' << it << endl;
		}
		ss << "}";
	}
	ss << ";";
	return ss.str();
}

//////////////////

IOProcessor::IOProcessor()
		: GLSLProcessor("InputOutput"),
		  isInputSpecified_(GL_FALSE),
		  currStage_(-1) {
}

string IOProcessor::getNameWithoutPrefix(const string &name) {
	static const string prefixes[] = {"in_", "out_", "u_", "c_", "gs_", "fs_", "vs_", "tes_", "tcs_"};
	for (const auto & prefix : prefixes) {
		if (hasPrefix(name, prefix)) {
			return truncPrefix(name, prefix);
		}
	}
	return name;
}

void IOProcessor::defineHandleIO(PreProcessorState &state) {
	list<InputOutput> genOut, genIn;
	map<string, InputOutput> &nextInputs = inputs_[state.nextStage];
	map<string, InputOutput> &inputs = inputs_[state.currStage];
	map<string, InputOutput> &outputs = outputs_[state.currStage];

	// for each input of the next stage
	// make sure it is declared at least as output in this stage
	for (auto & nextInput : nextInputs) {
		const string &nameWithoutPrefix = nextInput.first;
		const InputOutput &nextIn = nextInput.second;

		if (outputs.count(nameWithoutPrefix) > 0) { continue; }
		genOut.push_back(InputOutput(nextIn));
		genOut.back().name = "out_" + nameWithoutPrefix;
		genOut.back().ioType = "out";
		genOut.back().interpolation = nextIn.interpolation;
		if (state.currStage == GL_GEOMETRY_SHADER) {
			genOut.back().numElements = "";
		} else if (state.currStage == GL_VERTEX_SHADER) {
			genOut.back().numElements = "";
		}
#ifdef GL_TESS_EVALUATION_SHADER
		else if (state.currStage == GL_TESS_EVALUATION_SHADER) {
			genOut.back().numElements = "";
		}
#endif
#ifdef GL_TESS_CONTROL_SHADER
		else if (state.currStage == GL_TESS_CONTROL_SHADER) {
			genOut.back().numElements = " ";
		}
#endif
		outputs.insert(make_pair(nameWithoutPrefix, genOut.back()));

		if (inputs.count(nameWithoutPrefix) > 0) { continue; }
		genIn.push_back(InputOutput(nextIn));
		genIn.back().name = "in_" + nameWithoutPrefix;
		genIn.back().ioType = "in";
		genIn.back().interpolation = nextIn.interpolation;
		if (state.currStage == GL_GEOMETRY_SHADER) {
			genIn.back().numElements = " ";
		} else if (state.currStage == GL_VERTEX_SHADER) {
			genIn.back().numElements = "";
		}
#ifdef GL_TESS_EVALUATION_SHADER
		else if (state.currStage == GL_TESS_EVALUATION_SHADER) {
			genIn.back().numElements = " ";
		}
#endif
#ifdef GL_TESS_CONTROL_SHADER
		else if (state.currStage == GL_TESS_CONTROL_SHADER) {
			genIn.back().numElements = " ";
		}
#endif
		inputs.insert(make_pair(nameWithoutPrefix, genIn.back()));
	}

	if (genOut.empty() && genIn.empty()) {
		lineQueue_.push_back("#define HANDLE_IO(i)");
		return;
	}

	// declare IO:
	//    * insert a redefinition of the IO name using the stage prefix
	//    * just insert the previous declaration again
	for (auto & it : genIn) {
		lineQueue_.push_back("#define " + it.name + " " +
							 glenum::glslStagePrefix(state.currStage) + "_" + getNameWithoutPrefix(it.name));
		lineQueue_.push_back(it.declaration());
	}
	for (auto & it : genOut) {
		lineQueue_.push_back("#define " + it.name + " " +
							 glenum::glslStagePrefix(state.nextStage) + "_" + getNameWithoutPrefix(it.name));
		lineQueue_.push_back(it.declaration());
	}

	// declare HANDLE_IO() function
	lineQueue_.push_back("void HANDLE_IO(int i) {");
	for (auto & io : genOut) {
		const string &outName = io.name;
		string inName = inputs[getNameWithoutPrefix(outName)].name;

		switch (state.currStage) {
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

IOProcessor::InputOutput IOProcessor::getUniformIO(const NamedShaderInput &uniform) {
	string nameWithoutPrefix = getNameWithoutPrefix(uniform.name_.empty() ?
			uniform.in_->name() : uniform.name_);

	IOProcessor::InputOutput io;
	io.layout = "";
	io.interpolation = "";
	io.ioType = "uniform";
	io.value = "";
	GLuint numElements = uniform.in_->numArrayElements() * uniform.in_->numInstances();
	io.numElements = (numElements > 1 || uniform.in_->forceArray()) ?
					 REGEN_STRING(numElements) : "";
	if (uniform.in_->numInstances()>1 && currStage_ != GL_COMPUTE_SHADER) {
		io.name = "instances_" + nameWithoutPrefix;
	} else {
		io.name = "in_" + nameWithoutPrefix;
	}

	auto *tex = dynamic_cast<Texture *>(uniform.in_.get());
	if (tex == nullptr) {
		io.dataType = glenum::glslDataType(uniform.in_->dataType(), uniform.in_->valsPerElement());
	} else {
		io.dataType = tex->samplerType();
	}

	return io;
}

void IOProcessor::declareSpecifiedInput(PreProcessorState &state) {
	list<NamedShaderInput> specifiedInput = state.in.specifiedInput;
	InputOutput io;
	io.layout = "";
	io.interpolation = "";

	// move all uniform blocks to the begin in specifiedInput
	std::vector<NamedShaderInput> uniformBlocks;
	auto it = specifiedInput.begin();
	while (it != specifiedInput.end()) {
		if (it->in_->isBufferBlock()) {
			uniformBlocks.push_back(*it);
			specifiedInput.erase(it++);
		} else {
			++it;
		}
	}
	specifiedInput.insert(specifiedInput.begin(), uniformBlocks.begin(), uniformBlocks.end());


	for (auto & it : specifiedInput) {
		ref_ptr<ShaderInput> in = it.in_;
		string nameWithoutPrefix = getNameWithoutPrefix(it.name_);
		if (inputNames_.count(nameWithoutPrefix)) continue;
		io.block.clear();
		io.layout.clear();

		if (it.type_.empty()) {
			auto *tex = dynamic_cast<Texture *>(in.get());
			if (tex == nullptr) {
				io.dataType = glenum::glslDataType(in->dataType(), in->valsPerElement());
			} else {
				io.dataType = tex->samplerType();
			}
		} else {
			io.dataType = it.type_;
		}
		GLuint numElements = in->numArrayElements() * in->numInstances();
		io.numElements = (numElements > 1 || in->forceArray()) ?
						 REGEN_STRING(numElements) : "";
		if (in->numInstances() > 1 && currStage_ != GL_COMPUTE_SHADER) {
			io.name = "instances_" + nameWithoutPrefix;
			lineQueue_.push_back(REGEN_STRING("#define in_" << nameWithoutPrefix <<
					" instances_" << nameWithoutPrefix << "[regen_InstanceID]"));
		} else {
			io.name = "in_" + nameWithoutPrefix;
		}

		if (in->isVertexAttribute()) {
			if (state.currStage != GL_VERTEX_SHADER) continue;
			io.ioType = "in";
			io.value = "";
			inputs_[state.currStage].insert(make_pair(nameWithoutPrefix, io));

			lineQueue_.push_back(REGEN_STRING("#define " << io.name << " " <<
														 glenum::glslStagePrefix(state.currStage) << "_"
														 << nameWithoutPrefix));
		} else if (in->isConstant()) {
			io.ioType = "const";

			stringstream val;
			val << io.dataType << "(";
			(*in.get()).write(val);
			val << ")";
			io.value = val.str();
		} else if (in->isBufferBlock()) {
			auto *block = dynamic_cast<BufferBlock *>(in.get());
			bool isSSBO = block->storageQualifier() == BufferBlock::StorageQualifier::BUFFER;
			std::stringstream layoutStr;
			layoutStr << "layout(";
			layoutStr << REGEN_STRING(block->memoryLayout());
			layoutStr << ") ";
			io.layout = layoutStr.str();
			io.ioType = REGEN_STRING(block->storageQualifier());
			io.value = "";
			io.dataType = "";
			for (int i=0; i<block->blockInputs().size(); i++) {
				auto &blockUniform = block->blockInputs()[i];
				auto memberIO = getUniformIO(blockUniform);
				auto blockNameWithoutPrefix = getNameWithoutPrefix(blockUniform.name_.empty() ?
						blockUniform.in_->name() : blockUniform.name_);
				memberIO.ioType = "";
				// last element in SSBO can omit the array size
				if (isSSBO && i == block->blockInputs().size() - 1) {
					memberIO.requiresArrayElements = false;
				}
				io.block.push_back(memberIO.declaration());
				inputNames_.insert(blockNameWithoutPrefix);

				if (blockUniform.in_->numInstances() > 1 && currStage_ != GL_COMPUTE_SHADER) {
					lineQueue_.push_back(REGEN_STRING("#define in_" << blockNameWithoutPrefix <<
						" instances_" << blockNameWithoutPrefix << "[regen_InstanceID]"));
				}
			}
			uniforms_[state.currStage].insert(make_pair(nameWithoutPrefix, io));
		} else {
			io.ioType = "uniform";
			io.value = "";
			uniforms_[state.currStage].insert(make_pair(nameWithoutPrefix, io));
		}

		lineQueue_.push_back(io.declaration());
		inputNames_.insert(nameWithoutPrefix);
	}
}

void IOProcessor::parseValue(string &v, string &val) {
	static const char *pattern_ = "[ ]*([^= ]+)[ ]*=[ ]*([^;]+)[ ]*";
	static boost::regex regex_(pattern_);

	boost::sregex_iterator it(v.begin(), v.end(), regex_);
	if (it != NO_REGEX_MATCH) {
		val = (*it)[2];
		v = (*it)[1];
	}
}

void IOProcessor::parseArray(string &v, string &numElements) {
	static const char *pattern_ = "([^\\[]+)\\[([^\\]]*)\\]";
	static boost::regex regex_(pattern_);

	boost::sregex_iterator it(v.begin(), v.end(), regex_);
	if (it != NO_REGEX_MATCH) {
		numElements = (*it)[2];
		v = (*it)[1];
	}
}

void IOProcessor::clear() {
	inputs_.clear();
	outputs_.clear();
	uniforms_.clear();
	lineQueue_.clear();
	inputNames_.clear();
	isInputSpecified_ = GL_FALSE;
	currStage_ = -1;
}

bool IOProcessor::process(PreProcessorState &state, string &line) {
	static const char *interpolationPattern_ =
			"^[ |\t|]*((flat|noperspective|smooth|centroid)[ |\t]+(.*))$";
	static boost::regex interpolationRegex_(interpolationPattern_);
	static const char *pattern_ =
			"^[ |\t|]*((in|uniform|buffer|const|out)[ |\t]+([^ ]*)[ |\t]+([^;]+);)$";
	static boost::regex regex_(pattern_);
	static const char *handleIOPattern_ =
			"^[ |\t]*#define[ |\t]+HANDLE_IO[ |\t]*";
	static boost::regex handleIORegex_(handleIOPattern_);
	static const char *macroPattern_ = "^[ |\t]*#(.*)$";
	static boost::regex macroRegex_(macroPattern_);

	if (currStage_ != state.currStage) {
		inputNames_.clear();
		isInputSpecified_ = GL_FALSE;
		currStage_ = state.currStage;
	}

	// read a line from the queue
	if (!lineQueue_.empty()) {
		line = lineQueue_.front();
		lineQueue_.pop_front();
		return true;
	}
	// read a line from the input stream
	if (!getlineParent(state, line)) {
		return false;
	}
	boost::sregex_iterator it;

	// Insert declarations of specified inputs when the first
	// code line occurs.
	if (!isInputSpecified_) {
		it = boost::sregex_iterator(line.begin(), line.end(), macroRegex_);
		if (it == NO_REGEX_MATCH) {
			declareSpecifiedInput(state);
			isInputSpecified_ = GL_TRUE;
		}
	}

	// Processing of HANLDE_IO macro.
	it = boost::sregex_iterator(line.begin(), line.end(), handleIORegex_);
	if (it != NO_REGEX_MATCH) {
		if (!isInputSpecified_) {
			declareSpecifiedInput(state);
			isInputSpecified_ = GL_TRUE;
		}
		defineHandleIO(state);
		return process(state, line);
	}

	// Parse input declaration.
	InputOutput io;
	it = boost::sregex_iterator(line.begin(), line.end(), interpolationRegex_);
	if (it == NO_REGEX_MATCH) {
		it = boost::sregex_iterator(line.begin(), line.end(), regex_);
	} else {
		// interpolation qualifier specified
		io.interpolation = (*it)[2];
		line = (*it)[3];
		it = boost::sregex_iterator(line.begin(), line.end(), regex_);
	}
	// No input declaration found.
	if (it == NO_REGEX_MATCH) {
		lineQueue_.push_back(line);
		return process(state, line);
	}

	io.ioType = (*it)[2];
	io.dataType = (*it)[3];
	io.name = (*it)[4];
	io.numElements = "";
	io.value = "";
	io.layout = "";
	io.block.clear();

	parseArray(io.dataType, io.numElements);
	parseValue(io.name, io.value);
	parseArray(io.name, io.numElements);

	string nameWithoutPrefix = getNameWithoutPrefix(io.name);
	if (io.ioType != "out") {
		// skip input if already defined
		if (inputNames_.count(nameWithoutPrefix) > 0)
			return process(state, line);
		inputNames_.insert(nameWithoutPrefix);

		// Change IO type based on specified input.
		list<NamedShaderInput>::const_iterator needle;
		for (needle = state.in.specifiedInput.begin();
			 needle != state.in.specifiedInput.end(); ++needle) {
			if (needle->name_ == nameWithoutPrefix) {
				// Change declaration based on specified input
				const ref_ptr<ShaderInput> &in = needle->in_;
				if (in->isVertexAttribute()) {
					io.ioType = "in";
					io.value = "";
				} else if (in->isConstant()) {
					io.ioType = "const";

					stringstream val;
					val << io.dataType << "(";
					(*in.get()).write(val);
					val << ")";
					io.value = val.str();
				} else {
					io.ioType = "uniform";
					io.value = "";
				}
				break;
			}
		}
	}

	if (io.ioType == "in") {
		// define input name with matching prefix
		lineQueue_.push_back(REGEN_STRING("#define " << io.name << " " <<
													 glenum::glslStagePrefix(state.currStage) << "_"
													 << nameWithoutPrefix));
	} else if (io.ioType == "out" && state.nextStage != GL_NONE) {
		// define output name with matching prefix
		lineQueue_.push_back(REGEN_STRING("#define " << io.name << " " <<
													 glenum::glslStagePrefix(state.nextStage) << "_"
													 << nameWithoutPrefix));
	}
	line = io.declaration();

	if (io.ioType == "out") {
		outputs_[state.currStage].insert(make_pair(nameWithoutPrefix, io));
	} else if (io.ioType == "in") {
		inputs_[state.currStage].insert(make_pair(nameWithoutPrefix, io));
	} else if (io.ioType == "uniform" || io.ioType == "buffer") {
		uniforms_[state.currStage].insert(make_pair(nameWithoutPrefix, io));
	}

	lineQueue_.push_back(line);
	return process(state, line);
}
