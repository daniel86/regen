#include "buffer-enums.h"

using namespace regen;

namespace regen {
	uint32_t glStorageFlags(BufferStorageMode storageMode) {
		static constexpr uint32_t storageFlags[] = {
				// BUFFER_MODE_GPU_ONLY
				0,
				////////// CPU read modes
				// BUFFER_MODE_CPU_R_MAP_TEMPORARY
				MAP_READ,
				// BUFFER_MODE_CPU_R_MAP_PERSISTENT_COHERENT
				MAP_READ | MAP_PERSISTENT | MAP_COHERENT,
				// BUFFER_MODE_CPU_R_MAP_PERSISTENT_FLUSH
				MAP_READ | MAP_PERSISTENT,
				////////// CPU write modes
				// BUFFER_MODE_CPU_W_MAP_DISABLED
				DYNAMIC_STORAGE,
				// BUFFER_MODE_CPU_W_MAP_TEMPORARY
				DYNAMIC_STORAGE | MAP_WRITE,
				// BUFFER_MODE_CPU_W_MAP_PERSISTENT_COHERENT
				DYNAMIC_STORAGE | MAP_WRITE | MAP_PERSISTENT | MAP_COHERENT,
				// BUFFER_MODE_CPU_W_MAP_PERSISTENT_FLUSH
				DYNAMIC_STORAGE | MAP_WRITE | MAP_PERSISTENT,
				////////// CPU read/write modes
				// BUFFER_MODE_CPU_RW_MAP_TEMPORARY
				DYNAMIC_STORAGE | MAP_READ | MAP_WRITE,
				// BUFFER_MODE_CPU_RW_MAP_PERSISTENT_COHERENT
				DYNAMIC_STORAGE | MAP_READ | MAP_WRITE | MAP_PERSISTENT | MAP_COHERENT,
				// BUFFER_MODE_CPU_RW_MAP_PERSISTENT_FLUSH
				DYNAMIC_STORAGE | MAP_READ | MAP_WRITE | MAP_PERSISTENT,
				// BUFFER_STORAGE_MODE_LAST
				0
		};
		return storageFlags[(int) storageMode];
	}

	uint32_t glAccessFlags(BufferStorageMode storageMode) {
		static constexpr uint32_t storageFlags[] = {
				// BUFFER_MODE_GPU_ONLY
				0,
				////////// CPU read modes
				// BUFFER_MODE_CPU_R_MAP_TEMPORARY
				MAP_READ,
				// BUFFER_MODE_CPU_R_MAP_PERSISTENT_COHERENT
				MAP_READ | MAP_PERSISTENT | MAP_COHERENT,
				// BUFFER_MODE_CPU_R_MAP_PERSISTENT_FLUSH
				MAP_READ | MAP_PERSISTENT,
				////////// CPU write modes
				// BUFFER_MODE_CPU_W_MAP_DISABLED
				0,
				// BUFFER_MODE_CPU_W_MAP_TEMPORARY
				MAP_WRITE,
				// BUFFER_MODE_CPU_W_MAP_PERSISTENT_COHERENT
				MAP_WRITE | MAP_PERSISTENT | MAP_COHERENT,
				// BUFFER_MODE_CPU_W_MAP_PERSISTENT_FLUSH
				MAP_WRITE | MAP_PERSISTENT | MAP_FLUSH_EXPLICIT,
				////////// CPU read/write modes
				// BUFFER_MODE_CPU_RW_MAP_TEMPORARY
				MAP_READ | MAP_WRITE,
				// BUFFER_MODE_CPU_RW_MAP_PERSISTENT_COHERENT
				MAP_READ | MAP_WRITE | MAP_PERSISTENT | MAP_COHERENT,
				// BUFFER_MODE_CPU_RW_MAP_PERSISTENT_FLUSH
				MAP_READ | MAP_WRITE | MAP_PERSISTENT | MAP_FLUSH_EXPLICIT,
				// BUFFER_STORAGE_MODE_LAST
				0
		};
		return storageFlags[(int) storageMode];
	}

