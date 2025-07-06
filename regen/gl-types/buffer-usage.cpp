#include "buffer-usage.h"

using namespace regen;

namespace regen {
	uint32_t glBufferUsage(BufferStorageMode storageMode) {
		static constexpr uint32_t storageFlags[] = {
				// BUFFER_MODE_GPU_ONLY
				0,
				// BUFFER_MODE_CPU_R_MAP_TEMPORARY
				GL_MAP_READ_BIT,
				// BUFFER_MODE_CPU_R_MAP_PERSISTENT_COHERENT
				GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT,
				// BUFFER_MODE_CPU_R_MAP_PERSISTENT_FLUSH
				GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT,
				// BUFFER_MODE_CPU_W_MAP_DISABLED
				GL_DYNAMIC_STORAGE_BIT,
				// BUFFER_MODE_CPU_W_MAP_TEMPORARY
				GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT,
				// BUFFER_MODE_CPU_W_MAP_PERSISTENT_COHERENT
				GL_DYNAMIC_STORAGE_BIT
				| GL_MAP_WRITE_BIT
				| GL_MAP_PERSISTENT_BIT
				| GL_MAP_COHERENT_BIT,
				// BUFFER_MODE_CPU_W_MAP_PERSISTENT_FLUSH
				GL_DYNAMIC_STORAGE_BIT
				| GL_MAP_WRITE_BIT
				| GL_MAP_PERSISTENT_BIT
				| GL_MAP_FLUSH_EXPLICIT_BIT,
				// BUFFER_MODE_CPU_RW_MAP_TEMPORARY
				GL_DYNAMIC_STORAGE_BIT
				| GL_MAP_READ_BIT
				| GL_MAP_WRITE_BIT,
				// BUFFER_MODE_CPU_RW_MAP_PERSISTENT_COHERENT
				GL_DYNAMIC_STORAGE_BIT
				| GL_MAP_READ_BIT
				| GL_MAP_WRITE_BIT
				| GL_MAP_PERSISTENT_BIT
				| GL_MAP_COHERENT_BIT,
				// BUFFER_MODE_CPU_RW_MAP_PERSISTENT_FLUSH
				GL_DYNAMIC_STORAGE_BIT
				| GL_MAP_READ_BIT
				| GL_MAP_WRITE_BIT
				| GL_MAP_PERSISTENT_BIT
				| GL_MAP_FLUSH_EXPLICIT_BIT,
				// BUFFER_STORAGE_MODE_LAST
				0
		};
		return storageFlags[(int) storageMode];
	}

	bool isMapModePersistent(BufferMapMode mapMode) {
		return mapMode == BUFFER_MAP_PERSISTENT_COHERENT || mapMode == BUFFER_MAP_PERSISTENT_FLUSH;
	}

