#ifndef REGEN_SHADER_STATE_H_
#define REGEN_SHADER_STATE_H_

#include "regen/states/state.h"
#include "regen/textures/texture-state.h"
#include "regen/states/light-state.h"
#include "shader.h"

namespace regen {
	/**
	 * \brief Binds Shader program to the RenderState.
	 */
	class ShaderState : public State {
	public:
		/**
		 * @param shader the shader object.
		 */
		explicit ShaderState(const ref_ptr<Shader> &shader);

		ShaderState();

		static ref_ptr<ShaderState> load(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * Load, compile and link shader with given include key.
		 * @param cfg the shader config.
		 * @param shaderKey the shader include key.
		 * @return GL_TRUE on success.
		 */
		GLboolean createShader(const StateConfig &cfg, const std::string &shaderKey);

		GLboolean createShader(const StateConfig &cfg, const std::vector<std::string> &shaderKeys);

		/**
		 * @return the shader object.
		 */
		const ref_ptr<Shader> &shader() const;

		/**
		 * @param shader the shader object.
		 */
		void set_shader(const ref_ptr<Shader> &shader);

		// overwrite
		void enable(RenderState *) override;

		static ref_ptr<Shader> findShader(State *s);

		static ref_ptr<Shader> findShader(StateNode *n);

	protected:
		ref_ptr<Shader> shader_;

		GLboolean createShader(const StateConfig &cfg, const std::map<GLenum, std::string> &unprocessedCode);

		void loadStage(
				const std::map<std::string, std::string> &shaderConfig,
				const std::string &effectName,
				std::map<GLenum, std::string> &code,
				GLenum stage);
	};
} // namespace

namespace regen {
	/**
	 * \brief can be used to mix in a shader.
	 */
	class HasShader {
	public:
		/**
		 * @param shaderKey the shader include key
		 */
		explicit HasShader(const std::string &shaderKey)
				: shaderKey_(shaderKey) { shaderState_ = ref_ptr<ShaderState>::alloc(); }

		virtual ~HasShader() = default;

		/**
		 * @param cfg the shader configuration.
		 */
		virtual void createShader(const StateConfig &cfg) { shaderState_->createShader(cfg, shaderKey_); }

		/**
		 * @return the shader state.
		 */
		const ref_ptr<ShaderState> &shaderState() const { return shaderState_; }

		/**
		 * @return the shader include key.
		 */
		const std::string &shaderKey() const { return shaderKey_; }

	protected:
		ref_ptr<ShaderState> shaderState_;
		std::string shaderKey_;
	};
} // namespace

#endif /* REGEN_SHADER_STATE_H_ */
