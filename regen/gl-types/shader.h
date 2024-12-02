/*
 * shader.h
 *
 *  Created on: 02.02.2011
 *      Author: daniel
 */

#ifndef _SHADER_H_
#define _SHADER_H_

#include <map>
#include <set>

#include <regen/gl-types/render-state.h>
#include <regen/gl-types/texture.h>
#include <regen/gl-types/shader-input.h>
#include <regen/gl-types/glsl/preprocessor.h>

namespace regen {
	/**
	 * \brief Maps input to shader location.
	 */
	struct ShaderInputLocation {
		ref_ptr<ShaderInput> input; /**< the shader input. */
		GLint location; /**< the input location. */
		GLuint uploadStamp; /**< time stamp of last upload. */
		/**
		 * @param _input input data.
		 * @param _location input location in shader.
		 */
		ShaderInputLocation(const ref_ptr<ShaderInput> &_input, GLint _location)
				: input(_input), location(_location), uploadStamp(0) {}

		ShaderInputLocation()
				: location(-1), uploadStamp(0) {}
	};

	/**
	 * \brief Maps texture to shader location.
	 */
	struct ShaderTextureLocation {
		std::string name;    /**< name in shader. **/
		GLint location; /**< the texture location. */
		ref_ptr<Texture> tex; /**< the texture. */
		GLint uploadChannel; /**< last uploaded channel. */
		/**
		 * @param _name texture name.
		 * @param _tex the texture.
		 * @param _location texture location in shader.
		 */
		ShaderTextureLocation(const std::string &_name, const ref_ptr<Texture> &_tex, GLint _location)
				: name(_name), location(_location), tex(_tex), uploadChannel(-1) {}

