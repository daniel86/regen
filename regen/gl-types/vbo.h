#ifndef REGEN_VBO_H_
#define REGEN_VBO_H_

#include <regen/gl-types/buffer-object.h>

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
		explicit VBO(
				BufferTarget target = ARRAY_BUFFER,
				BufferUsage usage = BUFFER_USAGE_DYNAMIC_DRAW);

		~VBO() override = default;

		/**
		 * Allocate a block in the VBO memory.
		 * And copy the data from RAM to GPU.
		 * Note that as long as you keep a reference the allocated storage
		 * is marked as used.
		 */
		ref_ptr<BufferReference> &alloc(const ref_ptr<ShaderInput> &att);

		/**
		 * Allocate GPU memory for the given attributes.
		 * And copy the data from RAM to GPU.
		 * Note that as long as you keep a reference the allocated storage
		 * is marked as used.
		 */
		ref_ptr<BufferReference> &allocInterleaved(const std::list<ref_ptr<ShaderInput> > &attributes);

		/**
		 * Allocate GPU memory for the given attributes.
		 * And copy the data from RAM to GPU.
		 * Note that as long as you keep a reference the allocated storage
		 * is marked as used.
		 */
		ref_ptr<BufferReference> &allocSequential(const std::list<ref_ptr<ShaderInput> > &attributes);

	protected:
		void uploadInterleaved(
				GLuint startByte,
				GLuint endByte,
				const std::list<ref_ptr<ShaderInput> > &attributes,
				ref_ptr<BufferReference> &ref);

		void uploadSequential(
				GLuint startByte,
				GLuint endByte,
				const std::list<ref_ptr<ShaderInput> > &attributes,
				ref_ptr<BufferReference> &ref);
	};
} // namespace

#endif /* REGEN_VBO_H_ */
