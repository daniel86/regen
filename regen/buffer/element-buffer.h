#ifndef REGEN_ELEMENT_BUFFER_H_
#define REGEN_ELEMENT_BUFFER_H_

#include "buffer-object.h"

namespace regen {
	/**
	 * \brief Buffer object that is used for element/index data.
	 */
	class ElementBuffer : public BufferObject {
	public:
		/**
		 * Default-Constructor.
		 * @param hints usage hint.
		 */
		explicit ElementBuffer(const BufferUpdateFlags &hints);

		~ElementBuffer() override = default;

		/**
		 * @return indexes to the vertex data of this primitive set.
		 */
		const ref_ptr<ShaderInput> &elements() const { return elements_; }

		/**
		 * Allocate a block in the element buffer memory.
		 * Note that as long as you keep a reference the allocated storage
		 * is marked as used.
		 */
		ref_ptr<BufferReference> &alloc(const ref_ptr<ShaderInput> &att);

	protected:
		ref_ptr<BufferReference> elementRef_;
		ref_ptr<ShaderInput> elements_;
	};
} // namespace

#endif /* REGEN_ELEMENT_BUFFER_H_ */
