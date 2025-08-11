#ifndef REGEN_CLIENT_DATA_H_
#define REGEN_CLIENT_DATA_H_

#include <regen/regen.h>
#include <regen/buffer/client-buffer.h>

namespace regen {
	/**
	 * A low-level interface for read/write access to client data of shader input.
	 * The access is thread-safe and will be synchronized with the GL thread.
	 */
	struct ClientDataRaw_rw {
		/**
		 * Default constructor.
		 * @param input the shader input.
		 * @param mapMode the mapping mode, i.e. a bitwise combination of MappingMode flags.
		 */
		ClientDataRaw_rw(ClientBuffer *clientBuffer, int32_t mapMode, uint32_t offset, uint32_t size);

		/**
		 * Default constructor.
		 * @param input the shader input.
		 * @param mapMode the mapping mode, i.e. a bitwise combination of MappingMode flags.
		 */
		ClientDataRaw_rw(ClientBuffer *clientBuffer, int32_t mapMode);

		~ClientDataRaw_rw();

		// do not allow copying
		ClientDataRaw_rw(const ClientDataRaw_rw &) = delete;

		ClientDataRaw_rw &operator=(const ClientDataRaw_rw &) = delete;

		/**
		 * Unmap the data. Do not read or write after calling this method.
		 */
		void unmap();

		/**
		 * Get the read index of the mapped data.
		 * @return the read index.
		 */
		inline int readIndex() const { return r_index; }

		/**
		 * Get the write index of the mapped data.
		 * @return the write index.
		 */
		inline int writeIndex() const { return w_index; }

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
	struct ClientDataRaw_ro {
		/**
		 * Default constructor.
		 * @param input the shader input.
		 * @param mapMode the mapping mode, i.e. a bitwise combination of MappingMode flags.
		 */
		ClientDataRaw_ro(const ClientBuffer *clientBuffer, int32_t mapMode, uint32_t offset, uint32_t size);

		/**
		 * Default constructor.
		 * @param input the shader input.
		 * @param mapMode the mapping mode, i.e. a bitwise combination of MappingMode flags.
		 */
		ClientDataRaw_ro(const ClientBuffer *clientBuffer, int32_t mapMode);

		~ClientDataRaw_ro();

		// do not allow copying
		ClientDataRaw_ro(const ClientDataRaw_ro &) = delete;

		ClientDataRaw_ro &operator=(const ClientDataRaw_ro &) = delete;

		/**
		 * Get the read index of the mapped data.
		 * @return the read index.
		 */
		inline int readIndex() const { return r_index; }

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
	const T& r_access_vertex(const byte* base, size_t index, size_t stride) {
		if (stride == 0) {
			return reinterpret_cast<const T*>(base)[index];
		} else {
			return *reinterpret_cast<const T*>(base + index * stride);
		}
	}

	template<typename T>
	T& w_access_vertex(byte* base, size_t index, size_t stride) {
		if (stride == 0) {
			return reinterpret_cast<T*>(base)[index];
		} else {
			return *reinterpret_cast<T*>(base + index * stride);
		}
	}

	/**
	 * A low-level interface for typed write access to client data of shader input.
	 * @tparam T the data type.
	 */
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

	/**
	 * A low-level interface for typed read access to client data of shader input.
	 * @tparam T the data type.
	 */
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
	struct ClientData_rw {
		/**
		 * Packed-access constructor.
		 * @param clientBuffer the client buffer.
		 * @param stride the stride in bytes between consecutive elements of type T.
		 * @param mapMode the mapping mode.
		 * @param mapOffset the offset in bytes from the start of the buffer.
		 * @param mapSize the size in bytes to map.
		 */
		ClientData_rw(ClientBuffer *clientBuffer,
					  uint32_t stride,
					  int32_t mapMode,
					  uint32_t mapOffset,
					  uint32_t mapSize)
				: rawData(clientBuffer, mapMode, mapOffset, mapSize),
				  r(rawData.r, stride, stride==0u ? r_access_packed<T> : r_access_strided<T>),
				  w(rawData.w, stride, stride==0u ? w_access_packed<T> : w_access_strided<T>) {
		}

		// do not allow copying
		ClientData_rw(const ClientData_rw &) = delete;

		/**
		 * Get the read index of the mapped data.
		 * @return the read index.
		 */
		int readIndex() const { return rawData.readIndex(); }

