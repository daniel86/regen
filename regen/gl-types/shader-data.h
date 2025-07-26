
#ifndef SHADER_INPUT_DATA_H_
#define SHADER_INPUT_DATA_H_

#include <regen/regen.h>
#include <regen/buffer/client-buffer.h>
#include <regen/utility/ref-ptr.h>

namespace regen {
	/**
	 * A low-level interface for read/write access to client data of shader input.
	 * The access is thread-safe and will be synchronized with the GL thread.
	 */
	struct ShaderDataRaw_rw {
		/**
		 * Default constructor.
		 * @param input the shader input.
		 * @param mapMode the mapping mode, i.e. a bitwise combination of MappingMode flags.
		 */
		ShaderDataRaw_rw(ClientBuffer *clientBuffer, int32_t mapMode, uint32_t offset, uint32_t size);

		/**
		 * Default constructor.
		 * @param input the shader input.
		 * @param mapMode the mapping mode, i.e. a bitwise combination of MappingMode flags.
		 */
		ShaderDataRaw_rw(ClientBuffer *clientBuffer, int32_t mapMode);

		~ShaderDataRaw_rw();

		// do not allow copying
		ShaderDataRaw_rw(const ShaderDataRaw_rw &) = delete;

		/**
		 * Unmap the data. Do not read or write after calling this method.
		 */
		void unmap();

		/**
		 * The mapped data for reading.
		 */
		const byte *r;
		/**
		 * The mapped data for writing.
		 */
		byte *w;
	private:
		ClientBuffer *clientBuffer;
		int r_index;
		int w_index;
		const int32_t mapMode;
		const uint32_t mapOffset;
		const uint32_t mapSize;

		friend class ShaderInput;
	};

	/**
	 * A low-level interface for read-only access to client data of shader input.
	 * The access is thread-safe and will be synchronized with the GL thread.
	 */
	struct ShaderDataRaw_ro {
		/**
		 * Default constructor.
		 * @param input the shader input.
		 * @param mapMode the mapping mode, i.e. a bitwise combination of MappingMode flags.
		 */
		ShaderDataRaw_ro(const ClientBuffer *clientBuffer, int32_t mapMode, uint32_t offset, uint32_t size);

		/**
		 * Default constructor.
		 * @param input the shader input.
		 * @param mapMode the mapping mode, i.e. a bitwise combination of MappingMode flags.
		 */
		ShaderDataRaw_ro(const ClientBuffer *clientBuffer, int32_t mapMode);

		~ShaderDataRaw_ro();

		// do not allow copying
		ShaderDataRaw_ro(const ShaderDataRaw_ro &) = delete;

		/**
		 * Unmap the data. Do not read after calling this method.
		 */
		void unmap();

		/**
		 * The mapped data for reading.
		 */
		const byte *r;
	private:
		const ClientBuffer *clientBuffer;
		int r_index;
		const int32_t mapMode;
		const uint32_t mapOffset;
		const uint32_t mapSize;

		friend class ShaderInput;
	};

	template<typename T>
	class PackedWriteAccessor {
	public:
		explicit PackedWriteAccessor(T* data)
			: data_(data) {}

		T& operator[](size_t index) { return data_[index]; }

		bool operator()() const {
			return data_ != nullptr;
		}

		T* data() { return data_; }

		bool hasData() const { return data_ != nullptr; }

	private:
		T* data_;
	};

	template<typename T>
	class PackedReadAccessor {
	public:
		explicit PackedReadAccessor(const T* data) : data_(data) {}

		const T& operator[](size_t index) const { return data_[index]; }

		const T* data() const { return data_; }

		bool hasData() const { return data_ != nullptr; }

	private:
		const T* data_;
	};

	/**
	 * A low-level interface for typed read/write access to client data of shader input.
	 * @tparam T the data type.
	 */
	template<typename T>
	struct ShaderData_rw {
		/**
		 * Default constructor.
		 * @param input the shader input.
		 * @param mapMode the mapping mode, i.e. a bitwise combination of MappingMode flags.
		 */
		ShaderData_rw(ClientBuffer *clientBuffer, int32_t mapMode, uint32_t offset, uint32_t size)
				: rawData(clientBuffer, mapMode, offset, size),
				  r(reinterpret_cast<const T *>(rawData.r)),
				  w(reinterpret_cast<T *>(rawData.w)) {
		}

		// do not allow copying
		ShaderData_rw(const ShaderData_rw &) = delete;

		/**
		 * Unmap the data. Do not read or write after calling this method.
		 */
		void unmap() { rawData.unmap(); }

		/**
		 * Create a null data object.
		 * @return a null data object.
		 */
		static ShaderData_rw<T> nullData() {
			return ShaderData_rw<T>(nullptr, 0, 0, 0);
		}