		ShaderTextureLocation()
				: name(""), location(-1), uploadChannel(-1) {}
	};

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
				const std::map<std::string, std::string> &_defines =
				std::map<std::string, std::string>(),
				const std::map<std::string, std::string> &_externalFunctions =
				std::map<std::string, std::string>(),
				const std::list<NamedShaderInput> &_specifiedInput =
				std::list<NamedShaderInput>())
				: version(_version),
				  unprocessed(_unprocessed),
				  defines(_defines),
				  externalFunctions(_externalFunctions),
				  specifiedInput(_specifiedInput) {}

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
	};

	/**
	 * \brief a piece of code that is executed on the GPU.
	 *
	 * Encapsulates a GLSL program, helps
	 * compiling and linking together the
	 * shader stages.
	 */
	class Shader {
	public:
		/**
		 * Pre-processor for usual shader loading.
		 * @return the default pre-processor instance.
		 */
		static ref_ptr<PreProcessor> &defaultPreProcessor();

		/**
		 * Pre-processor for loading a single shader stage
		 * (no IOProcessor used).
		 * @return the single stage pre-processor instance.
		 */
		static ref_ptr<PreProcessor> &singleStagePreProcessor();

		/**
		 * Create a new shader or return an identical shader that
		 * was loaded before.
		 */
		static void preProcess(
				std::map<GLenum, std::string> &ret,
				const PreProcessorConfig &cfg,
				const ref_ptr<PreProcessor> &preProcessor = defaultPreProcessor());

		/**
		 * Prints the shader log.
		 * @param shader the shader handle.
		 * @param shaderType shader stage enumeration.
		 * @param shaderCode the GLSL code.
		 * @param success compiling/linking success ?
		 */
		static void printLog(
				GLuint shader,
				GLenum shaderType,
				const char *shaderCode,
				GLboolean success);

		/////////////

		/**
		 * Share GL resource with other shader.
		 * Each shader has an individual configuration only GL resources
		 * are shared.
		 */
		Shader(const Shader &);

		/**
		 * Construct pre-compiled shader.
		 * link() must be called to use this shader.
		 * Note: make sure IO names in stages match each other.
		 */
		Shader(
				const std::map<GLenum, std::string> &shaderCode,
				const std::map<GLenum, ref_ptr<GLuint> > &shaderObjects);

		/**
		 * Create a new shader with given stage map.
		 * compile() and link() must be called to use this shader.
		 */
		explicit Shader(const std::map<GLenum, std::string> &shaderNames);

		~Shader();

		/**
		 * Compiles and attaches shader stages.
		 */
		GLboolean compile();

		/**
		 * Link together previous compiled stages.
		 * Note: For MRT you must call setOutputs before and for
		 * transform feedback you must call setTransformFeedback before.
		 */
		GLboolean link();

		/**
		 * @return GL_TRUE if the validation was successful.
		 */
		GLboolean validate();

		/**
		 * The program object.
		 */
		GLint id() const;

		/**
		 * Returns true if the given name is a valid vertex attribute name.
		 */
		GLboolean isAttribute(const std::string &name) const;

		/**
		 * Returns the locations for a given vertex attribute name or -1 if the name is not known.
		 */
		GLint attributeLocation(const std::string &name);

		/**
		 * Returns true if the given name is a valid uniform name
		 * and the uniform was added to the shader using setInput().
		 */
		GLboolean hasUniform(const std::string &name) const;

		/**
		 * Returns the location for a given uniform name or -1 if the name is not known.
		 */
		GLint uniformLocation(const std::string &name);

		/**
		 * Returns the location for a given uniform block name or -1 if the name is not known.
		 */
		GLint uniformBlockLocation(const std::string &name);

		/**
		 * Returns true if the given name is a valid uniform name and
		 * the uniform has some data set (no null pointer data).
		 */
		GLboolean hasUniformData(const std::string &name) const;

		/**
		 * Returns true if the given name is a valid sampler name.
		 * and the texture was added to the shader using setTexture().
		 */
		GLboolean hasSampler(const std::string &name) const;

		/**
		 * Returns the location for a given sampler name or -1 if the name is not known.
		 */
		GLint samplerLocation(const std::string &name);

		/**
		 * Returns inputs for this shader.
		 * Each attribute and uniform will appear in this map after the
		 * program was linked with a NULL data pointer.
		 * You can overwrite these with setInput or you can allocate data
		 * for the inputs as returned by this function.
		 */
		const ShaderInputList &inputs() const;

		/**
		 * @return list of textures attached to this shader.
		 */
		const std::map<GLint, ShaderTextureLocation> &textures() const;

		/**
		 * @return list of attributes attached to this shader.
		 */
		const std::list<ShaderInputLocation> &attributes() const;

		/**
		 * @return list of uniforms attached to this shader.
		 */
		auto &uniforms() const { return uniforms_; }

		/**
		 * Returns input with given name.
		 */
		ref_ptr<ShaderInput> input(const std::string &name);

		/**
		 * Set a single shader input. Inputs are automatically
		 * setup when the shader is enabled.
		 */
		void setInput(const ref_ptr<ShaderInput> &in, const std::string &name = "");

		/**
		 * Set a set of shader inputs for this program.
		 */
		void setInputs(const std::list<NamedShaderInput> &inputs);

		/**
		 * Set a single texture for this program.
		 * channel must point to the channel the texture is bound to.
		 */
		GLboolean setTexture(const ref_ptr<Texture> &tex, const std::string &name);

		/**
		 * Returns shader stage GL handle from enumeration.
		 * Enumaretion may be GL_VERTEX_SHADER, GL_FRAGMENT_SHADER,
		 * GL_GEOMETRY_SHADER, ...
		 * Returns a NULL reference if no such shader stage is used.
		 */
		ref_ptr<GLuint> stage(GLenum stage) const;

		/**
		 * Returns shader stage GLSL code from enumeration.
		 * Enumeration may be GL_VERTEX_SHADER, GL_FRAGMENT_SHADER,
		 * GL_GEOMETRY_SHADER, ...
		 */
		const std::string &stageCode(GLenum stage) const;

		/**
		 * Returns true if the given stage enumeration is used
		 * in this program.
		 * Enumeration may be GL_VERTEX_SHADER, GL_FRAGMENT_SHADER,
		 * GL_GEOMETRY_SHADER, ...
		 */
		GLboolean hasStage(GLenum stage) const;

		/**
		 * Must be done before linking for transform feedback.
		 */
		void setTransformFeedback(
				const std::list<std::string> &transformFeedback,
				GLenum attributeLayout,
				GLenum feedbackStage);

		/**
		 * Create uniform based on declaration in shader.
		 * @param name the uniform name
		 * @return new ShaderInput instance
		 */
		ref_ptr<ShaderInput> createUniform(const std::string &name);

		/**
		 * Enables states attached to shader.
		 */
		void enable(RenderState *rs);

	protected:
		// the GL shader handle that can be shared by multiple Shader's
		ref_ptr<GLuint> id_;

		// shader codes without replaced input prefix
		std::map<GLenum, std::string> shaderCodes_;
		// compiled shader objects
		std::map<GLenum, ref_ptr<GLuint> > shaders_;

		// location maps
		std::map<std::string, GLint> samplerLocations_;
		std::map<std::string, GLint> uniformLocations_;
		std::map<std::string, GLint> uniformBlockLocations_;
		std::map<std::string, GLint> attributeLocations_;

		std::list<ShaderInputLocation> attributes_;
		std::list<ShaderInputLocation> uniforms_;
		std::map<GLint, ShaderTextureLocation> textures_;
		// available inputs
		ShaderInputList inputs_;
		std::map<std::string, ShaderInputList::iterator> inputNames_;

		std::list<std::string> transformFeedback_;
		GLenum feedbackLayout_;
		GLenum feedbackStage_;

		void setupInputLocations();
	};
} // namespace

#endif /* _SHADER_H_ */
