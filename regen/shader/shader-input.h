#ifndef SHADER_INPUT_H_
#define SHADER_INPUT_H_

#include <string>
#include <map>
#include <atomic>

#include "regen/memory/buffer-reference.h"
#include "regen/memory/client-data.h"
#include "regen/gl/gl-enum.h"
#include "regen/utility/ref-ptr.h"
#include "regen/utility/stack.h"
#include "regen/utility/strings.h"
#include "regen/compute/matrix.h"
#include "regen/compute/vector.h"
#include <condition_variable>
#include "input-schema.h"
#include "regen/memory/client-buffer.h"
#include "regen/memory/buffer-enums.h"

namespace regen {
	// default attribute names
#define ATTRIBUTE_NAME_POS "pos"
#define ATTRIBUTE_NAME_NOR "nor"
#define ATTRIBUTE_NAME_TAN "tan"
#define ATTRIBUTE_NAME_COL0 "col0"
#define ATTRIBUTE_NAME_COL1 "col1"

	/**
	 * \brief Provides input to shader programs.
	 *
	 * Inputs can be constants, uniforms, instanced attributes
	 * and per vertex attributes.
	 *
	 * Vertex attributes are used to communicate from "outside"
	 * to the vertex shader. Unlike uniform variables,
	 * values are provided per vertex (and not globally for all vertices).
	 *
	 * A constant is a global GLSL variable declared with the "constant"
	 * storage qualifier. These are compiled into generated shaders,
	 * if the user changes the value it will have no influence until
	 * the shader is regenerated.
	 *
	 * A uniform is a global GLSL variable declared with the "uniform"
	 * storage qualifier. These act as parameters that the user
	 * of a shader program can pass to that program.
	 * They are stored in a program object.
	 * Uniforms are so named because they do not change from
	 * one execution of a shader program to the next within
	 * a particular rendering call.
	 */
	class ShaderInput {
	public:
		/**
		 * Factory function.
		 * @param name the input name.
		 * @param dataType the input data type.
		 * @param valsPerElement number of values per element.
		 * @return the created ShaderInput.
		 */
		static ref_ptr<ShaderInput> create(const ref_ptr<ShaderInput> &in);

		/**
		 * Copy ShaderInput instance.
		 * VBO reference is not copied.
		 * @param in the ShaderInput instance.
		 * @param copyData copy RAM data if any.
		 * @return the copy.
		 */
		static ref_ptr<ShaderInput> copy(const ref_ptr<ShaderInput> &in, bool copyData = false);

		/**
		 * @param name Name of this attribute used in shader programs.
		 * @param baseType Specifies the data type of each component in the array.
		 * @param dataTypeBytes Size of a single instance of the data type in bytes.
		 * @param valsPerElement Specifies the number of components per generic vertex attribute.
		 * @param numArrayElements Number of array elements.
		 * @param normalize Specifies whether fixed-point data values should be normalized.
		 */
		ShaderInput(
				std::string_view name,
				GLenum baseType,
				uint32_t dataTypeBytes,
				int32_t valsPerElement,
				uint32_t numArrayElements,
				bool normalize);

		virtual ~ShaderInput();

		/**
		 * Name of this attribute used in shader programs.
		 */
		auto &name() const { return name_; }

		/**
		 * Name of this attribute used in shader programs.
		 */
		void set_name(const std::string &s) { name_ = s; }

		/**
		 * Editable inputs can be changed by the user.
		 * Non-editable inputs are e.g. system generated
		 * inputs that should not be changed by the user.
		 * @return true if the input is editable.
		 */
		bool isEditable() const { return editable_; }

		/**
		 * Editable inputs can be changed by the user.
		 * Non-editable inputs are e.g. system generated
		 * inputs that should not be changed by the user.
		 * @param v true if the input is editable.
		 */
		void setEditable(bool v) { editable_ = v; }

		/**
		 * no call to glUniform when inactive.
		 * @return the active toggle value
		 */
		inline bool active() const { return active_; }

		/**
		 * no call to glUniform when inactive.
		 * @param v the active toggle value
		 */
		void set_active(bool v) { active_ = v; }

		/**
		 * The data stamp of the data which is currently read.
		 * @return the data stamp.
		 */
		inline uint32_t stampOfReadData() const { return clientBuffer_->stampOfReadData(); }

