#ifndef REGEN_BUFFER_USAGE_H_
#define REGEN_BUFFER_USAGE_H_

#include <regen/gl-types/gl-object.h>

namespace regen {
	/**
	 * \brief Buffer update hints.
	 *
	 * Defines how the buffer data will be updated.
	 * This is used to optimize buffer usage and memory allocation.
	 */
	enum BufferUpdateHint {
		// Indicates that the buffer data will not change dynamically.
		// Meaning that the client data is uploaded once, or that the buffer
		// is populated on the GPU side once and then re-used.
		// Should not be used in combination with BUFFER_CPU_MAPPABLE_*.
		BUFFER_HINT_STATIC = 0,
		// Indicates that the buffer data will change dynamically, but not frequently.
		// The updates are either CPU-to-GPU or GPU-to-CPU, but not both.
		BUFFER_HINT_UPDATE_RARELY,
		// Indicates that the buffer data will change dynamically, and frequently.
		// The updates are either CPU-to-GPU or GPU-to-CPU, but not both.
		BUFFER_HINT_UPDATE_STREAM
	};

	/**
	 * \brief Buffer access modes.
	 *
	 * Defines how the CPU can access the buffer.
	 * The GPU can always read/write the buffer.
	 */
	enum BufferAccessMode {
		// CPU cannot access the buffer, only GPU can read/write.
		BUFFER_GPU_ONLY = 0,
		// CPU can read from the buffer, but not write to it.
		BUFFER_CPU_READ,
		// CPU can write to the buffer, but not read from it.
		BUFFER_CPU_WRITE,
		// CPU can read and write the buffer.
		BUFFER_CPU_READ_WRITE,
		BUFFER_ACCESS_LAST // must be last
	};

	/**
	 * \brief Buffer mapping modes.
	 *
	 * Defines how the buffer can be mapped to CPU memory.
	 * Mapping is only possible if the buffer is CPU readable or writable (see @BufferAccessMode).
	 */
	enum BufferMapMode {
		// Default: no mapping is allowed.
		// i.e. CPU cannot read the buffer, or write to it via mapping (copy may still be ok, @see BufferAccessMode).
		BUFFER_MAP_DISABLED = 0,
		// Allows mapping the buffer for temporary access, i.e. direct unmap after use.
		BUFFER_MAP_TEMPORARY,
		// Allows persistent mapping of the buffer.
		// `PERSISTENT_BIT | COHERENT_BIT`
		BUFFER_MAP_PERSISTENT_COHERENT,
		// Allows persistent mapping of the buffer.
		// `PERSISTENT_BIT | FLUSH_EXPLICIT_BIT`
		BUFFER_MAP_PERSISTENT_FLUSH,
		BUFFER_MAP_LAST // must be last
	};

	/**
	 * \brief Check if the buffer map mode is persistent.
	 *
	 * @param mapMode the buffer map mode to check.
	 * @return true if the map mode is persistent, false otherwise.
	 */
	bool isMapModePersistent(BufferMapMode mapMode);

	/**
	 * \brief Buffer storage modes.
	 *
	 * Defines how the buffer is stored in memory.
	 * This is a combination of access mode and map mode for internal use.
	 */
	enum BufferStorageMode {
		BUFFER_MODE_GPU_ONLY = 0,
		BUFFER_MODE_CPU_R_MAP_TEMPORARY,
		BUFFER_MODE_CPU_R_MAP_PERSISTENT_COHERENT,
		BUFFER_MODE_CPU_R_MAP_PERSISTENT_FLUSH,
		BUFFER_MODE_CPU_W_MAP_DISABLED,
		BUFFER_MODE_CPU_W_MAP_TEMPORARY,
		BUFFER_MODE_CPU_W_MAP_PERSISTENT_COHERENT,
		BUFFER_MODE_CPU_W_MAP_PERSISTENT_FLUSH,
		BUFFER_MODE_CPU_RW_MAP_TEMPORARY,
		BUFFER_MODE_CPU_RW_MAP_PERSISTENT_COHERENT,
		BUFFER_MODE_CPU_RW_MAP_PERSISTENT_FLUSH,
		BUFFER_STORAGE_MODE_LAST // must be last
	};

	/**
	 * \brief Buffer configuration structure.
	 *
	 * This structure is used to configure buffer objects.
	 * It contains the update hint, access mode, and map mode.
	 */
	struct BufferConfig {
		BufferUpdateHint updateHint = BUFFER_HINT_STATIC;
		std::optional<BufferAccessMode> accessMode = std::nullopt;
		std::optional<BufferMapMode> mapMode = std::nullopt;

		BufferConfig() = default;

		explicit BufferConfig(BufferUpdateHint hint) : updateHint(hint) {}

		BufferConfig(BufferUpdateHint hint, BufferAccessMode access, BufferMapMode map)
			: updateHint(hint), accessMode(access), mapMode(map) {}
	};

	/**
	 * \brief Get the OpenGL buffer usage flags for a given storage mode.
	 *
	 * @param storageMode the buffer storage mode.
	 * @return OpenGL buffer usage flags.
	 */
	uint32_t glStorageFlags(BufferStorageMode storageMode);

	/**
	 * \brief Get the OpenGL access flags for a given buffer storage mode.
	 *
	 * @param storageMode the buffer storage mode.
	 * @return OpenGL access flags.
	 */
	uint32_t glAccessFlags(BufferStorageMode storageMode);


	/**
	 * \brief Get the buffer storage mode based on access mode, map mode, and update hint.
	 *
	 * @param accessMode the buffer access mode.
	 * @param mapMode the buffer map mode.
	 * @param updateHint the buffer update hint.
	 * @return the buffer storage mode.
	 */
	BufferStorageMode getBufferStorageMode(
			BufferAccessMode accessMode,
			BufferMapMode mapMode,
			BufferUpdateHint updateHint);

	std::ostream &operator<<(std::ostream &out, const BufferAccessMode &v);

	std::istream &operator>>(std::istream &in, BufferAccessMode &v);

	std::ostream &operator<<(std::ostream &out, const BufferMapMode &v);

	std::istream &operator>>(std::istream &in, BufferMapMode &v);

	std::ostream &operator<<(std::ostream &out, const BufferUpdateHint &v);

	std::istream &operator>>(std::istream &in, BufferUpdateHint &v);
} // namespace

#endif /* REGEN_BUFFER_USAGE_H_ */
