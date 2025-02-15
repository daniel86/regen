
#ifndef SHADER_INPUT_DATA_H_
#define SHADER_INPUT_DATA_H_

namespace regen::ShaderData {
	/**
	 * Flags for client data read/write access.
	 */
	enum MappingMode {
		READ = 1 << 0,
		WRITE = 1 << 1,
		// indicates that only a single vertex is mapped
		INDEX = 1 << 2
	};
}

namespace regen {
	class ShaderInput;

#ifndef byte
	typedef unsigned char byte;
#endif

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
		ShaderDataRaw_rw(ShaderInput *input, int mapMode);

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
		ShaderInput *input;
		int r_index;
		int w_index;
		const int mapMode;

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
		ShaderDataRaw_ro(const ShaderInput *input, int mapMode);

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
		const ShaderInput *input;
		int r_index;
		const int mapMode;

		friend class ShaderInput;
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
		ShaderData_rw(ShaderInput *input, int mapMode)
				: rawData(input, mapMode),
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
			return ShaderData_rw<T>(nullptr, 0);
		}

	private:
		ShaderDataRaw_rw rawData;
	public:
		/**
		 * The mapped data for reading.
		 */
		const T *r;
		/**
		 * The mapped data for writing.
		 */
		T *w;

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
		ShaderData_ro(const ShaderInput *input, int mapMode)
				: rawData(input, mapMode),
				  r(reinterpret_cast<const T *>(rawData.r)) {
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
		/**
		 * The mapped data for reading.
		 */
		const T *r;

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
		ShaderVertex_rw(ShaderInput *input, int mapMode, unsigned int vertexIndex)
				: rawData(input, mapMode | ShaderData::INDEX),
				  r(((const T *) rawData.r)[vertexIndex]),
				  w(((T *) rawData.w)[vertexIndex]) {
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
		ShaderVertex_ro(const ShaderInput *input, int mapMode, unsigned int vertexIndex)
				: rawData(input, mapMode | ShaderData::INDEX),
				  r(((const T *) rawData.r)[vertexIndex]) {
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

	/**
	 * A low-level interface for read/write access to client data of shader input.
	 */
	struct MappedData {
		/**
		 * Default constructor.
		 * @param r the read data.
		 * @param r_index the read index.
		 * @param w the write data.
		 * @param w_index the write index.
		 */
		MappedData(const byte *r, int r_index, byte *w, int w_index)
				: r(r), w(w), r_index(r_index), w_index(w_index) {}

		/**
		 * Read-only constructor.
		 * @param r the read data.
		 * @param r_index the read index.
		 */
		MappedData(const byte *r, int r_index)
				: r(r), w(nullptr), r_index(r_index), w_index(-1) {}

		/**
		 * The mapped data for reading.
		 */
		const byte *r;
		/**
		 * The mapped data for writing.
		 */
		byte *w;
		/**
		 * The read index.
		 */
		int r_index;
		/**
		 * The write index.
		 */
		int w_index;
	};
} // namespace

#endif /* SHADER_INPUT_DATA_H_ */
