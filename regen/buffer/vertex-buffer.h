#ifndef REGEN_VERTEX_BUFFER_H_
#define REGEN_VERTEX_BUFFER_H_

#include "staged-buffer.h"

namespace regen {
	class VertexBuffer : public StagedBuffer {
	public:
		VertexBuffer(std::string_view name,
				const BufferUpdateFlags &hints,
				VertexLayout layoutQualifier);

		/**
		 * Copy constructor. Does not copy GPU data, both objects will share the same buffer.
		 * @param other another buffer object
		 */
		explicit VertexBuffer(const StagedBuffer &other, std::string_view name="");

		~VertexBuffer() override;
	};
} // namespace

#endif /* REGEN_VERTEX_BUFFER_H_ */