	BufferStorageMode getBufferStorageMode(
			BufferAccessMode accessMode,
			BufferMapMode mapMode,
			BufferUpdateHint /*updateHint*/) {
		if (accessMode == BUFFER_GPU_ONLY) {
			if (mapMode != BUFFER_MAP_DISABLED) {
				REGEN_WARN("Buffer access mode is GPU_ONLY, but map mode is not DISABLED. Using GPU_ONLY.");
			}
			return BUFFER_MODE_GPU_ONLY;
		} else if (accessMode == BUFFER_CPU_READ) {
			if (mapMode == BUFFER_MAP_DISABLED) {
				REGEN_WARN("Buffer access mode is CPU_READ, but map mode is DISABLED. Using CPU_R_MAP_TEMPORARY.");
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
		} else if (accessMode == BUFFER_CPU_READ_WRITE) {
			if (mapMode == BUFFER_MAP_DISABLED) {
				REGEN_WARN(
						"Buffer access mode is CPU_READ_WRITE, but map mode is DISABLED. Using CPU_RW_MAP_TEMPORARY.");
				return BUFFER_MODE_CPU_RW_MAP_TEMPORARY;
			} else if (mapMode == BUFFER_MAP_TEMPORARY) {
				return BUFFER_MODE_CPU_RW_MAP_TEMPORARY;
			} else if (mapMode == BUFFER_MAP_PERSISTENT_COHERENT) {
				return BUFFER_MODE_CPU_RW_MAP_PERSISTENT_COHERENT;
			} else if (mapMode == BUFFER_MAP_PERSISTENT_FLUSH) {
				return BUFFER_MODE_CPU_RW_MAP_PERSISTENT_FLUSH;
			}
		}
		REGEN_WARN("Unknown buffer access mode or map mode. Using CPU_RW_MAP_TEMPORARY.");
		return BUFFER_MODE_CPU_RW_MAP_TEMPORARY;
	}

	std::ostream &operator<<(std::ostream &out, const BufferAccessMode &mode) {
		switch (mode) {
			case BUFFER_GPU_ONLY:
				return out << "GPU_ONLY";
			case BUFFER_CPU_READ:
				return out << "CPU_READ";
			case BUFFER_CPU_WRITE:
				return out << "CPU_WRITE";
			case BUFFER_CPU_READ_WRITE:
				return out << "CPU_READ_WRITE";
			case BUFFER_ACCESS_LAST:
				return out << "GPU_ONLY"; // default case
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, BufferAccessMode &mode) {
		std::string val;
		in >> val;
		boost::to_upper(val);
		if (val == "GPU_ONLY") mode = BUFFER_GPU_ONLY;
		else if (val == "CPU_READ") mode = BUFFER_CPU_READ;
		else if (val == "CPU_WRITE") mode = BUFFER_CPU_WRITE;
		else if (val == "CPU_READ_WRITE") mode = BUFFER_CPU_READ_WRITE;
		else {
			REGEN_WARN("Unknown buffer access mode '" << val << "'. Using default GPU_ONLY.");
			mode = BUFFER_GPU_ONLY;
		}
		return in;
	}

	std::ostream &operator<<(std::ostream &out, const BufferMapMode &mode) {
		switch (mode) {
			case BUFFER_MAP_DISABLED:
				return out << "DISABLED";
			case BUFFER_MAP_TEMPORARY:
				return out << "TEMPORARY";
			case BUFFER_MAP_PERSISTENT_COHERENT:
				return out << "PERSISTENT_COHERENT";
			case BUFFER_MAP_PERSISTENT_FLUSH:
				return out << "PERSISTENT_FLUSH";
			case BUFFER_MAP_LAST:
				return out << "DISABLED"; // default case
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, BufferMapMode &mode) {
		std::string val;
		in >> val;
		boost::to_upper(val);
		if (val == "DISABLED") mode = BUFFER_MAP_DISABLED;
		else if (val == "TEMPORARY") mode = BUFFER_MAP_TEMPORARY;
		else if (val == "PERSISTENT_COHERENT") mode = BUFFER_MAP_PERSISTENT_COHERENT;
		else if (val == "PERSISTENT_FLUSH") mode = BUFFER_MAP_PERSISTENT_FLUSH;
		else {
			REGEN_WARN("Unknown buffer map mode '" << val << "'. Using default DISABLED.");
			mode = BUFFER_MAP_DISABLED;
		}
		return in;
	}

	std::ostream &operator<<(std::ostream &out, const BufferUpdateHint &hint) {
		switch (hint) {
			case BUFFER_HINT_STATIC:
				return out << "STATIC";
			case BUFFER_HINT_UPDATE_RARELY:
				return out << "UPDATE_RARELY";
			case BUFFER_HINT_UPDATE_STREAM:
				return out << "UPDATE_STREAM";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, BufferUpdateHint &hint) {
		std::string val;
		in >> val;
		boost::to_upper(val);
		if (val == "STATIC") hint = BUFFER_HINT_STATIC;
		else if (val == "UPDATE_RARELY") hint = BUFFER_HINT_UPDATE_RARELY;
		else if (val == "UPDATE_STREAM") hint = BUFFER_HINT_UPDATE_STREAM;
		else {
			REGEN_WARN("Unknown buffer update hint '" << val << "'. Using default STATIC.");
			hint = BUFFER_HINT_STATIC;
		}
		return in;
	}
}