	BufferStorageMode getBufferStorageMode(
			ClientAccessMode accessMode,
			BufferMapMode mapMode,
			BufferUpdateFlags /*updateHints*/) {
		if (accessMode == BUFFER_GPU_ONLY) {
			if (mapMode != BUFFER_MAP_DISABLED) {
				REGEN_WARN("Buffer access mode is GPU_ONLY, but map mode is not DISABLED. "
						   "Using GPU_ONLY.");
			}
			return BUFFER_MODE_GPU_ONLY;
		} else if (accessMode == BUFFER_CPU_READ) {
			if (mapMode == BUFFER_MAP_DISABLED) {
				REGEN_WARN("Buffer access mode is CPU_READ, but map mode is DISABLED. "
						   "Using CPU_R_MAP_TEMPORARY.");
				return BUFFER_MODE_CPU_R_MAP_TEMPORARY;
			} else if (mapMode == BUFFER_MAP_TEMPORARY) {
				return BUFFER_MODE_CPU_R_MAP_TEMPORARY;
			} else if (mapMode == BUFFER_MAP_PERSISTENT_COHERENT) {
				return BUFFER_MODE_CPU_R_MAP_PERSISTENT_COHERENT;
			} else if (mapMode == BUFFER_MAP_PERSISTENT_FLUSH) {
				return BUFFER_MODE_CPU_R_MAP_PERSISTENT_FLUSH;
			}
		} else if (accessMode == BUFFER_CPU_WRITE) {
			if (mapMode == BUFFER_MAP_DISABLED) {
				return BUFFER_MODE_CPU_W_MAP_DISABLED;
			} else if (mapMode == BUFFER_MAP_TEMPORARY) {
				return BUFFER_MODE_CPU_W_MAP_TEMPORARY;
			} else if (mapMode == BUFFER_MAP_PERSISTENT_COHERENT) {
				return BUFFER_MODE_CPU_W_MAP_PERSISTENT_COHERENT;
			} else if (mapMode == BUFFER_MAP_PERSISTENT_FLUSH) {
				return BUFFER_MODE_CPU_W_MAP_PERSISTENT_FLUSH;
			}
		}
		REGEN_WARN("Unknown buffer access mode or map mode. "
				   "Using BUFFER_MODE_CPU_W_MAP_DISABLED.");
		return BUFFER_MODE_CPU_W_MAP_DISABLED;
	}

	BufferStorageMode getBufferStorageMode(const BufferFlags &flags) {
		return getBufferStorageMode(flags.accessMode, flags.mapMode, flags.updateHints);
	}

