#ifndef REGEN_DRAW_INDIRECT_BUFFER_H_
#define REGEN_DRAW_INDIRECT_BUFFER_H_

#include "ssbo.h"

namespace regen {
	/**
	 * \brief A buffer for storing draw indirect commands.
	 * This buffer is used to store draw commands that can be executed by the GPU.
	 */
	class DrawIndirectBuffer : public SSBO {
	public:
		DrawIndirectBuffer(const std::string &name, const BufferUpdateFlags &hints);
	};
} // namespace

#endif /* REGEN_DRAW_INDIRECT_BUFFER_H_ */