		/**
		 * The data stamp of the data which is currently written.
		 * This is the data that will be read in the next frame.
		 * @return the data stamp.
		 */
		inline uint32_t stampOfWriteData() const { return clientBuffer_->stampOfWriteData(); }

		/**
		 * Set the memory layout of the input.
		 * @param layout the memory layout of the input.
		 */
		void setMemoryLayout(BufferMemoryLayout layout);

		/**
		 * @return the memory layout of the input.
		 */
		BufferMemoryLayout memoryLayout() const { return memoryLayout_; }

		/**
		 * The base size of the input.
		 * @return the base size of the input in bytes.
		 */
		inline uint32_t baseSize() const { return baseSize_; }

		/**
		 * Specifies the byte offset between consecutive elements of the shader data.
		 * This is e.g. the offset between two Vec3f elements in a Vec3f array.
		 * @return the byte offset between consecutive elements of the shader data.
		 */
		inline uint32_t stride() const { return stride_; }

		/**
		 * Specifies the byte offset between consecutive elements of the shader data.
		 * This is e.g. the offset between two Vec3f elements in a Vec3f array.
		 * @param stride the byte offset between consecutive elements of the shader data.
		 */
		void set_stride(GLsizei stride) { stride_ = stride; }

		/**
		 * Specifies the data type of each component in the array.
		 * Symbolic constants GL_FLOAT,GL_DOUBLE,.. accepted.
		 */
		inline GLenum baseType() const { return baseType_; }

		/**
		 * Specifies the number of components per generic vertex attribute.
		 * Must be 1, 2, 3, or 4.
		 */
		inline int32_t valsPerElement() const { return valsPerElement_; }

		/**
		 * Specified the complex data type, e.g. GL_RGBA32F for Vec4f.
		 * @return the complex data type.
		 */
		inline GLenum dataType() const { return dataType_; }

		/**
		 * Size of a single instance of the data type in bytes.
		 */
		inline uint32_t dataTypeBytes() const { return dataTypeBytes_; }

		/**
		 * Base alignment of the input.
		 * @return the base alignment of the input in bytes.
		 */
		inline uint32_t baseAlignment() const { return baseAlignment_; }

		/**
		 * Aligned base size of the input.
		 * This is the size of a single element with alignment applied.
		 * @return the aligned base size of the input in bytes.
		 */
		inline uint32_t alignedBaseSize() const { return alignedBaseSize_; }

		/**
		 * Aligned size of the input.
		 * @return the aligned size of the input in bytes.
		 */
		inline uint32_t alignedInputSize() const { return alignedInputSize_; }

		/**
		 * This is the number of times the base alignment is applied to the input
		 * per element. i.e. the size of an element is baseAlignment * alignmentCount.
		 * @return the alignment count of the input.
		 */
		inline uint32_t alignmentCount() const { return alignmentCount_; }

		/**
		 * Attribute size for a single vertex.
		 */
		inline uint32_t elementSize() const { return elementSize_; }

		/**
		 * numArrayElements() * numInstances()
		 */
		inline uint32_t numElements() const { return numElements_ui_; }

		/**
		 * Number of array elements.
		 * returns 1 if this is not an array attribute.
		 */
		inline uint32_t numArrayElements() const { return numArrayElements_; }

		/**
		 * Used for instanced attributes.
		 */
		inline uint32_t numInstances() const { return numInstances_; }

		/**
		 * @return the vertex count.
		 */
		inline uint32_t numVertices() const { return numVertices_; }

		/**
		 * Stride used for client data mapping.
		 * This is the size of a single element with alignment applied.
		 * @return the stride used for client data mapping in bytes.
		 */
		inline uint32_t mapClientStride() const { return mapClientStride_; }

		/**
		 * Number of array elements.
		 * returns 1 if this is not an array attribute.
		 */
		void set_numArrayElements(uint32_t v);

		/**
		 * @param numVertices the vertex count.
		 */
		void set_numVertices(uint32_t numVertices) { numVertices_ = numVertices; }

		/**
		 * Attribute size for all vertices.
		 */
		inline uint32_t inputSize() const { return inputSize_; }

		/**
		 * Attribute size for all vertices.
		 */
		void set_inputSize(uint32_t size) { inputSize_ = size; }

