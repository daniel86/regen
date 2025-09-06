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
		 * @param usage usage hint.
		 */
		VBO(BufferTarget target, const BufferUpdateFlags &hints, VertexLayout vertexLayout);

		~VBO() override = default;

		/**
		 * Allocate a block in the VBO memory.
		 * And copy the data from RAM to GPU.
		 * Note that as long as you keep a reference the allocated storage
		 * is marked as used.
		 */
		ref_ptr<BufferReference> &alloc(const ref_ptr<ShaderInput> &att);

		/**
		 * Allocate a block in the VBO memory.
		 * And copy the data from RAM to GPU.
		 * Note that as long as you keep a reference the allocated storage
		 * is marked as used.
		 */
		ref_ptr<BufferReference> &alloc(const std::list<ref_ptr<ShaderInput>> &attributes);

	protected:
		const VertexLayout vertexLayout_;

		ref_ptr<BufferReference> &allocInterleaved(const std::list<ref_ptr<ShaderInput> > &attributes);

		ref_ptr<BufferReference> &allocSequential(const std::list<ref_ptr<ShaderInput> > &attributes);
	};
} // namespace

#endif /* REGEN_VBO_H_ */
