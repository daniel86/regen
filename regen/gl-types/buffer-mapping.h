#ifndef REGEN_BUFFER_MAPPING_H_
#define REGEN_BUFFER_MAPPING_H_

#include <regen/gl-types/gl-object.h>
#include <regen/gl-types/buffer-reference.h>
#include <regen/gl-types/shader-input.h>

namespace regen {
	/**
	 * \brief A utility class for mapping buffer objects for reading or writing.
	 *
	 * To avoid synchronization issues, an additional buffer is used that can
	 * be single, double or triple buffered.
	 * If persistent mapping is used, the mapped data will be available by the
	 * pointers provided by this class.
	 */
	class BufferMapping : public GLObject {
	public:
		/**
		 * The buffering mode, i.e. how many buffers are used.
		 */
		enum Buffering {
			SINGLE_BUFFER = 1,
			DOUBLE_BUFFER = 2,
			TRIPLE_BUFFER = 3
		};
		/**
		 * The mapping flags.
		 */
		enum Flag {
			DYNAMIC = GL_DYNAMIC_STORAGE_BIT,
			PERSISTENT = GL_MAP_PERSISTENT_BIT,
			COHERENT = GL_MAP_COHERENT_BIT,
			READ = GL_MAP_READ_BIT,
			WRITE = GL_MAP_WRITE_BIT,
		};

		/**
		 * The mapping flags.
		 */
		explicit BufferMapping(uint32_t storageFlags,
			Buffering storageBuffering = Buffering::DOUBLE_BUFFER);

		~BufferMapping() override;

		/**
		 * It will take 1-3 frames until the data is available.
		 * @return true if the client data was loaded from storage.
		 */
		bool hasData() const { return hasData_[writeBufferIndex_]; }

		/**
		 * Initializes fixed-sized buffers for the mapping.
		 * @param numBytes the number of bytes to allocate for each buffer.
		 */
		void initializeMapping(GLuint numBytes);

		/**
		 * Update the mapping with the data from the input reference.
		 * @param inputReference the input reference to copy data from.
		 * @param inputTarget the target of the input reference.
		 */
		void updateMapping(const ref_ptr<BufferReference> &inputReference, GLenum inputTarget);

		/**
		 * @return the next buffer to read from.
		 */
		uint32_t nextReadBuffer() const { return ids_[readBufferIndex_]; }

		/**
		 * @return the next buffer to write to.
		 */
		uint32_t nextWriteBuffer() const { return ids_[writeBufferIndex_]; }

		/**
		 * @return the current client data, initially all zero.
		 */
		const byte* clientData() const { return storageClientData_; }

		/**
		 * @return the mapped GPU data for reading if mapping is persistent.
		 */
		const byte* mappedReadData() const {
			return storageMappedData_[readBufferIndex_];
		}

		/**
		 * @return the mapped GPU data for writing if mapping is persistent.
		 */
		const byte* mappedWriteData() const {
			return storageMappedData_[writeBufferIndex_];
		}

	protected:
		uint32_t storageFlags_;
		Buffering storageBuffering_;
		std::vector<byte*> storageMappedData_;
		std::vector<bool> hasData_;
		byte* storageClientData_ = nullptr;
		int readBufferIndex_ = 0;
		int writeBufferIndex_ = 0;
		uint32_t storageSize_ = 0;
	};

	/**
	 * \brief A utility class for mapping single structs for reading or writing.
	 */
	template<typename T> class BufferStructMapping : public BufferMapping {
	public:
		explicit BufferStructMapping(uint32_t storageFlags, Buffering storageBuffering = Buffering::DOUBLE_BUFFER)
				: BufferMapping(storageFlags, storageBuffering) {
			initializeMapping(sizeof(T));
		}
		const T& storageValue() {
			return *((const T*)storageClientData_);
		}
	};

	/**
	 * \brief A utility class for mapping arrays of structs for reading or writing.
	 */
	template<typename T> class BufferArrayMapping : public BufferMapping {
	public:
		BufferArrayMapping(uint32_t storageFlags, uint32_t numArrayElements,
				Buffering storageBuffering = Buffering::DOUBLE_BUFFER)
				: BufferMapping(storageFlags, storageBuffering) {
			initializeMapping(sizeof(T)*numArrayElements);
		}
		const T* storageArray() {
			return ((const T*)storageClientData_);
		}
	};
} // namespace

#endif /* REGEN_BUFFER_REFERENCE_H_ */