		/**
		 * VBO that contains this vertex data.
		 * Iterator should be exclusively owned by this instance.
		 */
		void set_buffer(uint32_t buffer, const ref_ptr<BufferReference> &ref);

		/**
		 * VBO that contains this vertex data.
		 */
		uint32_t buffer() const { return buffer_; }

		/**
		 * data with stamp was uploaded to GL.
		 */
		uint32_t bufferStamp() const { return bufferStamp_; }

		/**
		 * Iterator to allocated VBO block.
		 */
		auto &bufferIterator() const { return bufferIterator_; }

		/**
		 * Offset in the VBO to the first
		 * attribute element.
		 */
		void set_offset(uint32_t offset) { offset_ = offset; }

		/**
		 * Offset in the VBO to the first
		 * attribute element.
		 */
		uint32_t offset() const { return offset_; }

		/**
		 * Specify the number of instances that will pass between updates
		 * of the generic attribute at slot index.
		 */
		uint32_t divisor() const { return divisor_; }

		/**
		 * Set the flag to true if this input is a vertex attribute.
		 */
		void set_isVertexAttribute(bool isVertexAttribute);

		/**
		 * Specifies whether fixed-point data values should be normalized (true)
		 * or converted directly as fixed-point values (false) when they are accessed.
		 */
		bool normalize() const { return normalize_; }

		/**
		 * @param transpose transpose the data.
		 */
		void set_transpose(bool transpose) { transpose_ = transpose; }

		/**
		 * @return transpose the data.
		 */
		bool transpose() const { return transpose_; }

		/**
		 * Returns true if this input is a vertex attribute or
		 * an instanced attribute.
		 */
		bool isVertexAttribute() const { return isVertexAttribute_; }

		/**
		 * Constants can not change the value during the lifetime
		 * of the shader program.
		 */
		void set_isConstant(bool isConstant) { isConstant_ = isConstant; }

		/**
		 * Constants can not change the value during the lifetime
		 * of the shader program.
		 */
		bool isConstant() const { return isConstant_; }

		/**
		 * @return true if this input is a uniform block.
		 */
		bool isBufferBlock() const { return isBufferBlock_; }

		/**
		 * @return true if this input is a struct (array).
		 */
		bool isStruct() const { return isStruct_; }

		/**
		 * Uniforms with a single array element will appear
		 * with [1] in the generated shader if forceArray is true.
		 * Note: attributes can not be arrays.
		 */
		void set_forceArray(bool forceArray) { forceArray_ = forceArray; }

		/**
		 * Uniforms with a single array element will appear
		 * with [1] in the generated shader if forceArray is true.
		 * Note: attributes can not be arrays.
		 */
		bool forceArray() const { return forceArray_; }

		/**
		 * Set the server access mode. This is used to determine
		 * how the data is accessed on the server side.
		 * @param mode the server access mode.
		 */
		void setServerAccessMode(ServerAccessMode mode) { serverAccessMode_ = mode; }

		/**
		 * Get the server access mode. This is used to determine
		 * how the data is accessed on the server side.
		 * @return the server access mode.
		 */
		ServerAccessMode serverAccessMode() const { return serverAccessMode_; }

		/**
		 * Allocates RAM for the attribute and does a memcpy
		 * if the data pointer is not null.
		 * numVertices*elementSize bytes will be allocated.
		 */
		void setVertexData(uint32_t numVertices, const byte *vertexData = nullptr);

		/**
		 * Allocates RAM for the attribute and does a memcpy
		 * if the data pointer is not null.
		 * numInstances*elementSize/divisor bytes will be allocated.
		 */
		void setInstanceData(uint32_t numInstances, uint32_t divisor, const byte *instanceData = nullptr);

		/**
		 * @param data the input data.
		 */
		void setUniformUntyped(const byte *data = nullptr);

		/**
		 * Map client data for reading/writing.
		 * @return the mapped data.
		 */
		inline ClientDataRaw_rw mapClientDataRaw(int32_t mapMode) {
			return {clientBuffer_.get(), mapMode, 0, inputSize_};
		}

		/**
		 * Map client data for reading/writing.
		 * @return the mapped data.
		 */
		inline ClientDataRaw_rw mapClientDataRaw(int32_t mapMode, uint32_t offset, uint32_t size) {
			return {clientBuffer_.get(), mapMode, offset, size};
		}

