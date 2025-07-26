
#ifndef SHADER_INPUT_DATA_H_
#define SHADER_INPUT_DATA_H_

#include <regen/regen.h>
#include <regen/buffer/client-buffer.h>
#include <regen/utility/ref-ptr.h>
#include <regen/utility/logging.h>

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
	T& w_access_packed(byte* base, size_t index, size_t /*stride*/) {
		return reinterpret_cast<T*>(base)[index];
	}

	template<typename T>
	T& w_access_strided(byte* base, size_t index, size_t stride) {
		return *reinterpret_cast<T*>(base + index * stride);
	}

	template<typename T>
	const T& r_access_packed(const byte* base, size_t index, size_t /*stride*/) {
		return reinterpret_cast<const T*>(base)[index];
	}

	template<typename T>
	const T& r_access_strided(const byte* base, size_t index, size_t stride) {
		return *reinterpret_cast<const T*>(base + index * stride);
	}

	template<typename T>
	class WriteAccessor {
	public:
		using AccessFunc = T& (*)(byte*, size_t, size_t);

		WriteAccessor(byte* base, size_t stride, AccessFunc func)
			: base_(base), stride_(stride), accessFunc_(func) {}

		T& operator[](size_t index) {
			return accessFunc_(base_, index, stride_);
		}

		bool hasData() const { return base_ != nullptr; }
		T* data() { return reinterpret_cast<T *>(base_); }

	private:
		byte* base_;
		size_t stride_;
		AccessFunc accessFunc_;
	};

	template<typename T>
	class ReadAccessor {
	public:
		using AccessFunc = const T& (*)(const byte*, size_t, size_t);

		ReadAccessor(const byte* base, size_t stride, AccessFunc func)
			: base_(base), stride_(stride), accessFunc_(func) {}

		const T& operator[](size_t index) const {
			return accessFunc_(base_, index, stride_);
		}

		const T* data() const { return reinterpret_cast<const T *>(base_); }
		bool hasData() const { return base_ != nullptr; }

	private:
		const byte* base_;
		size_t stride_;
		AccessFunc accessFunc_;
	};

	/**
	 * A low-level interface for typed read/write access to client data of shader input.
	 * @tparam T the data type.
	 */
	template<typename T>
	struct ShaderData_rw {
		/**
		 * Packed-access constructor.
		 * @param clientBuffer the client buffer.
		 * @param stride the stride in bytes between consecutive elements of type T.
		 * @param mapMode the mapping mode.
		 * @param mapOffset the offset in bytes from the start of the buffer.
		 * @param mapSize the size in bytes to map.
		 */
		ShaderData_rw(ClientBuffer *clientBuffer,
					uint32_t stride,
					int32_t mapMode,
					uint32_t mapOffset,
					uint32_t mapSize)
				: rawData(clientBuffer, mapMode, mapOffset, mapSize),
				  r(rawData.r, stride, stride==0u ? r_access_packed<T> : r_access_strided<T>),
				  w(rawData.w, stride, stride==0u ? w_access_packed<T> : w_access_strided<T>) {
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
			return ShaderData_rw<T>(nullptr, 0, 0, 0, 0);
		}

	private:
		ShaderDataRaw_rw rawData;

	public:
		/**
		 * The mapped data for reading.
		 */
		const ReadAccessor<T> r;
		/**
		 * The mapped data for writing.
		 */
		WriteAccessor<T> w;

		friend class ShaderInput;
	};

	/**
	 * A low-level interface for typed read-only access to client data of shader input.
	 * @tparam T the data type.
	 */
	template<typename T>
	struct ShaderData_ro {
		/**
		 * Packed-access constructor.
		 * @param clientBuffer the client buffer.
		 * @param stride the stride in bytes between consecutive elements of type T.
		 * @param mapMode the mapping mode.
		 * @param mapOffset the offset in bytes from the start of the buffer.
		 * @param mapSize the size in bytes to map.
		 */
		ShaderData_ro(ClientBuffer *clientBuffer,
					uint32_t stride,
					int32_t mapMode,
					uint32_t mapOffset,
					uint32_t mapSize)
				: rawData(clientBuffer, mapMode, mapOffset, mapSize),
				  r(rawData.r, stride, stride==0u ? r_access_packed<T> : r_access_strided<T>) {
		}

		// do not allow copying
		ShaderData_ro(const ShaderData_ro &) = delete;

		/**
		 * Unmap the data. Do not read after calling this method.
		 */
		void unmap() { rawData.unmap(); }

	private:
		ShaderDataRaw_ro rawData;

	public:
		const ReadAccessor<T> r;

		friend class ShaderInput;
	};

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
