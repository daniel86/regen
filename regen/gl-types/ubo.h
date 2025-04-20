#ifndef REGEN_UBO_H_
#define REGEN_UBO_H_

#include <map>
#include <regen/gl-types/buffer-block.h>
#include <regen/gl-types/shader-input.h>
#include <regen/scene/scene-input.h>

namespace regen {
	/**
	 * \brief Uniform Buffer Objects are a mechanism for sharing data between the CPU and the GPU.
	 *
	 * They are OpenGL Objects that allow you to store data in a buffer that can be accessed by shaders.
	 */
	class UBO : public BufferBlock, public ShaderInput {
	public:
		explicit UBO(const std::string &name, BufferUsage usage = BUFFER_USAGE_DYNAMIC_DRAW);

		~UBO() override = default;

		UBO(const UBO &) = delete;

		void write(std::ostream &out) const override;
	};
} // namespace

#endif /* REGEN_UBO_H_ */