		/**
		 * Map client data for reading/writing.
		 * @return the mapped data.
		 */
		inline ClientDataRaw_ro mapClientDataRaw(int32_t mapMode) const {
			return {clientBuffer_.get(), mapMode, 0, inputSize_};
		}

		/**
		 * Map client data for reading/writing.
		 * @return the mapped data.
		 */
		inline ClientDataRaw_ro mapClientDataRaw(int32_t mapMode, uint32_t mapOffset, uint32_t mapSize) const {
			return {clientBuffer_.get(), mapMode, mapOffset, mapSize};
		}

		/**
		 * Map client data for reading/writing.
		 * @tparam T the data type.
		 * @param mapMode the map mode.
		 * @return the mapped data.
		 */
		template<typename T>
		ClientData_rw<T> mapClientData(int32_t mapMode) {
			return {clientBuffer_.get(), mapClientStride_, mapMode, 0, inputSize_};
		}

		/**
		 * Map client data for reading/writing.
		 * @tparam T the data type.
		 * @param mapMode the map mode.
		 * @return the mapped data.
		 */
		template<typename T>
		ClientData_rw<T> mapClientData(int32_t mapMode, uint32_t mapOffset, uint32_t mapSize) {
			return {clientBuffer_.get(), mapClientStride_, mapMode, mapOffset, mapSize};
		}

		/**
		 * Map client data for reading/writing.
		 * @tparam T the data type.
		 * @param mapMode the map mode.
		 * @return the mapped data.
		 */
		template<typename T>
		ClientData_ro<T> mapClientData(int32_t mapMode) const {
			return {clientBuffer_.get(), mapClientStride_, mapMode, 0, inputSize_};
		}

		/**
		 * Map client data for reading/writing.
		 * @tparam T the data type.
		 * @param mapMode the map mode.
		 * @return the mapped data.
		 */
		template<typename T>
		ClientData_ro<T> mapClientData(int32_t mapMode, uint32_t mapOffset, uint32_t mapSize) const {
			return {clientBuffer_.get(), mapClientStride_, mapMode, mapOffset, mapSize};
		}

		/**
		 * Map a single vertex for reading/writing.
		 * @tparam T the data type.
		 * @param mapMode the map mode.
		 * @param vertexIndex the vertex index.
		 * @return the mapped vertex.
		 */
		template<typename T>
		ClientVertex_rw<T> mapClientVertex(int32_t mapMode, uint32_t vertexIndex) {
			return {clientBuffer_.get(), mapClientStride_, mapMode, vertexIndex};
		}

		/**
		 * Map a single vertex for reading/writing.
		 * @tparam T the data type.
		 * @param mapMode the map mode.
		 * @param vertexIndex the vertex index.
		 * @return the mapped vertex.
		 */
		template<typename T>
		ClientVertex_ro<T> mapClientVertex(int32_t mapMode, uint32_t vertexIndex) const {
			return {clientBuffer_.get(), mapClientStride_, mapMode, vertexIndex};
		}

		/**
		 * Writes client data at index.
		 * Note that it is more efficient to map the data and write directly to it
		 * in case you can write multiple vertices at once.
		 * @param index vertex index.
		 * @param data the data, will be copied into the internal data buffer.
		 */
		void writeVertex(uint32_t index, const byte *data);

		/**
		 * Deallocates data pointer owned by this instance.
		 */
		void deallocateClientData();

		/**
		 * Maps VRAM and copies over data.
		 * Afterwards clientData() will return the data that was uploaded
		 * to the GL.
		 * If not server-side data is available, nothing is done.
		 */
		void readServerData();

		/**
		 * Write this attribute to the GL server.
		 * @param rs The RenderState.
		 */
		void writeServerData() const;

		/**
		 * Returns true if this attribute is allocated in RAM
		 * or if it was uploaded to GL already.
		 */
		bool hasData() const { return hasClientData() || hasServerData(); }

		/**
		 * Returns true if this attribute is allocated in RAM.
		 */
		bool hasClientData() const { return clientBuffer_->hasClientData(); }

		/**
		 * Obtains the client data of the read slot without locking.
		 * Be sure that no other thread is writing to the data at the same time.
		 * @return the client data.
		 */
		byte *clientData() const { return clientBuffer_->clientData(clientBuffer_->currentReadSlot()); }

