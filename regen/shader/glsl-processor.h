#ifndef REGEN_SHADER_PROCESSOR_H_
#define REGEN_SHADER_PROCESSOR_H_

#include <map>
#include <sstream>

#include "shader-input.h"
#include "regen/utility/logging.h"

namespace regen {
	/**
	 * \brief Specifies input for GLSL pre-processors.
	 */
	struct PreProcessorInput {
		/**
		 * Default-Constructor.
		 */
		PreProcessorInput(
				const std::string &_header,
				const std::map<GLenum, std::string> &_unprocessed,
				const std::map<std::string, std::string> &_externFunctions,
				const std::list<NamedShaderInput> &_specifiedInput)
				: header(_header),
				  unprocessed(_unprocessed),
				  externFunctions(_externFunctions),
				  specifiedInput(_specifiedInput) {}

		/**
		 * Copy-Constructor.
		 */
		PreProcessorInput(const PreProcessorInput &other)
				: header(other.header),
				  unprocessed(other.unprocessed),
				  externFunctions(other.externFunctions),
				  specifiedInput(other.specifiedInput) {}

		/** Header is prepended to input stream. */
		const std::string &header;
		/** Unprocessed GLSL code. */
		const std::map<GLenum, std::string> &unprocessed;
		/** Extern function definitions. */
		const std::map<std::string, std::string> &externFunctions;
		/** Input data configuration. */
		const std::list<NamedShaderInput> &specifiedInput;
	};

	/**
	 * \brief The current state of pre-processing a GLSL code.
	 */
	struct PreProcessorState {
		/**
		 * Default constructor.
		 * @param _in input for GLSL pre-processors.
		 */
		PreProcessorState(const PreProcessorInput &_in)
				: in(_in), nextStage(GL_NONE), currStage(GL_NONE), version(150) {}

		/** Input for GLSL pre-processors. */
		PreProcessorInput in;
		/** Shader stage following the current stage in the shader pipeline. */
		GLenum nextStage;
		/** The currently pre-processed shader stage. */
		GLenum currStage;
		/** The minimum GLSL version */
		uint32_t version;
		/** Input stream providing unprocessed GLSL code. */
		std::stringstream inStream;
	};

	/**
	 * \brief Baseclass for GLSL pre-processors.
	 *
	 * The stream-like interface of GLSLProcessor is used
	 * to process a sequence of pre-processors line-by-line.
	 */
	class GLSLProcessor {
	public:
		/**
		 * Default Constructor.
		 * @param name The processor name.
		 */
		GLSLProcessor(const std::string &name) : name_(name) {}

		virtual ~GLSLProcessor() {}

		/**
		 * Sets the parent pre-processor. Most pre-processors take their input
		 * from the parent processor.
		 * @param parent the parent processor.
		 */
		void setParent(const ref_ptr<GLSLProcessor> &parent) { parent_ = parent; }

		/**
		 * @return the parent processor.
		 */
		const ref_ptr<GLSLProcessor> &getParent() { return parent_; }

		/**
		 * Get the next line from the processor.
		 * @param state The processor state.
		 * @param line The line output.
		 * @return true on success.
		 */
		bool getline(PreProcessorState &state, std::string &line) {
			bool success = process(state, line);
#ifdef DEBUG_GLSL_PREPROCESSOR
			REGEN_DEBUG("[GLSL] " << name_ << ": " << line);
#endif
			return success;
		}

		/**
		 * Produce a single line.
		 * @param state the pre-processor state.
		 * @param line the line return.
		 * @return true on success
		 */
		virtual bool process(PreProcessorState &state, std::string &line) = 0;

		/**
		 * Clear is called to reset the processor to initial state.
		 */
		virtual void clear() {}

	protected:
		ref_ptr<GLSLProcessor> parent_;
		const std::string name_;

		bool getlineParent(PreProcessorState &state, std::string &line) {
			if (parent_.get()) {
				return parent_->getline(state, line);
			} else {
				return false;
			}
		}
	};

	/**
	 * \brief Simple GLSL pre-processor that
	 * provides the unprocessed GLSL code to child processors.
	 */
	class InputProviderProcessor : public GLSLProcessor {
	public:
		InputProviderProcessor() : GLSLProcessor("InputProvider") {}

		// override
		bool process(PreProcessorState &state, std::string &line) {
			return static_cast<bool>(std::getline(state.inStream, line));
		}
	};

	/**
	 * \brief Removes empty lines from GLSL code.
	 */
	class WhiteSpaceProcessor : public GLSLProcessor {
	public:
		WhiteSpaceProcessor() : GLSLProcessor("WhiteSpace") {}

		// override
		bool process(PreProcessorState &state, std::string &line) {
			if (!getlineParent(state, line)) return false;

			if (line.find_first_not_of(' ') != std::string::npos) {
				// There's a non-space.
				return true;
			} else {
				// empty string
				return getline(state, line);
			}
		}
	};

	/**
	 * \brief Simple GLSL pre-processor that
	 * provides input from an arbitrary stringstream.
	 */
	class StreamProcessor : public GLSLProcessor {
	public:
		StreamProcessor() : GLSLProcessor("Stream") {}

		/**
		 * Constructor that fills the stringstream with initial data.
		 * @param v the initial string data.
		 */
		StreamProcessor(const std::string &v) : GLSLProcessor("Stream"), ss_(v) {}

		/**
		 * @return the attached stringstream.
		 */
		std::stringstream &stream() { return ss_; }

		// override
		bool process(PreProcessorState &state, std::string &line) { return static_cast<bool>(std::getline(ss_, line)); }

	protected:
		std::stringstream ss_;
	};
}

#endif /* REGEN_SHADER_PROCESSOR_H_ */
