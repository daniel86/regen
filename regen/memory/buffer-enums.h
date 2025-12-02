#ifndef REGEN_BUFFER_ENUMS_H_
#define REGEN_BUFFER_ENUMS_H_

#include "regen/gl/gl-object.h"

namespace regen {
	/**
	 * \brief Buffer target enumeration.
	 * The buffer target defines the type of buffer object.
	 */
	enum BufferTarget {
		ARRAY_BUFFER = 0,
		ATOMIC_COUNTER_BUFFER,
		COPY_READ_BUFFER,
		COPY_WRITE_BUFFER,
		DISPATCH_INDIRECT_BUFFER,
		DRAW_INDIRECT_BUFFER,
		ELEMENT_ARRAY_BUFFER,
		PIXEL_PACK_BUFFER,
		PIXEL_UNPACK_BUFFER,
		SHADER_STORAGE_BUFFER,
		TEXTURE_BUFFER,
		TRANSFORM_FEEDBACK_BUFFER,
		UNIFORM_BUFFER,
		TARGET_LAST // must be last
	};

	/**
	 * @param target the buffer target.
	 * @return the OpenGL buffer target.
	 */
	GLenum glBufferTarget(BufferTarget target);

	/**
	 * Memory layout for shader storage blocks.
	 * Defines how the data is laid out in memory.
	 */
	enum BufferMemoryLayout {
		BUFFER_MEMORY_STD140 = 0,
		BUFFER_MEMORY_STD430,
		BUFFER_MEMORY_PACKED,
		BUFFER_MEMORY_SHARED,
		BUFFER_MEMORY_INTERLEAVED
	};

	/**
	 * The vertex layout, i.e. how vertex attributes are arranged in the buffer.
	 */
	enum VertexLayout {
		VERTEX_LAYOUT_INTERLEAVED = 0,
		VERTEX_LAYOUT_SEQUENTIAL = 1
	};

	/**
	  * The buffering mode, i.e. how many buffers are used
	  * (usually by the staging system).
	  */
	enum BufferingMode {
		RING_BUFFER = 0,
		SINGLE_BUFFER = 1,
		DOUBLE_BUFFER = 2,
		TRIPLE_BUFFER = 3
	};

	/**
	 * \brief Buffer size classes.
	 *
	 * A rough classification of buffer sizes intended to optimize memory allocation.
	 */
	enum BufferSizeClass {
		BUFFER_SIZE_SMALL = 0, // keep small to large order
		BUFFER_SIZE_MEDIUM,
		BUFFER_SIZE_LARGE,
		BUFFER_SIZE_VERY_LARGE,
	};

	/**
	 * \brief Buffer update frequency.
	 *
	 * Defines how often the buffer data is updated.
	 */
	enum BufferUpdateFrequency {
		// Indicates that the buffer data will not change dynamically.
		// Meaning that the client data is uploaded once, or that the buffer
		// is populated on the GPU side once and then re-used.
		BUFFER_UPDATE_NEVER = 0,
		// Indicates that the buffer data will change dynamically, but not frequently.
		BUFFER_UPDATE_RARE,
		// Indicates that the buffer data will change dynamically, and frequently.
		BUFFER_UPDATE_PER_FRAME,
		// Indicates that the buffer data will change dynamically, and very frequently.
		BUFFER_UPDATE_PER_DRAW,
	};

	/**
	 * \brief Buffer update scope.
	 *
	 * Defines how much of the buffer data is updated each time it is updated.
	 */
	enum BufferUpdateScope {
		// Indicates that the buffer data will change fully each time it is updated.
		BUFFER_UPDATE_FULLY = 0,
		// Indicates that the buffer data will change partially, dynamically, but not frequently.
		BUFFER_UPDATE_PARTIALLY,
	};

	/**
	 * \brief Buffer update hints.
	 *
	 * Defines how the buffer is updated.
	 */
	struct BufferUpdateFlags {
		static const BufferUpdateFlags NEVER;
		static const BufferUpdateFlags FULL_PER_FRAME;
		static const BufferUpdateFlags FULL_PER_DRAW;
		static const BufferUpdateFlags FULL_RARELY;
		static const BufferUpdateFlags PARTIAL_PER_FRAME;
		static const BufferUpdateFlags PARTIAL_PER_DRAW;
		static const BufferUpdateFlags PARTIAL_RARELY;

