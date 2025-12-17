#ifndef REGEN_VBO_H_
#define REGEN_VBO_H_

#include "buffer-object.h"

namespace regen {
	/**
	 * \brief Buffer object that is used for vertex data.
	 */
	class VBO : public BufferObject {
	public:
		/**
		 * Default-Constructor.
		 */
		VBO(BufferTarget target, const BufferUpdateFlags &hints);

		~VBO() override = default;

		/**
		 * Allocate a block in the VBO memory.
		 * And copy the data from RAM to GPU.
		 * Note that as long as you keep a reference the allocated storage
		 * is marked as used.
		 */
		ref_ptr<BufferReference> &alloc(const std::vector<ref_ptr<ShaderInput>> &attributes);
	};
} // namespace

#endif /* REGEN_VBO_H_ */
