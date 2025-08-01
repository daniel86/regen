#ifndef REGEN_DRAW_INDIRECT_BUFFER_H_
#define REGEN_DRAW_INDIRECT_BUFFER_H_

#include "ssbo.h"

namespace regen {
	class DrawIndirectBuffer : public SSBO {
	public:
		DrawIndirectBuffer(const std::string &name, const BufferUpdateFlags &hints);
	};
} // namespace

#endif /* REGEN_DRAW_INDIRECT_BUFFER_H_ */