		// how often the buffer is updated
		BufferUpdateFrequency frequency = BUFFER_UPDATE_NEVER;
		// how much of the buffer is updated each time it is updated
		BufferUpdateScope scope = BUFFER_UPDATE_FULLY;
	};

	/**
	 * \brief Client access modes.
	 *
	 * Defines how the CPU can access the buffer.
	 * This is mainly used for client-side data management,
	 * i.e. how CPU accesses the client and staging buffer.
	 */
	enum ClientAccessMode {
		// CPU cannot access the buffer, only GPU can read/write.
		BUFFER_GPU_ONLY = 0,
		// CPU can read from the buffer, but not write to it.
		BUFFER_CPU_READ = 1 << 0,
		// CPU can write to the buffer, but not read from it.
		BUFFER_CPU_WRITE = 1 << 1
	};

	/**
	 * \brief Server access modes.
	 *
	 * Defines how the GPU can access the buffer.
	 * This is mainly used for mechanisms that decide adaptively how data
	 * is stored on the GPU, e.g. SSBO allows writing while UBO does not.
	 */
	enum ServerAccessMode {
		BUFFER_GPU_READ = 1 << 0,
		BUFFER_GPU_WRITE = 1 << 1
	};

	/**
	 * \brief Buffer synchronization flags.
	 *
	 * Defines how the staging buffer is synchronized with the GPU.
	 * This is used to optimize buffer usage and memory allocation.
	 */
	enum BufferSyncFlag {
		BUFFER_SYNC_NOTHING_SPECIAL = 0,
		// Do NOT use explicit staging buffer, i.e. write directly to the draw buffer.
		// BEWARE: This may introduce synchronization delays in some cases!
		// In such a case maybe more segments in the staging ring buffer will help.
		BUFFER_SYNC_IMPLICIT_STAGING = 1 << 0,
		// Allow frame dropping when the GPU is still using the buffer and CPU
		// tries to read/write to it.
		// e.g. in case of reading this means that the read data might not be up to date
		// after read finished!
		BUFFER_SYNC_FRAME_DROPPING = 1 << 1,
		// Do not use fences for protecting read/write operations.
		BUFFER_SYNC_DISABLE_FENCING = 1 << 2,
	};

	/**
	 * \brief The mapping flags.
	 *
	 * Defines how the buffer can be mapped to CPU memory.
	 * These flags are used in the glMapBufferRange and glMapNamedBufferRange functions.
	 */
	enum MappingFlag {
		DYNAMIC_STORAGE = GL_DYNAMIC_STORAGE_BIT,
		MAP_PERSISTENT = GL_MAP_PERSISTENT_BIT,
		MAP_COHERENT = GL_MAP_COHERENT_BIT,
		MAP_READ = GL_MAP_READ_BIT,
		MAP_WRITE = GL_MAP_WRITE_BIT,
		MAP_FLUSH_EXPLICIT = GL_MAP_FLUSH_EXPLICIT_BIT,
		MAP_INVALIDATE_RANGE = GL_MAP_INVALIDATE_RANGE_BIT,
		MAP_INVALIDATE_BUFFER = GL_MAP_INVALIDATE_BUFFER_BIT,
		MAP_UNSYNCHRONIZED = GL_MAP_UNSYNCHRONIZED_BIT,
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
	inline bool isMapModePersistent(BufferMapMode mapMode) {
		return mapMode >= BUFFER_MAP_PERSISTENT_COHERENT;
	}

	/**
	 * \brief Buffer storage modes.
	 *
	 * Defines how the buffer is stored in memory.
	 * This is a combination of access mode and map mode.
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
		BUFFER_STORAGE_MODE_LAST // must be last
	};

	// only use GL_WRITE_BIT for static write buffers.
	const BufferStorageMode BUFFER_MODE_STATIC_WRITE = BUFFER_MODE_CPU_W_MAP_TEMPORARY;
	const BufferStorageMode BUFFER_MODE_STATIC_READ = BUFFER_MODE_CPU_R_MAP_TEMPORARY;

	/**
	 * \brief Buffer configuration structure.
	 *
	 * This structure is used to configure buffer objects.
	 * It contains the update hint, access mode, and map mode.
	 */
	struct BufferFlags {
		BufferTarget target;
		BufferUpdateFlags updateHints;
		ClientAccessMode accessMode = BUFFER_GPU_ONLY;
		BufferMapMode mapMode = BUFFER_MAP_DISABLED;
		BufferingMode bufferingMode = SINGLE_BUFFER;
		uint32_t syncFlags = 0;

