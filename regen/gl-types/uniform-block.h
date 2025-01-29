#ifndef REGEN_UNIFORM_BLOCK_H_
#define REGEN_UNIFORM_BLOCK_H_

#include "shader-input.h"
#include "ubo.h"

namespace regen {
	/**
	 * \brief Provides input data via a Uniform Buffer Object (UBO).
	 */
	class UniformBlock : public ShaderInput {
	public:
		/**
		 * @param name the uniform block name.
		 */
		explicit UniformBlock(const std::string &name);

		UniformBlock(const std::string &name, const ref_ptr<UBO> &ubo);

		~UniformBlock() override;

		UniformBlock(const UniformBlock &) = delete;

		/**
		 * @return the list of uniforms.
		 */
		const std::vector<NamedShaderInput> &uniforms() const;

		ref_ptr<UBO> ubo() const;

		/**
		 * @param input the uniform to add.
		 */
		void addUniform(const ref_ptr<ShaderInput> &input, const std::string &name = "");

		/**
		 * @param input the uniform to update.
		 */
		void updateUniform(const ref_ptr<ShaderInput> &input);

		/**
		 * Binds the uniform block to the given shader location.
		 */
		void enableUniformBlock(GLint loc) const;

		void write(std::ostream &out) const override;

	protected:
		struct UniformBlockData;
		UniformBlockData *priv_;
	};
} // namespace

#endif /* REGEN_UNIFORM_BLOCK_H_ */
