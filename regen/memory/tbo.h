#ifndef REGEN_TBO_H_
#define REGEN_TBO_H_

#include "staged-buffer.h"
#include "regen/textures/texture-buffer.h"

namespace regen {
	/**
	 * \brief A buffer object that can be bound to a texture.
	 */
	class TBO : public StagedBuffer {
	public:
		/**
		 * \brief Creates a TBO.
		 *
		 * @param usage the buffer usage.
		 */
		TBO(std::string_view name, GLenum texelFormat, const BufferUpdateFlags &hints);

		/**
		 * @return the input data.
		 */
		auto &input() const { return input_; }

		/**
		 * @return the TBO texture.
		 */
		auto &tboTexture() const { return tboTexture_; }

		void write(std::ostream &out) const override;

	protected:
		ref_ptr<ShaderInput> input_;
		ref_ptr<TextureBuffer> tboTexture_;
	};
} // namespace

#endif /* REGEN_TBO_H_ */