		/**
		 * Obtains the client data for a specific slot without locking.
		 * Be sure that no other thread is writing to the data at the same time.
		 * @param slot the slot index (0 or 1).
		 * @return the client data for the given slot.
		 */
		byte *clientData(uint32_t slot) const { return clientBuffer_->clientData(slot); }

		/**
		 * Returns the client buffer.
		 * This is used to access the client data.
		 * @return the client buffer.
		 */
		const ref_ptr<ClientBuffer> &clientBuffer() { return clientBuffer_; }

		/**
		 * Returns true if this attribute was uploaded to GL already.
		 */
		bool hasServerData() const { return buffer_ != 0; }

		/**
		 * Binds vertex attribute for active buffer to the
		 * given shader location.
		 */
		void enableAttribute(int loc) const;

		/**
		 * Binds uniform to the given shader location.
		 */
		void enableUniform(int loc) const;

		/**
		 * Bind the attribute to the given shader location.
		 */
		void enableAttribute_f(int location) const;

		/**
		 * Only the integer types GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT,
		 * GL_UNSIGNED_SHORT, GL_INT, GL_UNSIGNED_INT are accepted.
		 * Values are always left as integer values.
		 */
		void enableAttribute_i(int location) const;

		/**
		 * Matrix attributes have special enable functions.
		 */
		void enableAttributeMat4(int location) const;

		/**
		 * Matrix attributes have special enable functions.
		 */
		void enableAttributeMat3(int location) const;

		/**
		 * Matrix attributes have special enable functions.
		 */
		void enableAttributeMat2(int location) const;

		/**
		 * @return the input schema.
		 */
		const InputSchema &schema() const { return *schema_; }

		/**
		 * Set the input schema.
		 */
		void setSchema(const InputSchema *schema) { schema_ = schema; }

		/**
		 * Write ShaderInput.
		 */
		virtual void write(std::ostream &out) const = 0;

	protected:
		std::string name_;
		const GLenum baseType_;
		const GLenum dataType_;
		const uint32_t dataTypeBytes_;
		const uint32_t baseSize_;
		const int32_t valsPerElement_;

		uint32_t baseAlignment_;
		uint32_t alignmentCount_;
		uint32_t alignedBaseSize_;
		uint32_t alignedInputSize_ = 0u;
		uint32_t unalignedSize_ = 0u;

		uint32_t stride_ = 0u;
		uint32_t offset_ = 0u;
		uint32_t inputSize_ = 0u;
		// This is the size in bytes of one element in the vertex buffer.
		// e.g. elementSize(vec3f[2]) = 2 * 3 * sizeof(float)
		uint32_t elementSize_;
		uint32_t numArrayElements_;
		uint32_t numVertices_ = 1u;
		uint32_t numInstances_ = 1u;
		// note: not exactly sure why GL API uses int32_t here,
		//       well we keep num-elements as both signed and unsigned then :/
		int32_t numElements_i_;
		uint32_t numElements_ui_;
		uint32_t divisor_ = 0;

		ServerAccessMode serverAccessMode_ = BUFFER_GPU_READ;
		BufferMemoryLayout memoryLayout_ = BUFFER_MEMORY_PACKED;
		uint32_t buffer_ = 0;
		mutable uint32_t bufferStamp_;
		ref_ptr<BufferReference> bufferIterator_;

		ref_ptr<ClientBuffer> clientBuffer_;
		// stride in bytes for typed client data in the client buffer.
		// 0 is interpreted as tightly packed.
		uint32_t mapClientStride_ = 0;

		bool normalize_;
		bool isVertexAttribute_ = false;
		bool transpose_ = false;
		bool isConstant_ = false;
		bool isBufferBlock_ = false;
		bool isStruct_ = false;
		bool forceArray_ = false;
		bool active_ = true;
		bool editable_ = true;

		const InputSchema *schema_ = InputSchema::unknown();

		void (ShaderInput::*enableAttribute_)(int loc) const;

		std::function<void(int)> enableInput_;

		ShaderInput(const ShaderInput &);

		ShaderInput &operator=(const ShaderInput &) { return *this; }

		void updateAlignment();

		void updateAlignedSize();

		friend struct ClientDataRaw_rw;
		friend struct ClientDataRaw_ro;
	};

