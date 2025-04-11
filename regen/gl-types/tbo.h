#ifndef REGEN_TBO_H_
#define REGEN_TBO_H_

#include <regen/gl-types/buffer-object.h>
#include "regen/textures/texture-buffer.h"

namespace regen {
	/**
	 * \brief A buffer object that can be bound to a texture.
	 */
	class TBO : public BufferObject {
	public:
		/**
		 * \brief Creates a TBO.
		 *
		 * @param usage the buffer usage.
		 */
		explicit TBO(BufferUsage usage);

		/**
		 * Attach a client data pointer to the TBO.
		 * This will also create a texture, and attach it to the TBO.
		 * @param input the shader input to attach.
		 */
		void setBufferInput(const ref_ptr<regen::ShaderInput> &input);

		/**
		 * Update the TBO with the current data.
		 */
		void updateTBO();

		/**
		 * @return the input data.
		 */
		auto &input() const { return input_; }

		/**
		 * @return the TBO reference.
		 */
		ref_ptr<BufferReference> &tboRef();

		/**
		 * @return the TBO texture.
		 */
		auto &tboTexture() const { return tboTexture_; }

	protected:
		ref_ptr<ShaderInput> input_;
		ref_ptr<BufferReference> tboRef_;
		ref_ptr<TextureBuffer> tboTexture_;
		unsigned int lastStamp_ = 0u;

		void resizeTBO();
	};
} // namespace

#endif /* REGEN_TBO_H_ */
