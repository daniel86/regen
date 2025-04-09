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
	};
} // namespace

#endif /* REGEN_TBO_H_ */
