#ifndef REGEN_PREPROCESSOR_CONFIG_H_
#define REGEN_PREPROCESSOR_CONFIG_H_

#include <regen/gl-types/shader-input.h>

namespace regen {
	/**
	 * \brief Configuration for GLSL pre-processing.
	 */
	struct PreProcessorConfig {
		/**
		 * Default constructor.
		 */
		PreProcessorConfig(
				GLuint _version,
				const std::map<GLenum, std::string> &_unprocessed,
				const std::map<std::string, std::string> &_defines = std::map<std::string, std::string>(),
				const std::map<std::string, std::string> &_externalFunctions = std::map<std::string, std::string>(),
				const std::list<NamedShaderInput> &_specifiedInput = std::list<NamedShaderInput>(),
				const std::vector<std::string> &_includes = std::vector<std::string>())
				: version(_version),
				  unprocessed(_unprocessed),
				  defines(_defines),
				  externalFunctions(_externalFunctions),
				  specifiedInput(_specifiedInput),
				  includes(_includes) {}

		/** GLSL version. */
		GLuint version;
		/** Input GLSL code. */
		const std::map<GLenum, std::string> &unprocessed;
		/** Shader configuration using macros. */
		const std::map<std::string, std::string> &defines;
		/** External GLSL functions. */
		const std::map<std::string, std::string> &externalFunctions;
		/** Input data specification. */
		const std::list<NamedShaderInput> &specifiedInput;
		/** Included GLSL code. */
		const std::vector<std::string> &includes;
	};
} // namespace

#endif /* REGEN_PREPROCESSOR_CONFIG_H_ */
