#ifndef REGEN_VBO_H_
#define REGEN_VBO_H_

#include "staged-buffer.h"

namespace regen {
	/**
	 * \brief Buffer object that is used for vertex data.
	 */
	class VBO : public StagedBuffer {
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

		void write(std::ostream &out) const override;
	};
} // namespace

#endif /* REGEN_VBO_H_ */