	std::ostream &operator<<(std::ostream &out, const ClientAccessMode &mode) {
		switch (mode) {
			case BUFFER_GPU_ONLY:
				return out << "GPU_ONLY";
			case BUFFER_CPU_READ:
				return out << "CPU_READ";
			case BUFFER_CPU_WRITE:
				return out << "CPU_WRITE";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, ClientAccessMode &mode) {
		std::string val;
		in >> val;
		boost::to_upper(val);
		if (val == "GPU_ONLY") mode = BUFFER_GPU_ONLY;
		else if (val == "CPU_READ") mode = BUFFER_CPU_READ;
		else if (val == "CPU_WRITE") mode = BUFFER_CPU_WRITE;
		else {
			REGEN_WARN("Unknown buffer access mode '" << val << "'. Using default GPU_ONLY.");
			mode = BUFFER_GPU_ONLY;
		}
		return in;
	}

	std::ostream &operator<<(std::ostream &out, const BufferMapMode &mode) {
		switch (mode) {
			case BUFFER_MAP_DISABLED:
				return out << "MAP_DISABLED";
			case BUFFER_MAP_TEMPORARY:
				return out << "MAP_TEMPORARY";
			case BUFFER_MAP_PERSISTENT_COHERENT:
				return out << "PERSISTENT_COHERENT";
			case BUFFER_MAP_PERSISTENT_FLUSH:
				return out << "PERSISTENT_FLUSH";
			case BUFFER_MAP_LAST:
				return out << "MAP_DISABLED"; // default case
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, BufferMapMode &mode) {
		std::string val;
		in >> val;
		boost::to_upper(val);
		if (val == "MAP_DISABLED" || val == "DISABLED") mode = BUFFER_MAP_DISABLED;
		else if (val == "MAP_TEMPORARY" || val == "TEMPORARY") mode = BUFFER_MAP_TEMPORARY;
		else if (val == "MAP_PERSISTENT_COHERENT" || val == "PERSISTENT_COHERENT")
			mode = BUFFER_MAP_PERSISTENT_COHERENT;
		else if (val == "MAP_PERSISTENT_FLUSH" || val == "PERSISTENT_FLUSH") mode = BUFFER_MAP_PERSISTENT_FLUSH;
		else {
			REGEN_WARN("Unknown buffer map mode '" << val << "'. Using default DISABLED.");
			mode = BUFFER_MAP_DISABLED;
		}
		return in;
	}

	const BufferUpdateFlags BufferUpdateFlags::NEVER = {
			.frequency = BUFFER_UPDATE_NEVER,
			.scope = BUFFER_UPDATE_FULLY
	};
	const BufferUpdateFlags BufferUpdateFlags::FULL_PER_FRAME = {
			.frequency = BUFFER_UPDATE_PER_FRAME,
			.scope = BUFFER_UPDATE_FULLY
	};
	const BufferUpdateFlags BufferUpdateFlags::FULL_PER_DRAW = {
			.frequency = BUFFER_UPDATE_PER_DRAW,
			.scope = BUFFER_UPDATE_FULLY
	};
	const BufferUpdateFlags BufferUpdateFlags::FULL_RARELY = {
			.frequency = BUFFER_UPDATE_RARE,
			.scope = BUFFER_UPDATE_FULLY
	};
	const BufferUpdateFlags BufferUpdateFlags::PARTIAL_PER_FRAME = {
			.frequency = BUFFER_UPDATE_PER_FRAME,
			.scope = BUFFER_UPDATE_PARTIALLY
	};
	const BufferUpdateFlags BufferUpdateFlags::PARTIAL_PER_DRAW = {
			.frequency = BUFFER_UPDATE_PER_DRAW,
			.scope = BUFFER_UPDATE_PARTIALLY
	};
	const BufferUpdateFlags BufferUpdateFlags::PARTIAL_RARELY = {
			.frequency = BUFFER_UPDATE_RARE,
			.scope = BUFFER_UPDATE_PARTIALLY
	};

	std::ostream &operator<<(std::ostream &out, const BufferUpdateFrequency &hint) {
		switch (hint) {
			case BUFFER_UPDATE_NEVER:
				return out << "NEVER";
			case BUFFER_UPDATE_RARE:
				return out << "RARE";
			case BUFFER_UPDATE_PER_FRAME:
				return out << "PER_FRAME";
			case BUFFER_UPDATE_PER_DRAW:
				return out << "PER_DRAW";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, BufferUpdateFrequency &hint) {
		std::string val;
		in >> val;
		boost::to_upper(val);
		if (val == "STATIC" || val == "NEVER") hint = BUFFER_UPDATE_NEVER;
		else if (val == "RARE" || val == "RARELY") { hint = BUFFER_UPDATE_RARE; }
		else if (val == "PER_FRAME") { hint = BUFFER_UPDATE_PER_FRAME; }
		else if (val == "PER_DRAW") { hint = BUFFER_UPDATE_PER_DRAW; }
		else {
			REGEN_WARN("Unknown buffer update frequency '" << val << "'. Using default NEVER.");
			hint = BUFFER_UPDATE_NEVER;
		}
		return in;
	}

	std::ostream &operator<<(std::ostream &out, const BufferUpdateScope &hint) {
		switch (hint) {
			case BUFFER_UPDATE_FULLY:
				return out << "FULL";
			case BUFFER_UPDATE_PARTIALLY:
				return out << "PARTIAL";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, BufferUpdateScope &hint) {
		std::string val;
		in >> val;
		boost::to_upper(val);
		if (val == "FULL" || val == "FULLY") hint = BUFFER_UPDATE_FULLY;
		else if (val == "PARTIAL" || val == "PARTIALLY") { hint = BUFFER_UPDATE_PARTIALLY; }
		else {
			REGEN_WARN("Unknown buffer update scope '" << val << "'. Using default FULLY.");
			hint = BUFFER_UPDATE_FULLY;
		}
		return in;
	}
}


namespace regen {
	GLenum glBufferTarget(BufferTarget target) {
		switch (target) {
			case ARRAY_BUFFER:
				return GL_ARRAY_BUFFER;
			case ELEMENT_ARRAY_BUFFER:
				return GL_ELEMENT_ARRAY_BUFFER;
			case PIXEL_PACK_BUFFER:
				return GL_PIXEL_PACK_BUFFER;
			case PIXEL_UNPACK_BUFFER:
				return GL_PIXEL_UNPACK_BUFFER;
			case UNIFORM_BUFFER:
				return GL_UNIFORM_BUFFER;
			case TEXTURE_BUFFER:
				return GL_TEXTURE_BUFFER;
			case SHADER_STORAGE_BUFFER:
				return GL_SHADER_STORAGE_BUFFER;
			case TRANSFORM_FEEDBACK_BUFFER:
				return GL_TRANSFORM_FEEDBACK_BUFFER;
			case COPY_READ_BUFFER:
				return GL_COPY_READ_BUFFER;
			case COPY_WRITE_BUFFER:
				return GL_COPY_WRITE_BUFFER;
			case DRAW_INDIRECT_BUFFER:
				return GL_DRAW_INDIRECT_BUFFER;
			case ATOMIC_COUNTER_BUFFER:
				return GL_ATOMIC_COUNTER_BUFFER;
			case DISPATCH_INDIRECT_BUFFER:
				return GL_DISPATCH_INDIRECT_BUFFER;
			case TARGET_LAST:
				break;
		}
		return GL_ARRAY_BUFFER;
	}

	std::ostream &operator<<(std::ostream &out, const BufferTarget &mode) {
		switch (mode) {
			case TARGET_LAST:
			case ARRAY_BUFFER:
				return out << "ARRAY_BUFFER";
			case ELEMENT_ARRAY_BUFFER:
				return out << "ELEMENT_ARRAY_BUFFER";
			case PIXEL_PACK_BUFFER:
				return out << "PIXEL_PACK_BUFFER";
			case PIXEL_UNPACK_BUFFER:
				return out << "PIXEL_UNPACK_BUFFER";
			case UNIFORM_BUFFER:
				return out << "UBO";
			case TEXTURE_BUFFER:
				return out << "TBO";
			case SHADER_STORAGE_BUFFER:
				return out << "SSBO";
			case TRANSFORM_FEEDBACK_BUFFER:
				return out << "TRANSFORM_FEEDBACK_BUFFER";
			case COPY_READ_BUFFER:
				return out << "COPY_READ_BUFFER";
			case COPY_WRITE_BUFFER:
				return out << "COPY_WRITE_BUFFER";
			case DRAW_INDIRECT_BUFFER:
				return out << "DRAW_INDIRECT_BUFFER";
			case ATOMIC_COUNTER_BUFFER:
				return out << "ATOMIC_COUNTER_BUFFER";
			case DISPATCH_INDIRECT_BUFFER:
				return out << "DISPATCH_INDIRECT_BUFFER";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, BufferTarget &mode) {
		std::string val;
		in >> val;
		boost::to_upper(val);
		if (val == "ARRAY_BUFFER") mode = ARRAY_BUFFER;
		else if (val == "ELEMENT_ARRAY_BUFFER") mode = ELEMENT_ARRAY_BUFFER;
		else if (val == "PIXEL_PACK_BUFFER") mode = PIXEL_PACK_BUFFER;
		else if (val == "PIXEL_UNPACK_BUFFER") mode = PIXEL_UNPACK_BUFFER;
		else if (val == "UBO" || val == "UNIFORM_BUFFER") mode = UNIFORM_BUFFER;
		else if (val == "TBO" || val == "TEXTURE_BUFFER") mode = TEXTURE_BUFFER;
		else if (val == "SSBO" || val == "SHADER_STORAGE_BUFFER") mode = SHADER_STORAGE_BUFFER;
		else if (val == "TRANSFORM_FEEDBACK_BUFFER") mode = TRANSFORM_FEEDBACK_BUFFER;
		else if (val == "COPY_READ_BUFFER") mode = COPY_READ_BUFFER;
		else if (val == "COPY_WRITE_BUFFER") mode = COPY_WRITE_BUFFER;
		else if (val == "DRAW_INDIRECT_BUFFER") mode = DRAW_INDIRECT_BUFFER;
		else if (val == "ATOMIC_COUNTER_BUFFER") mode = ATOMIC_COUNTER_BUFFER;
		else if (val == "DISPATCH_INDIRECT_BUFFER") mode = DISPATCH_INDIRECT_BUFFER;
		else {
			REGEN_WARN("Unknown VBO target '" << val << "'. Using default ARRAY_BUFFER.");
			mode = ARRAY_BUFFER;
		}
		return in;
	}

	std::ostream &operator<<(std::ostream &out, const BufferFlags &v) {
		out << (v.useExplicitStaging() ? "explicit" : "implicit") << " ";
		out << v.bufferingMode << " ";
		out << v.mapMode << " ";
		out << REGEN_STRING(v.updateHints.scope << "+" << v.updateHints.frequency) << " ";
		out << v.accessMode << " ";
		out << v.target;
		return out;
	}
}

std::ostream &regen::operator<<(std::ostream &out, const BufferMemoryLayout &v) {
	switch (v) {
		case BUFFER_MEMORY_STD140:
			out << "std140";
			break;
		case BUFFER_MEMORY_STD430:
			out << "std430";
			break;
		case BUFFER_MEMORY_PACKED:
			out << "packed";
			break;
		case BUFFER_MEMORY_SHARED:
			out << "shared";
			break;
	}
	return out;
}

std::istream &regen::operator>>(std::istream &in, BufferMemoryLayout &v) {
	std::string val;
	in >> val;
	boost::to_lower(val);
	if (val == "std140") v = BUFFER_MEMORY_STD140;
	else if (val == "std430") v = BUFFER_MEMORY_STD430;
	else if (val == "packed") v = BUFFER_MEMORY_PACKED;
	else if (val == "shared") v = BUFFER_MEMORY_SHARED;
	else {
		REGEN_WARN("Unknown memory layout '" << val << "'. Using STD140.");
		v = BUFFER_MEMORY_STD140;
	}
	return in;
}

std::ostream &regen::operator<<(std::ostream &out, const BufferingMode &v) {
	switch (v) {
		case RING_BUFFER:
			return out << "RING_BUFFER";
		case SINGLE_BUFFER:
			return out << "SINGLE_BUFFER";
		case DOUBLE_BUFFER:
			return out << "DOUBLE_BUFFER";
		case TRIPLE_BUFFER:
			return out << "TRIPLE_BUFFER";
	}
	return out;
}

std::istream &regen::operator>>(std::istream &in, BufferingMode &v) {
	std::string val;
	in >> val;
	boost::to_upper(val);
	if (val == "SINGLE_BUFFER") v = SINGLE_BUFFER;
	else if (val == "DOUBLE_BUFFER") v = DOUBLE_BUFFER;
	else if (val == "TRIPLE_BUFFER") v = TRIPLE_BUFFER;
	else if (val == "RING_BUFFER") v = RING_BUFFER;
	else {
		REGEN_WARN("Unknown memory layout '" << val << "'. Using SINGLE_BUFFER.");
		v = SINGLE_BUFFER;
	}
	return in;
}

std::ostream &regen::operator<<(std::ostream &out, const BufferSizeClass &v) {
	switch (v) {
		case BUFFER_SIZE_SMALL:
			return out << "SMALL";
		case BUFFER_SIZE_MEDIUM:
			return out << "MEDIUM";
		case BUFFER_SIZE_LARGE:
			return out << "LARGE";
		case BUFFER_SIZE_VERY_LARGE:
			return out << "HUGE";
	}
	return out;
}

std::istream &regen::operator>>(std::istream &in, BufferSizeClass &v) {
	std::string val;
	in >> val;
	boost::to_upper(val);
	if (val == "SMALL") v = BUFFER_SIZE_SMALL;
	else if (val == "MEDIUM") v = BUFFER_SIZE_MEDIUM;
	else if (val == "LARGE") v = BUFFER_SIZE_LARGE;
	else if (val == "HUGE" || val == "VERY_LARGE") v = BUFFER_SIZE_VERY_LARGE;
	else {
		REGEN_WARN("Unknown buffer size class '" << val << "'. Using SMALL.");
		v = BUFFER_SIZE_SMALL;
	}
	return in;
}
