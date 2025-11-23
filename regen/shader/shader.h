#ifndef REGEN_SHADER_H_
#define REGEN_SHADER_H_

#include <map>
#include <set>

#include "regen/gl-types/render-state.h"
#include "regen/gl-types/input-location.h"
#include "regen/textures/texture-location.h"
#include "regen/shader/preprocessor.h"
#include "regen/shader/preprocessor-config.h"

namespace regen {
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
				uint32_t shader,
				GLenum shaderType,
				const char *shaderCode,
				bool success);

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
				const std::map<GLenum, ref_ptr<uint32_t> > &shaderObjects);

		/**
		 * Create a new shader with given stage map.
		 * compile() and link() must be called to use this shader.
		 */
		explicit Shader(const std::map<GLenum, std::string> &shaderNames);

		~Shader();

		/**
		 * Compiles and attaches shader stages.
		 */
		bool compile();

		/**
		 * Link together previous compiled stages.
		 * Note: For MRT you must call setOutputs before and for
		 * transform feedback you must call setTransformFeedback before.
		 */
		bool link();

		/**
		 * @return true if the validation was successful.
		 */
		bool validate();

		/**
		 * The program object.
		 */
		unsigned int id() const;

		/**
		 * Returns true if the given name is a valid vertex attribute name.
		 */
		bool isAttribute(const std::string &name) const;

		/**
		 * Returns the locations for a given vertex attribute name or -1 if the name is not known.
		 */
		int attributeLocation(const std::string &name);

		/**
		 * Returns true if the given name is a valid uniform name
		 * and the uniform was added to the shader using setInput().
		 */
		bool hasUniform(const std::string &name) const;

		/**
		 * Returns the location for a given uniform name or -1 if the name is not known.
		 */
		int uniformLocation(const std::string &name);

		/**
		 * Returns true if the given name is a valid uniform name and
		 * the uniform has some data set (no null pointer data).
		 */
		bool hasUniformData(const std::string &name) const;

		/**
		 * Returns true if the given name is a valid sampler name.
		 * and the texture was added to the shader using setTexture().
		 */
		bool hasSampler(const std::string &name) const;

		/**
		 * Returns the location for a given sampler name or -1 if the name is not known.
		 */
		int samplerLocation(const std::string &name);

		/**
		 * Returns inputs for this shader.
		 * Each attribute and uniform will appear in this map after the
		 * program was linked with a NULL data pointer.
		 * You can overwrite these with setInput or you can allocate data
		 * for the inputs as returned by this function.
		 */
		auto &inputs() const { return inputs_; }

		/**
		 * @return list of textures attached to this shader.
		 */
		auto &textures() const { return textures_; }

		/**
		 * @return list of attributes attached to this shader.
		 */
		auto &attributes() const { return attributes_; }

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
		bool setTexture(const ref_ptr<Texture> &tex, const std::string &name);

		/**
		 * Returns shader stage GL handle from enumeration.
		 * Enumaretion may be GL_VERTEX_SHADER, GL_FRAGMENT_SHADER,
		 * GL_GEOMETRY_SHADER, ...
		 * Returns a NULL reference if no such shader stage is used.
		 */
		ref_ptr<uint32_t> stage(GLenum stage) const;

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
		bool hasStage(GLenum stage) const;

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
		ref_ptr<unsigned int> id_;

		// shader codes without replaced input prefix
		std::map<GLenum, std::string> shaderCodes_;
		// compiled shader objects
		std::map<GLenum, ref_ptr<unsigned int> > shaders_;

		// location maps
		std::map<std::string, int> samplerLocations_;
		std::map<std::string, int> uniformLocations_;
		std::map<std::string, int> attributeLocations_;

		std::list<InputLocation> attributes_;
		std::list<InputLocation> uniforms_;
		std::map<int, TextureLocation> textures_;
		// available inputs
		ShaderInputList inputs_;
		std::map<std::string, ShaderInputList::iterator> inputNames_;

		std::list<std::string> transformFeedback_;
		GLenum feedbackLayout_;
		GLenum feedbackStage_;

		void setupInputLocations();
	};
} // namespace

#endif /* REGEN_SHADER_H_ */