	/**
	 * \brief ShaderInput plus optional name overwrite.
	 */
	struct NamedShaderInput {
		/**
		 * @param in the shader input data.
		 * @param name the name overwrite.
		 * @param type the type overwrite.
		 */
		explicit NamedShaderInput(
				const ref_ptr<ShaderInput> &in,
				const std::string &name = "",
				const std::string &type = "",
				const std::string &memberSuffix = "");

		/** the shader input data. */
		ref_ptr<ShaderInput> in_;
		/** the name overwrite. */
		std::string name_;
		/** the type overwrite. */
		std::string type_;
		/** for buffer blocks: a suffix appended for each member. */
		std::string memberSuffix_;
	};

	/**
	 * ShaderInput container.
	 */
	typedef std::list<NamedShaderInput> ShaderInputList;
	typedef std::list<ref_ptr<ShaderInput> >::const_iterator AttributeIteratorConst;

	class ShaderStructBase : public ShaderInput {
	public:
		ShaderStructBase(
				const std::string &structTypeName,
				const std::string &name,
				uint32_t structSize,
				uint32_t numArrayElements,
				bool normalize)
				: ShaderInput(name, GL_NONE, structSize, 1, numArrayElements, normalize),
				  structTypeName_(structTypeName) {
			isStruct_ = true;
		}

		/**
		 * @return The type name for the struct type.
		 */
		auto &structTypeName() const { return structTypeName_; }

	protected:
		std::string structTypeName_;
	};

	/**
	 * \brief Provides typed input to shader programs using a struct type.
	 * User needs to take care of padding.
	 */
	template<class StructType>
	class ShaderInputStruct : public ShaderStructBase {
	public:
		/**
		 * @param name Name of this attribute used in shader programs.
		 * @param numArrayElements Number of array elements.
		 * @param normalize Specifies whether fixed-point data values should be normalized.
		 */
		ShaderInputStruct(
				const std::string &typeName,
				const std::string &name,
				uint32_t numArrayElements,
				bool normalize = false)
				: ShaderStructBase(typeName, name, sizeof(StructType), numArrayElements, normalize) {}

		~ShaderInputStruct() override = default;

		/**
		 * @param data the uniforminput data.
		 */
		void setUniformData(const StructType &data) { setUniformUntyped((const byte *) &data); }

		/**
		 * @return the input data.
		 */
		auto uniformData() { return getVertex(0); }

		/**
		 * Set a value for the active stack data.
		 * @param vertexIndex index in data array.
		 * @param val the new value.
		 */
		void setVertex(uint32_t i, const StructType &val) {
			mapClientVertex<StructType>(BUFFER_GPU_WRITE, i).w = val;
		}

		/**
		 * @param vertexIndex index in data array.
		 * @return data value at given index.
		 */
		ClientVertex_ro<StructType> getVertex(uint32_t i) const {
			return mapClientVertex<StructType>(BUFFER_GPU_READ, i);
		}

		/**
		 * Write ShaderInput.
		 */
		void write(std::ostream &out) const override {
			auto x = getVertex(0);
			out << "struct " << structTypeName_ << " {";
			out << "}";
		}
	};

	/**
	 * \brief Provides typed input to shader programs.
	 *
	 * Template type must implement `<<` and `>>` operator.
	 */
	template<class ValueType, class BaseType, GLenum TypeValue>
	class ShaderInputTyped : public ShaderInput {
	public:
		/**
		 * @param name Name of this attribute used in shader programs.
		 * @param numArrayElements Number of array elements.
		 * @param normalize Specifies whether fixed-point data values should be normalized.
		 */
		ShaderInputTyped(
				const std::string &name,
				uint32_t numArrayElements,
				bool normalize)
				: ShaderInput(name,
							  TypeValue,
							  sizeof(BaseType),
							  sizeof(ValueType) / sizeof(BaseType),
							  numArrayElements, normalize) {}

		/**
		 * Read ShaderInput.
		 */
		virtual std::istream &operator<<(std::istream &in) {
			ValueType value;
			in >> value;
			setUniformData(value);
			return in;
		}

		/**
		 * @param data the uniforminput data.
		 */
		void setUniformData(const ValueType &data) { setUniformUntyped((const byte *) &data); }

		/**
		 * @return the input data.
		 */
		auto uniformData() { return getVertex(0); }

