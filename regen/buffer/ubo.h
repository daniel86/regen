#ifndef REGEN_UBO_H_
#define REGEN_UBO_H_

#include "buffer-block.h"

namespace regen {
	/**
	 * \brief Uniform Buffer Objects are a mechanism for sharing data between the CPU and the GPU.
	 *
	 * They are OpenGL Objects that allow you to store data in a buffer that can be accessed by shaders.
	 */
	class UBO : public BufferBlock {
	public:
		UBO(std::string_view name, const BufferUpdateFlags &hints);

		~UBO() override = default;

		UBO(const UBO &) = delete;

		void write(std::ostream &out) const override;
	};
} // namespace

#endif /* REGEN_UBO_H_ */
