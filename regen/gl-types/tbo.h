#ifndef REGEN_TBO_H_
#define REGEN_TBO_H_

#include <regen/gl-types/buffer-object.h>

namespace regen {
	/**
	 * \brief A buffer object that can be bound to a texture.
	 */
	class TBO : public BufferObject {
	public:
		explicit TBO(BufferUsage usage);

		void setBufferData(const ref_ptr<ShaderInput> &input);

		auto &input() const { return input_; }

	protected:
		ref_ptr<ShaderInput> input_;
	};
} // namespace

#endif /* REGEN_TBO_H_ */