		/**
		 * Set a value for the active stack data.
		 * @param vertexIndex index in data array.
		 * @param val the new value.
		 */
		void setVertex(uint32_t i, const ValueType &val) {
			mapClientVertex<ValueType>(BUFFER_GPU_WRITE, i).w = val;
		}

		/**
		 * @param vertexIndex index in data array.
		 * @return data value at given index.
		 */
		ClientVertex_ro<ValueType> getVertex(uint32_t i) const {
			return mapClientVertex<ValueType>(BUFFER_GPU_READ, i);
		}

		/**
		 * Set a value for the active stack data at index or the first vertex if index is out of bounds.
		 * @param vertexIndex index in data array.
		 * @param val the new value.
		 */
		void setVertexClamped(uint32_t i, const ValueType &val) { setVertex(numElements_ui_ > i ? i : 0, val); }

		/**
		 * Get vertex at index or the first vertex if index is out of bounds.
		 * @param vertexIndex index in data array.
		 * @return data value at given index.
		 */
		auto getVertexClamped(uint32_t i) const { return getVertex(numElements_ui_ > i ? i : 0); }

		/**
		 * Write ShaderInput.
		 */
		virtual std::ostream &operator>>(std::ostream &out) const {
			auto x = getVertex(0);
			auto &v = x.r;
			return out << v;
		}

		/**
		 * Write ShaderInput.
		 */
		void write(std::ostream &out) const override {
			auto x = getVertex(0);
			auto &v = x.r;
			out << v;
			//out << getVertex(0).v;
		}
	};

	/////////////