		explicit BufferFlags(BufferTarget target)
				: target(target),
				  updateHints(BufferUpdateFlags::NEVER) {}

		BufferFlags(BufferTarget target, const BufferUpdateFlags &hints)
				: target(target), updateHints(hints) {}

		bool areUpdatesVeryFrequent() const {
			return updateHints.frequency == BUFFER_UPDATE_PER_DRAW;
		}

		bool areUpdatesPerDraw() const {
			return updateHints.frequency == BUFFER_UPDATE_PER_DRAW;
		}

		bool areUpdatesFrequent() const {
			return updateHints.frequency == BUFFER_UPDATE_PER_FRAME;
		}

		bool areUpdatesPerFrame() const {
			return updateHints.frequency == BUFFER_UPDATE_PER_FRAME;
		}

		bool areUpdatesRare() const {
			return updateHints.frequency == BUFFER_UPDATE_RARE;
		}

		bool areUpdatesPartial() const {
			return updateHints.scope == BUFFER_UPDATE_PARTIALLY;
		}

		bool useExplicitFlushing() const {
			return mapMode == BUFFER_MAP_PERSISTENT_FLUSH;
		}

		bool useExplicitStaging() const {
			return (syncFlags & BUFFER_SYNC_IMPLICIT_STAGING) == 0;
		}

		bool useImplicitStaging() const {
			return (syncFlags & BUFFER_SYNC_IMPLICIT_STAGING) != 0;
		}

		bool useFrameDropping() const {
			return (syncFlags & BUFFER_SYNC_FRAME_DROPPING) != 0;
		}

		bool useSyncFences() const {
			return (syncFlags & BUFFER_SYNC_DISABLE_FENCING) == 0;
		}

		bool isMappable() const {
			return mapMode != BUFFER_MAP_DISABLED && accessMode != BUFFER_GPU_ONLY;
		}

		bool isReadable() const {
			return accessMode == BUFFER_CPU_READ;
		}

		bool isWritable() const {
			return accessMode == BUFFER_CPU_WRITE;
		}
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
			ClientAccessMode accessMode,
			BufferMapMode mapMode,
			BufferUpdateFlags updateHints);

	BufferStorageMode getBufferStorageMode(const BufferFlags &flags);

	std::ostream &operator<<(std::ostream &out, const ClientAccessMode &v);

	std::istream &operator>>(std::istream &in, ClientAccessMode &v);

	std::ostream &operator<<(std::ostream &out, const BufferMapMode &v);

	std::istream &operator>>(std::istream &in, BufferMapMode &v);

	std::ostream &operator<<(std::ostream &out, const BufferUpdateFrequency &v);

	std::istream &operator>>(std::istream &in, BufferUpdateFrequency &v);

	std::ostream &operator<<(std::ostream &out, const BufferUpdateScope &v);

	std::istream &operator>>(std::istream &in, BufferUpdateScope &v);

	std::ostream &operator<<(std::ostream &out, const BufferFlags &v);

	std::ostream &operator<<(std::ostream &out, const BufferMemoryLayout &v);

	std::istream &operator>>(std::istream &in, BufferMemoryLayout &v);

	std::ostream &operator<<(std::ostream &out, const BufferingMode &v);

	std::istream &operator>>(std::istream &in, BufferingMode &v);

	std::ostream &operator<<(std::ostream &out, const BufferSizeClass &v);

	std::istream &operator>>(std::istream &in, BufferSizeClass &v);

	std::ostream &operator<<(std::ostream &out, const BufferTarget &v);

	std::istream &operator>>(std::istream &in, BufferTarget &v);
} // namespace

#endif /* REGEN_BUFFER_ENUMS_H_ */
