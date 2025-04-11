/*
 * shader-input.h
 *
 *  Created on: 15.08.2012
 *      Author: daniel
 */

#ifndef SHADER_INPUT_H_
#define SHADER_INPUT_H_

#include <string>
#include <map>
#include <atomic>

#include <regen/gl-types/buffer-reference.h>
#include <regen/gl-types/gl-enum.h>
#include <regen/gl-types/shader-data.h>
#include <regen/utility/ref-ptr.h>
#include <regen/utility/stack.h>
#include <regen/utility/string-util.h>
#include <regen/math/matrix.h>
#include <regen/math/vector.h>
#include <condition_variable>
#include "regen/scene/input-schema.h"

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
		static ref_ptr<ShaderInput> copy(const ref_ptr<ShaderInput> &in, GLboolean copyData = GL_FALSE);

		/**
		 * @param name Name of this attribute used in shader programs.
		 * @param baseType Specifies the data type of each component in the array.
		 * @param dataTypeBytes Size of a single instance of the data type in bytes.
		 * @param valsPerElement Specifies the number of components per generic vertex attribute.
		 * @param numArrayElements Number of array elements.
		 * @param normalize Specifies whether fixed-point data values should be normalized.
		 */
		ShaderInput(
				const std::string &name,
				GLenum baseType,
				GLuint dataTypeBytes,
				GLint valsPerElement,
				GLsizei numArrayElements,
				GLboolean normalize);

		virtual ~ShaderInput();

		/**
		 * Write ShaderInput.
		 */
		virtual void write(std::ostream &out) const = 0;

		/**
		 * Name of this attribute used in shader programs.
		 */
		auto &name() const { return name_; }

		/**
		 * Name of this attribute used in shader programs.
		 */
		void set_name(const std::string &s) { name_ = s; }

		/**
		 * no call to glUniform when inactive.
		 * @return the active toggle value
		 */
		auto active() const { return active_; }

		/**
		 * no call to glUniform when inactive.
		 * @param v the active toggle value
		 */
		void set_active(GLboolean v) { active_ = v; }

		/**
		 * Compare stamps to check if the input data changed.
		 */
		unsigned int stamp() const;

		/**
		 * Increment the stamp.
		 */
		void nextStamp();

		/**
		 * Specifies the data type of each component in the array.
		 * Symbolic constants GL_FLOAT,GL_DOUBLE,.. accepted.
		 */
		GLenum baseType() const { return baseType_; }

		/**
		 * Specified the complex data type, e.g. GL_RGBA32F for Vec4f.
		 * @return the complex data type.
		 */
		GLenum dataType() const;

		/**
		 * Size of a single instance of the data type in bytes.
		 */
		auto dataTypeBytes() const { return dataTypeBytes_; }

		/**
		 * Specifies the byte offset between consecutive generic vertex attributes.
		 * If stride is 0, the generic vertex attributes are understood to be tightly
		 * packed in the array. The initial value is 0.
		 */
		void set_stride(GLsizei stride) { stride_ = stride; }

		/**
		 * VBO that contains this vertex data.
		 * Iterator should be exclusively owned by this instance.
		 */
		void set_buffer(GLuint buffer, const ref_ptr<BufferReference> &ref);

		/**
		 * VBO that contains this vertex data.
		 */
		auto buffer() const { return buffer_; }

		/**
		 * data with stamp was uploaded to GL.
		 */
		auto bufferStamp() const { return bufferStamp_; }

		/**
		 * Iterator to allocated VBO block.
		 */
		auto &bufferIterator() const { return bufferIterator_; }

		/**
		 * Specifies the byte offset between consecutive generic vertex attributes.
		 * If stride is 0, the generic vertex attributes are understood to be tightly
		 * packed in the array. The initial value is 0.
		 */
		auto stride() const { return stride_; }

		/**
		 * Attribute size for all vertices.
		 */
		auto inputSize() const { return inputSize_; }

		/**
		 * Attribute size for all vertices.
		 */
		void set_inputSize(GLuint size) { inputSize_ = size; }

		/**
		 * Attribute size for a single vertex.
		 */
		auto elementSize() const { return elementSize_; }

		/**
		 * Offset in the VBO to the first
		 * attribute element.
		 */
		void set_offset(GLuint offset) { offset_ = offset; }

		/**
		 * Offset in the VBO to the first
		 * attribute element.
		 */
		auto offset() const { return offset_; }

		/**
		 * numArrayElements() * numInstances()
		 */
		auto numElements() const { return numElements_; }

		/**
		 * Number of array elements.
		 * returns 1 if this is not an array attribute.
		 */
		auto numArrayElements() const { return numArrayElements_; }

		/**
		 * Number of array elements.
		 * returns 1 if this is not an array attribute.
		 */
		void set_numArrayElements(GLsizei v);

		/**
		 * Specifies the number of components per generic vertex attribute.
		 * Must be 1, 2, 3, or 4.
		 */
		auto valsPerElement() const { return valsPerElement_; }

		/**
		 * Used for instanced attributes.
		 */
		auto numInstances() const { return numInstances_; }

		/**
		 * Specify the number of instances that will pass between updates
		 * of the generic attribute at slot index.
		 */
		auto divisor() const { return divisor_; }

		/**
		 * Specifies whether fixed-point data values should be normalized (GL_TRUE)
		 * or converted directly as fixed-point values (GL_FALSE) when they are accessed.
		 */
		auto normalize() const { return normalize_; }

		/**
		 * @param transpose transpose the data.
		 */
		void set_transpose(GLboolean transpose) { transpose_ = transpose; }

		/**
		 * @return transpose the data.
		 */
		auto transpose() const { return transpose_; }

		/**
		 * @return the vertex count.
		 */
		auto numVertices() const { return numVertices_; }

		/**
		 * @param numVertices the vertex count.
		 */
		void set_numVertices(GLuint numVertices) { numVertices_ = numVertices; }

		/**
		 * Returns true if this input is a vertex attribute or
		 * an instanced attribute.
		 */
		auto isVertexAttribute() const { return isVertexAttribute_; }

		/**
		 * Constants can not change the value during the lifetime
		 * of the shader program.
		 */
		void set_isConstant(GLboolean isConstant) { isConstant_ = isConstant; }

		/**
		 * Constants can not change the value during the lifetime
		 * of the shader program.
		 */
		auto isConstant() const { return isConstant_; }

		/**
		 * @return true if this input is a uniform block.
		 */
		auto isBufferBlock() const { return isBufferBlock_; }

		/**
		 * Uniforms with a single array element will appear
		 * with [1] in the generated shader if forceArray is true.
		 * Note: attributes can not be arrays.
		 */
		void set_forceArray(GLboolean forceArray) { forceArray_ = forceArray; }

		/**
		 * Uniforms with a single array element will appear
		 * with [1] in the generated shader if forceArray is true.
		 * Note: attributes can not be arrays.
		 */
		auto forceArray() const { return forceArray_; }

		/**
		 * Allocates RAM for the attribute and does a memcpy
		 * if the data pointer is not null.
		 * numVertices*elementSize bytes will be allocated.
		 */
		void setVertexData(GLuint numVertices, const byte *vertexData = nullptr);

		/**
		 * Allocates RAM for the attribute and does a memcpy
		 * if the data pointer is not null.
		 * numInstances*elementSize/divisor bytes will be allocated.
		 */
		void setInstanceData(GLuint numInstances, GLuint divisor, const byte *instanceData = nullptr);

		/**
		 * @param data the input data.
		 */
		void setUniformUntyped(const byte *data = nullptr);

		/**
		 * Map client data for reading/writing.
		 * @return the mapped data.
		 */
		ShaderDataRaw_rw mapClientDataRaw(int mapMode) { return {this, mapMode}; }

		/**
		 * Map client data for reading/writing.
		 * @return the mapped data.
		 */
		ShaderDataRaw_ro mapClientDataRaw(int mapMode) const { return {this, mapMode}; }

		/**
		 * Map client data for reading/writing.
		 * @tparam T the data type.
		 * @param mapMode the map mode.
		 * @return the mapped data.
		 */
		template<typename T>
		ShaderData_rw<T> mapClientData(int mapMode) { return {this, mapMode}; }

		/**
		 * Map client data for reading/writing.
		 * @tparam T the data type.
		 * @param mapMode the map mode.
		 * @return the mapped data.
		 */
		template<typename T>
		ShaderData_ro<T> mapClientData(int mapMode) const { return {this, mapMode}; }

		/**
		 * Map a single vertex for reading/writing.
		 * @tparam T the data type.
		 * @param mapMode the map mode.
		 * @param vertexIndex the vertex index.
		 * @return the mapped vertex.
		 */
		template<typename T>
		ShaderVertex_rw<T> mapClientVertex(int mapMode, unsigned int vertexIndex) {
			return {this, mapMode, vertexIndex};
		}

		/**
		 * Map a single vertex for reading/writing.
		 * @tparam T the data type.
		 * @param mapMode the map mode.
		 * @param vertexIndex the vertex index.
		 * @return the mapped vertex.
		 */
		template<typename T>
		ShaderVertex_ro<T> mapClientVertex(int mapMode, unsigned int vertexIndex) const {
			return {this, mapMode, vertexIndex};
		}

		/**
		 * Writes client data at index.
		 * Note that it is more efficient to map the data and write directly to it
		 * in case you can write multiple vertices at once.
		 * @param index vertex index.
		 * @param data the data, will be copied into the internal data buffer.
		 */
		void writeVertex(GLuint index, const byte *data);

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
		 * Write a single vertex to the GL server.
		 * @param rs The RenderState.
		 * @param index The vertex index.
		 */
		void writeServerData(GLuint index) const;

		/**
		 * Write this attribute to the GL server.
		 * @param rs The RenderState.
		 */
		void writeServerData() const;

		/**
		 * Returns true if this attribute is allocated in RAM
		 * or if it was uploaded to GL already.
		 */
		GLboolean hasData() const;

		/**
		 * Returns true if this attribute is allocated in RAM.
		 */
		GLboolean hasClientData() const;

		/**
		 * Returns true if this attribute was uploaded to GL already.
		 */
		GLboolean hasServerData() const;

		/**
		 * Binds vertex attribute for active buffer to the
		 * given shader location.
		 */
		void enableAttribute(GLint loc) const;

		/**
		 * Binds uniform to the given shader location.
		 */
		void enableUniform(GLint loc) const;

		/**
		 * Bind the attribute to the given shader location.
		 */
		void enableAttributef(GLint location) const;

		/**
		 * Only the integer types GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT,
		 * GL_UNSIGNED_SHORT, GL_INT, GL_UNSIGNED_INT are accepted.
		 * Values are always left as integer values.
		 */
		void enableAttributei(GLint location) const;

		/**
		 * Matrix attributes have special enable functions.
		 */
		void enableAttributeMat4(GLint location) const;

		/**
		 * Matrix attributes have special enable functions.
		 */
		void enableAttributeMat3(GLint location) const;

		/**
		 * Matrix attributes have special enable functions.
		 */
		void enableAttributeMat2(GLint location) const;

		/**
		 * @return the input schema.
		 */
		const InputSchema& schema() const { return *schema_; }

		/**
		 * Set the input schema.
		 */
		void setSchema(const InputSchema *schema) { schema_ = schema; }

	protected:
		std::string name_;
		GLenum baseType_;
		GLuint dataTypeBytes_;
		GLsizei stride_;
		GLuint offset_;
		GLuint inputSize_;
		// This is the size in bytes of one element in the vertex buffer.
		// e.g. elementSize(vec3f[2]) = 2 * 3 * sizeof(float)
		GLuint elementSize_;
		GLuint numArrayElements_;
		GLuint numVertices_;
		GLuint numInstances_;
		GLsizei numElements_;
		GLint valsPerElement_;
		GLuint divisor_;
		GLuint buffer_;
		mutable GLuint bufferStamp_;
		ref_ptr<BufferReference> bufferIterator_;
		bool normalize_;
		bool isVertexAttribute_;
		bool transpose_;

		struct SlotLock {
			std::mutex lock;
			std::condition_variable readerQ;
			std::condition_variable writerQ;
			int activeReaders = 0;
			int activeWriters = 0;
			int waitingWriters = 0;
		};
		// Note: marked as mutable because client data mapping must be allowed in const functions
		//       for reading data, but mapping interacts with locks. Hence, locks must be mutable.
		mutable std::array<byte *, 2> dataSlots_;
		mutable std::array<SlotLock, 2> slotLocks_;
		mutable std::atomic<int> lastDataSlot_ = 0;
		mutable std::atomic<unsigned int> dataStamp_ = 0;

		bool isConstant_;
		bool isBufferBlock_;
		bool forceArray_;
		bool active_;
		mutable bool requiresReUpload_ = false;

		const InputSchema *schema_ = InputSchema::unknown();

		void (ShaderInput::*enableAttribute_)(GLint loc) const;

		MappedData mapClientData(int mapMode) const;

		void unmapClientData(int mapMode, int slotIndex) const;

		const byte *readLock(int slotIndex) const;

		const byte *readLockTry(int dataSlot) const;

		void readUnlock(int slotIndex) const;

		byte *writeLock(int slotIndex) const;

		byte *writeLockTry(int slotIndex) const;

		void writeUnlock(int slotIndex, bool hasDataChanged) const;

		void writeLockAll() const;

		void writeUnlockAll(bool hasDataChanged) const;

		bool hasTwoSlots() const { return dataSlots_[1] != nullptr; }

		int lastDataSlot() const;

		void allocateSecondSlot() const;

		void reallocateClientData(size_t size);

		bool writeClientData_(const byte *data);

		friend struct ShaderDataRaw_rw;
		friend struct ShaderDataRaw_ro;

		//void (ShaderInput::*enableUniform_)(GLint loc) const;
		std::function<void(GLint)> enableInput_;

		ShaderInput(const ShaderInput &);

		ShaderInput &operator=(const ShaderInput &) { return *this; }
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
		explicit NamedShaderInput(const ref_ptr<ShaderInput> &in, const std::string &name = "",
								  const std::string &type = "");

		/** the shader input data. */
		ref_ptr<ShaderInput> in_;
		/** the name overwrite. */
		// TODO: could use global atom table for shader input names
		std::string name_;
		/** the type overwrite. */
		std::string type_;
	};

	/**
	 * ShaderInput container.
	 */
	typedef std::list<NamedShaderInput> ShaderInputList;
	typedef std::list<ref_ptr<ShaderInput> >::const_iterator AttributeIteratorConst;

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
				GLuint numArrayElements,
				GLboolean normalize)
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
		void setVertex(GLuint i, const ValueType &val) {
			auto mapped = mapClientData<ValueType>(ShaderData::WRITE | ShaderData::INDEX);
			mapped.w[i] = val;
		}

		/**
		 * @param vertexIndex index in data array.
		 * @return data value at given index.
		 */
		ShaderVertex_ro<ValueType> getVertex(GLuint i) const {
			return mapClientVertex<ValueType>(ShaderData::READ, i);
		}

		/**
		 * Set a value for the active stack data at index or the first vertex if index is out of bounds.
		 * @param vertexIndex index in data array.
		 * @param val the new value.
		 */
		void setVertexClamped(GLuint i, const ValueType &val) { setVertex(numElements_ > i ? i : 0, val); }

		/**
		 * Get vertex at index or the first vertex if index is out of bounds.
		 * @param vertexIndex index in data array.
		 * @return data value at given index.
		 */
		auto getVertexClamped(GLuint i) const { return getVertex(numElements_ > i ? i : 0); }

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
	class ShaderInput1f : public ShaderInputTyped<GLfloat, GLfloat, GL_FLOAT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput1f(
				const std::string &name,
				GLuint numArrayElements = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 2D float input to shader programs.
	 */
	class ShaderInput2f : public ShaderInputTyped<Vec2f, GLfloat, GL_FLOAT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput2f(
				const std::string &name,
				GLuint numArrayElements = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 3D float input to shader programs.
	 */
	class ShaderInput3f : public ShaderInputTyped<Vec3f, GLfloat, GL_FLOAT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput3f(
				const std::string &name,
				GLuint numArrayElements = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 4D float input to shader programs.
	 */
	class ShaderInput4f : public ShaderInputTyped<Vec4f, GLfloat, GL_FLOAT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput4f(
				const std::string &name,
				GLuint numArrayElements = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 3x3 matrix input to shader programs.
	 */
	class ShaderInputMat3 : public ShaderInputTyped<Mat3f, GLfloat, GL_FLOAT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInputMat3(
				const std::string &name,
				GLuint numArrayElements = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 4x4 matrix input to shader programs.
	 */
	class ShaderInputMat4 : public ShaderInputTyped<Mat4f, GLfloat, GL_FLOAT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInputMat4(
				const std::string &name,
				GLuint numArrayElements = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 1D double input to shader programs.
	 */
	class ShaderInput1d : public ShaderInputTyped<GLdouble, GLdouble, GL_DOUBLE> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput1d(
				const std::string &name,
				GLuint numArrayElements = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 2D double input to shader programs.
	 */
	class ShaderInput2d : public ShaderInputTyped<Vec2d, GLdouble, GL_DOUBLE> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput2d(
				const std::string &name,
				GLuint numArrayElements = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 3D double input to shader programs.
	 */
	class ShaderInput3d : public ShaderInputTyped<Vec3d, GLdouble, GL_DOUBLE> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput3d(
				const std::string &name,
				GLuint numArrayElements = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 4D double input to shader programs.
	 */
	class ShaderInput4d : public ShaderInputTyped<Vec4d, GLdouble, GL_DOUBLE> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput4d(
				const std::string &name,
				GLuint numArrayElements = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 1D int input to shader programs.
	 */
	class ShaderInput1i : public ShaderInputTyped<GLint, GLint, GL_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput1i(
				const std::string &name,
				GLuint numArrayElements = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 2D int input to shader programs.
	 */
	class ShaderInput2i : public ShaderInputTyped<Vec2i, GLint, GL_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput2i(
				const std::string &name,
				GLuint numArrayElements = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 3D int input to shader programs.
	 */
	class ShaderInput3i : public ShaderInputTyped<Vec3i, GLint, GL_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput3i(
				const std::string &name,
				GLuint numArrayElements = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 4D int input to shader programs.
	 */
	class ShaderInput4i : public ShaderInputTyped<Vec4i, GLint, GL_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput4i(
				const std::string &name,
				GLuint numArrayElements = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 1D unsigned int input to shader programs.
	 */
	class ShaderInput1ui : public ShaderInputTyped<GLuint, GLuint, GL_UNSIGNED_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput1ui(
				const std::string &name,
				GLuint numArrayElements = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 2D unsigned int input to shader programs.
	 */
	class ShaderInput2ui : public ShaderInputTyped<Vec2ui, GLuint, GL_UNSIGNED_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput2ui(
				const std::string &name,
				GLuint numArrayElements = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 3D unsigned int input to shader programs.
	 */
	class ShaderInput3ui : public ShaderInputTyped<Vec3ui, GLuint, GL_UNSIGNED_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput3ui(
				const std::string &name,
				GLuint numArrayElements = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 4D unsigned int input to shader programs.
	 */
	class ShaderInput4ui : public ShaderInputTyped<Vec4ui, GLuint, GL_UNSIGNED_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param numArrayElements number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput4ui(
				const std::string &name,
				GLuint numArrayElements = 1,
				GLboolean normalize = GL_FALSE);
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
} // namespace

#endif /* SHADER_INPUT_H_ */