	/**
	 * \brief Provides 1D float input to shader programs.
	 */
	class ShaderInput1f : public ShaderInputTyped<float, float, GL_FLOAT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput1f(
				const std::string &name,
				uint32_t numArrayElements = 1,
				bool normalize = false);
	};

	/**
	 * \brief Provides 2D float input to shader programs.
	 */
	class ShaderInput2f : public ShaderInputTyped<Vec2f, float, GL_FLOAT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput2f(
				const std::string &name,
				uint32_t numArrayElements = 1,
				bool normalize = false);
	};

	/**
	 * \brief Provides 3D float input to shader programs.
	 */
	class ShaderInput3f : public ShaderInputTyped<Vec3f, float, GL_FLOAT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput3f(
				const std::string &name,
				uint32_t numArrayElements = 1,
				bool normalize = false);
	};

	/**
	 * \brief Provides 4D float input to shader programs.
	 */
	class ShaderInput4f : public ShaderInputTyped<Vec4f, float, GL_FLOAT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput4f(
				const std::string &name,
				uint32_t numArrayElements = 1,
				bool normalize = false);

		void setVertex3(uint32_t i, const Vec3f &val) {
			auto mapped = mapClientVertex<Vec4f>(BUFFER_GPU_WRITE, i);
			mapped.w.xyz() = val;
		}
	};

	/**
	 * \brief Provides 3x3 matrix input to shader programs.
	 */
	class ShaderInputMat3 : public ShaderInputTyped<Mat3f, float, GL_FLOAT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInputMat3(
				const std::string &name,
				uint32_t numArrayElements = 1,
				bool normalize = false);
	};

	/**
	 * \brief Provides 4x4 matrix input to shader programs.
	 */
	class ShaderInputMat4 : public ShaderInputTyped<Mat4f, float, GL_FLOAT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInputMat4(
				const std::string &name,
				uint32_t numArrayElements = 1,
				bool normalize = false);
	};

	/**
	 * \brief Provides 1D double input to shader programs.
	 */
	class ShaderInput1d : public ShaderInputTyped<double, double, GL_DOUBLE> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput1d(
				const std::string &name,
				uint32_t numArrayElements = 1,
				bool normalize = false);
	};

	/**
	 * \brief Provides 2D double input to shader programs.
	 */
	class ShaderInput2d : public ShaderInputTyped<Vec2d, double, GL_DOUBLE> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput2d(
				const std::string &name,
				uint32_t numArrayElements = 1,
				bool normalize = false);
	};

	/**
	 * \brief Provides 3D double input to shader programs.
	 */
	class ShaderInput3d : public ShaderInputTyped<Vec3d, double, GL_DOUBLE> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput3d(
				const std::string &name,
				uint32_t numArrayElements = 1,
				bool normalize = false);
	};

	/**
	 * \brief Provides 4D double input to shader programs.
	 */
	class ShaderInput4d : public ShaderInputTyped<Vec4d, double, GL_DOUBLE> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput4d(
				const std::string &name,
				uint32_t numArrayElements = 1,
				bool normalize = false);
	};

	/**
	 * \brief Provides 1D int input to shader programs.
	 */
	class ShaderInput1i : public ShaderInputTyped<int32_t, int32_t, GL_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput1i(
				const std::string &name,
				uint32_t numArrayElements = 1,
				bool normalize = false);
	};

	/**
	 * \brief Provides 2D int input to shader programs.
	 */
	class ShaderInput2i : public ShaderInputTyped<Vec2i, int32_t, GL_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput2i(
				const std::string &name,
				uint32_t numArrayElements = 1,
				bool normalize = false);
	};

	/**
	 * \brief Provides 3D int input to shader programs.
	 */
	class ShaderInput3i : public ShaderInputTyped<Vec3i, int32_t, GL_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput3i(
				const std::string &name,
				uint32_t numArrayElements = 1,
				bool normalize = false);
	};

	/**
	 * \brief Provides 4D int input to shader programs.
	 */
	class ShaderInput4i : public ShaderInputTyped<Vec4i, int32_t, GL_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput4i(
				const std::string &name,
				uint32_t numArrayElements = 1,
				bool normalize = false);
	};

	/**
	 * \brief Provides 1D unsigned int input to shader programs.
	 */
	class ShaderInput1ui : public ShaderInputTyped<uint32_t, uint32_t, GL_UNSIGNED_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput1ui(
				const std::string &name,
				uint32_t numArrayElements = 1,
				bool normalize = false);
	};

	using ShaderInput1ui_32 = ShaderInput1ui;

	/**
	 * \brief Provides 1D unsigned short input to shader programs.
	 */
	class ShaderInput1ui_16 : public ShaderInputTyped<uint16_t, uint16_t, GL_UNSIGNED_SHORT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput1ui_16(
				const std::string &name,
				uint32_t numArrayElements = 1,
				bool normalize = false);
	};

	/**
	 * \brief Provides 1D unsigned byte input to shader programs.
	 */
	class ShaderInput1ui_8 : public ShaderInputTyped<uint8_t, uint8_t, GL_UNSIGNED_BYTE> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput1ui_8(
				const std::string &name,
				uint32_t numArrayElements = 1,
				bool normalize = false);
	};

	/**
	 * \brief Provides 2D unsigned int input to shader programs.
	 */
	class ShaderInput2ui : public ShaderInputTyped<Vec2ui, uint32_t, GL_UNSIGNED_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput2ui(
				const std::string &name,
				uint32_t numArrayElements = 1,
				bool normalize = false);
	};

	/**
	 * \brief Provides 3D unsigned int input to shader programs.
	 */
	class ShaderInput3ui : public ShaderInputTyped<Vec3ui, uint32_t, GL_UNSIGNED_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput3ui(
				const std::string &name,
				uint32_t numArrayElements = 1,
				bool normalize = false);
	};

	/**
	 * \brief Provides 4D unsigned int input to shader programs.
	 */
	class ShaderInput4ui : public ShaderInputTyped<Vec4ui, uint32_t, GL_UNSIGNED_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput4ui(
				const std::string &name,
				uint32_t numArrayElements = 1,
				bool normalize = false);
	};

	/**
	 * Utility function to create a uniform input.
	 */
	template<class T, class U>
	ref_ptr<T> createUniform(const std::string &name, const U &value) {
		auto uniform = ref_ptr<T>::alloc(name);
		uniform->setUniformData(value);
		return uniform;
	}

	/**
	 * Utility function to create an input for index data.
	 */
	ref_ptr<ShaderInput> createIndexInput(uint32_t numIndices, uint32_t maxVertexIdx);

	/**
	 * Set index value in index buffer.
	 * @param data the index buffer data.
	 * @param t the index type.
	 * @param i the index to set.
	 * @param v the value to set as uint32_t.
	 */
	void setIndexValue(byte *data, GLenum t, uint32_t i, uint32_t v);
} // namespace

#endif /* SHADER_INPUT_H_ */
