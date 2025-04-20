#include "buffer-usage.h"

using namespace regen;

namespace regen {
	GLenum glBufferUsage(BufferUsage usage) {
		switch (usage) {
			case BUFFER_USAGE_DYNAMIC_DRAW:
				return GL_DYNAMIC_DRAW;
			case BUFFER_USAGE_DYNAMIC_COPY:
				return GL_DYNAMIC_COPY;
			case BUFFER_USAGE_DYNAMIC_READ:
				return GL_DYNAMIC_READ;
			case BUFFER_USAGE_STATIC_DRAW:
				return GL_STATIC_DRAW;
			case BUFFER_USAGE_STATIC_COPY:
				return GL_STATIC_COPY;
			case BUFFER_USAGE_STATIC_READ:
				return GL_STATIC_READ;
			case BUFFER_USAGE_STREAM_DRAW:
				return GL_STREAM_DRAW;
			case BUFFER_USAGE_STREAM_COPY:
				return GL_STREAM_COPY;
			case BUFFER_USAGE_STREAM_READ:
				return GL_STREAM_READ;
			case BUFFER_USAGE_LAST:
				return GL_DYNAMIC_DRAW;
		}
		return GL_DYNAMIC_DRAW;
	}

	std::ostream &operator<<(std::ostream &out, const BufferUsage &mode) {
		switch (mode) {
			case BUFFER_USAGE_DYNAMIC_DRAW:
				return out << "DYNAMIC_DRAW";
			case BUFFER_USAGE_DYNAMIC_COPY:
				return out << "DYNAMIC_COPY";
			case BUFFER_USAGE_DYNAMIC_READ:
				return out << "DYNAMIC_READ";
			case BUFFER_USAGE_STATIC_DRAW:
				return out << "STATIC_DRAW";
			case BUFFER_USAGE_STATIC_COPY:
				return out << "STATIC_COPY";
			case BUFFER_USAGE_STATIC_READ:
				return out << "STATIC_READ";
			case BUFFER_USAGE_STREAM_DRAW:
				return out << "STREAM_DRAW";
			case BUFFER_USAGE_STREAM_COPY:
				return out << "STREAM_COPY";
			case BUFFER_USAGE_STREAM_READ:
				return out << "STREAM_READ";
			case BUFFER_USAGE_LAST:
				return out << "DYNAMIC_DRAW";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, BufferUsage &mode) {
		std::string val;
		in >> val;
		boost::to_upper(val);
		if (val == "DYNAMIC_DRAW") mode = BUFFER_USAGE_DYNAMIC_DRAW;
		else if (val == "DYNAMIC_COPY") mode = BUFFER_USAGE_DYNAMIC_COPY;
		else if (val == "DYNAMIC_READ") mode = BUFFER_USAGE_DYNAMIC_READ;
		else if (val == "STATIC_DRAW") mode = BUFFER_USAGE_STATIC_DRAW;
		else if (val == "STATIC_COPY") mode = BUFFER_USAGE_STATIC_COPY;
		else if (val == "STATIC_READ") mode = BUFFER_USAGE_STATIC_READ;
		else if (val == "STREAM_DRAW") mode = BUFFER_USAGE_STREAM_DRAW;
		else if (val == "STREAM_COPY") mode = BUFFER_USAGE_STREAM_COPY;
		else if (val == "STREAM_READ") mode = BUFFER_USAGE_STREAM_READ;
		else {
			REGEN_WARN("Unknown buffer usage '" << val << "'. Using default DYNAMIC_DRAW.");
			mode =BUFFER_USAGE_DYNAMIC_DRAW;
		}
		return in;
	}
}