		/**
		 * Get the write index of the mapped data.
		 * @return the write index.
		 */
		int writeIndex() const { return rawData.writeIndex(); }

		/**
		 * Unmap the data. Do not read or write after calling this method.
		 */
		void unmap() { rawData.unmap(); }

		/**
		 * Create a null data object.
		 * @return a null data object.
		 */
		static ClientData_rw<T> nullData() {
			return ClientData_rw<T>(nullptr, 0, 0, 0, 0);
		}

	private:
		ClientDataRaw_rw rawData;

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
	struct ClientData_ro {
		/**
		 * Packed-access constructor.
		 * @param clientBuffer the client buffer.
		 * @param stride the stride in bytes between consecutive elements of type T.
		 * @param mapMode the mapping mode.
		 * @param mapOffset the offset in bytes from the start of the buffer.
		 * @param mapSize the size in bytes to map.
		 */
		ClientData_ro(ClientBuffer *clientBuffer,
					  uint32_t stride,
					  int32_t mapMode,
					  uint32_t mapOffset,
					  uint32_t mapSize)
				: rawData(clientBuffer, mapMode, mapOffset, mapSize),
				  r(rawData.r, stride, stride==0u ? r_access_packed<T> : r_access_strided<T>) {
		}

		// do not allow copying
		ClientData_ro(const ClientData_ro &) = delete;

		/**
		 * Get the read index of the mapped data.
		 * @return the read index.
		 */
		int readIndex() const { return rawData.readIndex(); }

		/**
		 * Unmap the data. Do not read after calling this method.
		 */
		void unmap() { rawData.unmap(); }

	private:
		ClientDataRaw_ro rawData;

	public:
		const ReadAccessor<T> r;

		friend class ShaderInput;
	};

	/**
	 * A low-level interface for read/write access to a single vertex of client data of shader input.
	 * @tparam T the data type.
	 */
	template<typename T>
	struct ClientVertex_rw {
		/**
		 * Default constructor.
		 * @param input the shader input.
		 * @param mapMode the mapping mode, i.e. a bitwise combination of MappingMode flags.
		 * @param vertexIndex the vertex index.
		 */
		ClientVertex_rw(ClientBuffer *clientBuffer,
						uint32_t stride,
						int32_t mapMode,
						uint32_t vertexIndex)
				: vertexSize(stride > 0 ? stride : sizeof(T)),
				  rawData(clientBuffer, mapMode, vertexIndex*vertexSize, vertexSize),
				  r(((const T*)rawData.r)[0]),
				  w(((T*)rawData.w)[0]) {
		}

		// do not allow copying
		ClientVertex_rw(const ClientVertex_rw &) = delete;

		/**
		 * Get the read index of the mapped data.
		 * @return the read index.
		 */
		int readIndex() const { return rawData.readIndex(); }

		/**
		 * Get the write index of the mapped data.
		 * @return the write index.
		 */
		int writeIndex() const { return rawData.writeIndex(); }

		/**
		 * Unmap the data. Do not read or write after calling this method.
		 */
		void unmap() { rawData.unmap(); }

	private:
		const uint32_t vertexSize;
		ClientDataRaw_rw rawData;
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
	struct ClientVertex_ro {
		/**
		 * Default constructor.
		 * @param input the shader input.
		 * @param mapMode the mapping mode, i.e. a bitwise combination of MappingMode flags.
		 * @param vertexIndex the vertex index.
		 */
		ClientVertex_ro(const ClientBuffer *clientBuffer,
						uint32_t stride,
						int32_t mapMode,
						uint32_t vertexIndex)
				: vertexSize(stride > 0 ? stride : sizeof(T)),
				  rawData(clientBuffer, mapMode, vertexIndex*vertexSize, vertexSize),
				  r(((const T*)rawData.r)[0]) {
		}

		// do not allow copying
		ClientVertex_ro(const ClientVertex_ro &) = delete;

		/**
		 * Get the read index of the mapped data.
		 * @return the read index.
		 */
		int readIndex() const { return rawData.readIndex(); }

		/**
		 * Unmap the data. Do not read after calling this method.
		 */
		void unmap() { rawData.unmap(); }

	private:
		const uint32_t vertexSize;
		ClientDataRaw_ro rawData;
	public:
		/**
		 * The mapped data for reading.
		 */
		const T &r;

		friend class ShaderInput;
	};
} // namespace

#endif /* REGEN_CLIENT_DATA_H_ */
