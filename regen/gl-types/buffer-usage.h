#ifndef REGEN_BUFFER_USAGE_H_
#define REGEN_BUFFER_USAGE_H_

#include <regen/gl-types/gl-object.h>

namespace regen {
	/**
	 * \brief Flag indicating the usage of the data in the VBO
	 */
	enum BufferUsage {
		BUFFER_USAGE_DYNAMIC_DRAW = 0,
		BUFFER_USAGE_DYNAMIC_COPY,
		BUFFER_USAGE_DYNAMIC_READ,
		BUFFER_USAGE_STATIC_DRAW,
		BUFFER_USAGE_STATIC_COPY,
		BUFFER_USAGE_STATIC_READ,
		BUFFER_USAGE_STREAM_DRAW,
		BUFFER_USAGE_STREAM_COPY,
		BUFFER_USAGE_STREAM_READ,
		BUFFER_USAGE_LAST
	};

	/**
	 * @param usage the buffer usage.
	 * @return the OpenGL buffer usage.
	 */
	GLenum glBufferUsage(BufferUsage usage);

	std::ostream &operator<<(std::ostream &out, const BufferUsage &v);

	std::istream &operator>>(std::istream &in, BufferUsage &v);
} // namespace

#endif /* REGEN_BUFFER_USAGE_H_ */