	private:
		ShaderDataRaw_rw rawData;

	public:
		/**
		 * The mapped data for reading.
		 */
		const PackedReadAccessor<T> r;
		/**
		 * The mapped data for writing.
		 */
		PackedWriteAccessor<T> w;

		friend class ShaderInput;
	};

	/**
	 * A low-level interface for typed read-only access to client data of shader input.
	 * @tparam T the data type.
	 */
	template<typename T>
	struct ShaderData_ro {
		/**
		 * Default constructor.
		 * @param input the shader input.
		 * @param mapMode the mapping mode, i.e. a bitwise combination of MappingMode flags.
		 */
		ShaderData_ro(ClientBuffer *clientBuffer, int32_t mapMode, uint32_t offset, uint32_t size)
				: rawData(clientBuffer, mapMode, offset, size),
				  r(reinterpret_cast<const T *>(rawData.r)) {
		}

		// do not allow copying
		ShaderData_ro(const ShaderData_ro &) = delete;

		//const T& operator[](size_t i) const {
        //	return r[i];
		//}

		/**
		 * Unmap the data. Do not read after calling this method.
		 */
		void unmap() { rawData.unmap(); }

	private:
		ShaderDataRaw_ro rawData;

	public:
		const PackedReadAccessor<T> r;

		friend class ShaderInput;
	};

	/**
	template<typename T>
	class StridedPtr {
	public:
		StridedPtr(const void* base, size_t stride)
			: base_(reinterpret_cast<const uint8_t*>(base)), stride_(stride) {}

		const T& operator[](size_t i) const {
			return *reinterpret_cast<const T*>(base_ + i * stride_);
		}

	private:
		const uint8_t* base_;
		size_t stride_;
	};

	template<typename T>
	struct ShaderData_ro_XXXX {
		ShaderData_ro_XXXX(ClientBuffer* clientBuffer, int32_t mapMode, uint32_t offset, uint32_t count)
			: rawData(clientBuffer, mapMode, offset, count * clientBuffer->itemStride()),
			  r(rawData.r, clientBuffer->itemStride()) {}

		ShaderData_ro_XXXX(const ShaderData_ro&) = delete;

		void unmap() { rawData.unmap(); }

	private:
		ShaderDataRaw_ro rawData;

	public:
		StridedPtr<T> r;

		friend class ShaderInput;
	};
	**/

	/**
	 * A low-level interface for read/write access to a single vertex of client data of shader input.
	 * @tparam T the data type.
	 */
	template<typename T>
	struct ShaderVertex_rw {
		/**
		 * Default constructor.
		 * @param input the shader input.
		 * @param mapMode the mapping mode, i.e. a bitwise combination of MappingMode flags.
		 * @param vertexIndex the vertex index.
		 */
		ShaderVertex_rw(ClientBuffer *clientBuffer, int32_t mapMode, uint32_t vertexIndex)
				: rawData(clientBuffer, mapMode,
						clientBuffer->itemSize() * vertexIndex,
						clientBuffer->itemSize()),
				  r(((const T *) rawData.r)[0]),
				  w(((T *) rawData.w)[0]) {
		}

		// do not allow copying
		ShaderVertex_rw(const ShaderVertex_rw &) = delete;

		/**
		 * Unmap the data. Do not read or write after calling this method.
		 */
		void unmap() { rawData.unmap(); }

	private:
		ShaderDataRaw_rw rawData;
	public:
		/**
		 * The mapped data for reading.
		 */
		const T &r;
		/**
		 * The mapped data for writing.
		 */
		T &w;

		friend class ShaderInput;
	};

	/**
	 * A low-level interface for read-only access to a single vertex of client data of shader input.
	 * @tparam T the data type.
	 */
	template<typename T>
	struct ShaderVertex_ro {
		/**
		 * Default constructor.
		 * @param input the shader input.
		 * @param mapMode the mapping mode, i.e. a bitwise combination of MappingMode flags.
		 * @param vertexIndex the vertex index.
		 */
		ShaderVertex_ro(const ClientBuffer *clientBuffer, int32_t mapMode, uint32_t vertexIndex)
				: rawData(clientBuffer, mapMode,
						clientBuffer->itemSize() * vertexIndex,
						clientBuffer->itemSize()),
				  r(((const T *) rawData.r)[0]) {
		}

		// do not allow copying
		ShaderVertex_ro(const ShaderVertex_ro &) = delete;

		/**
		 * Unmap the data. Do not read after calling this method.
		 */
		void unmap() { rawData.unmap(); }

	private:
		ShaderDataRaw_ro rawData;
	public:
		/**
		 * The mapped data for reading.
		 */
		const T &r;

		friend class ShaderInput;
	};
} // namespace

#endif /* SHADER_INPUT_DATA_H_ */
