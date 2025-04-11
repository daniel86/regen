#ifndef REGEN_STATE_CONFIG_H_
#define REGEN_STATE_CONFIG_H_

#include <map>
#include <string>

#include <regen/textures/texture.h>
#include <regen/gl-types/shader-input.h>

namespace regen {
	/**
	 * \brief Configures a Shader object.
	 *
	 * Configuration is done using macros.
	 * Using the GLSL preprocessors it is also possible to change
	 * the type of shader input data.
	 * Transform feedback configuration is also handled because
	 * the info is needed in advance of linking the shader.
	 */
	struct StateConfig {
	public:
		StateConfig()
				: feedbackMode_(GL_INTERLEAVED_ATTRIBS),
				  feedbackStage_(GL_VERTEX_SHADER) {
			version_ = 130;
#ifdef GLEW_ARB_tessellation_shader
			defines_["HAS_tessellation_shader"] = "TRUE";
#endif
		}

		/**
		 * Copy constructor.
		 */
		StateConfig(const StateConfig &other) {
			functions_ = other.functions_;
			defines_ = other.defines_;
			includes_ = other.includes_;
			inputs_ = other.inputs_;
			textures_ = other.textures_;
			feedbackAttributes_ = other.feedbackAttributes_;
			feedbackMode_ = other.feedbackMode_;
			feedbackStage_ = other.feedbackStage_;
			version_ = other.version_;
		}

		/**
		 * @param version the GLSL version.
		 */
		void setVersion(GLuint version) { if (version > version_) version_ = version; }

		/**
		 * @return the GLSL version.
		 */
		GLuint version() const { return version_; }

		/**
		 * Macro key-value map. Macros are prepended to loaded shaders.
		 */
		std::map<std::string, std::string> defines_;
		/**
		 * GLSL code to be included.
		 */
		std::vector<std::string> includes_;
		/**
		 * User defined GLSL functions for the shader.
		 */
		std::map<std::string, std::string> functions_;
		/**
		 * Specified shader input data.
		 */
		ShaderInputList inputs_;
		/**
		 * Specified shader textures.
		 */
		std::map<std::string, ref_ptr<Texture> > textures_;
		/**
		 * List of attribute names to capture using transform feedback.
		 */
		std::list<std::string> feedbackAttributes_;
		/**
		 * Interleaved or separate ?
		 */
		GLenum feedbackMode_;
		/**
		 * Capture output of this shader stage.
		 */
		GLenum feedbackStage_;
		/**
		 * Number of object instances used in the shader.
		 */
		unsigned int numInstances_ = 1;

	protected:
		GLuint version_;
	};
} // namespace

#endif /* REGEN_STATE_CONFIG_H_ */
