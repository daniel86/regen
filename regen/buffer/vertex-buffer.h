#ifndef REGEN_VERTEX_BUFFER_H_
#define REGEN_VERTEX_BUFFER_H_

#include "staged-buffer.h"

namespace regen {
	class VertexBuffer : public StagedBuffer {
	public:
		VertexBuffer(const std::string &name,
				const BufferUpdateFlags &hints,
				VertexLayout layoutQualifier);

		/**
		 * Copy constructor. Does not copy GPU data, both objects will share the same buffer.
		 * @param other another buffer object
		 */
		explicit VertexBuffer(
				const StagedBuffer &other,
				const std::string &name="");

		~VertexBuffer() override;
	};
} // namespace

#endif /* REGEN_VERTEX_BUFFER_H_ */
