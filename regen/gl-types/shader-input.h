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

#include <regen/gl-types/vbo.h>
#include <regen/gl-types/gl-enum.h>
#include <regen/utility/ref-ptr.h>
#include <regen/utility/stack.h>
#include <regen/utility/string-util.h>
#include <regen/math/matrix.h>
#include <regen/math/vector.h>

namespace regen {
	// default attribute names
#define ATTRIBUTE_NAME_POS "pos"
#define ATTRIBUTE_NAME_NOR "nor"
#define ATTRIBUTE_NAME_TAN "tan"
#define ATTRIBUTE_NAME_COL0 "col0"
#define ATTRIBUTE_NAME_COL1 "col1"

	/**
	 * Offset in the VBO
	 */
#ifndef BUFFER_OFFSET
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#endif

#ifndef byte
	typedef unsigned char byte;
#endif

	class Animation;

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
		 * @param dataType Specifies the data type of each component in the array.
		 * @param dataTypeBytes Size of a single instance of the data type in bytes.
		 * @param valsPerElement Specifies the number of components per generic vertex attribute.
		 * @param elementCount Number of array elements.
		 * @param normalize Specifies whether fixed-point data values should be normalized.
		 */
		ShaderInput(
				const std::string &name,
				GLenum dataType,
				GLuint dataTypeBytes,
				GLuint valsPerElement,
				GLsizei elementCount,
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
		GLboolean active() const;

		/**
		 * no call to glUniform when inactive.
		 * @param v the active toggle value
		 */
		void set_active(GLboolean v);

		/**
		 * Compare stamps to check if the input data changed.
		 */
		GLuint stamp() const;

		/**
		 * Sets a new stamp value.
		 */
		void nextStamp();

		/**
		 * Specifies the data type of each component in the array.
		 * Symbolic constants GL_FLOAT,GL_DOUBLE,.. accepted.
		 */
		GLenum dataType() const;

		/**
		 * Size of a single instance of the data type in bytes.
		 */
		GLuint dataTypeBytes() const;

		/**
		 * Specifies the byte offset between consecutive generic vertex attributes.
		 * If stride is 0, the generic vertex attributes are understood to be tightly
		 * packed in the array. The initial value is 0.
		 */
		void set_stride(GLuint stride);

		/**
		 * VBO that contains this vertex data.
		 * Iterator should be exclusively owned by this instance.
		 */
		void set_buffer(GLuint buffer, const VBOReference &ref);

		/**
		 * VBO that contains this vertex data.
		 */
		GLuint buffer() const;

		/**
		 * data with stamp was uploaded to GL.
		 */
		GLuint bufferStamp() const;

		/**
		 * Iterator to allocated VBO block.
		 */
		ref_ptr<VBO::Reference> bufferIterator();

		/**
		 * Specifies the byte offset between consecutive generic vertex attributes.
		 * If stride is 0, the generic vertex attributes are understood to be tightly
		 * packed in the array. The initial value is 0.
		 */
		GLuint stride() const;

		/**
		 * Attribute size for all vertices.
		 */
		GLuint inputSize() const;

		/**
		 * Attribute size for all vertices.
		 */
		void set_inputSize(GLuint size);

		/**
		 * Attribute size for a single vertex.
		 */
		GLuint elementSize() const;

		/**
		 * Offset in the VBO to the first
		 * attribute element.
		 */
		void set_offset(GLuint offset);

		/**
		 * Offset in the VBO to the first
		 * attribute element.
		 */
		GLuint offset() const;

		/**
		 * Number of array elements.
		 * returns 1 if this is not an array attribute.
		 */
		GLuint elementCount() const;

		/**
		 * Number of array elements.
		 * returns 1 if this is not an array attribute.
		 */
		void set_elementCount(GLuint);

		/**
		 * Specifies the number of components per generic vertex attribute.
		 * Must be 1, 2, 3, or 4.
		 */
		GLuint valsPerElement() const;

		/**
		 * Used for instanced attributes.
		 */
		GLuint numInstances() const;

		/**
		 * Specify the number of instances that will pass between updates
		 * of the generic attribute at slot index.
		 */
		GLuint divisor() const;

		/**
		 * Specifies whether fixed-point data values should be normalized (GL_TRUE)
		 * or converted directly as fixed-point values (GL_FALSE) when they are accessed.
		 */
		GLboolean normalize() const;

		/**
		 * @param transpose transpose the data.
		 */
		void set_transpose(GLboolean transpose);

		/**
		 * @return transpose the data.
		 */
		GLboolean transpose() const;

		/**
		 * @return the vertex count.
		 */
		GLuint numVertices() const;

		/**
		 * @param numVertices the vertex count.
		 */
		void set_numVertices(GLuint numVertices);

		/**
		 * Returns true if this input is a vertex attribute or
		 * an instanced attribute.
		 */
		GLboolean isVertexAttribute() const;

		/**
		 * Constants can not change the value during the lifetime
		 * of the shader program.
		 */
		void set_isConstant(GLboolean isConstant);

		/**
		 * Constants can not change the value during the lifetime
		 * of the shader program.
		 */
		GLboolean isConstant() const;

		auto isUniformBlock() const { return isUniformBlock_; }

		/**
		 * Uniforms with a single array element will appear
		 * with [1] in the generated shader if forceArray is true.
		 * Note: attributes can not be arrays.
		 */
		void set_forceArray(GLboolean forceArray);

		/**
		 * Uniforms with a single array element will appear
		 * with [1] in the generated shader if forceArray is true.
		 * Note: attributes can not be arrays.
		 */
		GLboolean forceArray() const;

		/**
		 * Allocates RAM for the attribute and does a memcpy
		 * if the data pointer is not null.
		 * numVertices*elementSize bytes will be allocated.
		 */
		void setVertexData(
				GLuint numVertices,
				const byte *vertexData = nullptr);

		void setArrayData(
				GLuint numArrayElements,
				const byte *arrayData = nullptr);

		/**
		 * Allocates RAM for the attribute and does a memcpy
		 * if the data pointer is not null.
		 * numInstances*elementSize/divisor bytes will be allocated.
		 */
		void setInstanceData(
				GLuint numInstances,
				GLuint divisor,
				const byte *instanceData = nullptr);

		/**
		 * @param data the input data.
		 */
		void setUniformDataUntyped(byte *data);

		/**
		 * Vertex data pointer.
		 * Returns pointer owned by this instance or the top of the
		 * data pointer stack.
		 */
		byte *clientDataPtr();

		/**
		 * Vertex data pointer.
		 * Returns pointer owned by this instance.
		 */
		byte *ownedClientData();

		/**
		 * Vertex data pointer.
		 * Returns pointer owned by this instance or the top of the
		 * data pointer stack.
		 */
		const byte *clientData() const;

		/**
		 * Pushes a data pointer onto the stack without doing a copy.
		 * Caller have to make sure the pointer stays valid until the data
		 * is pushed.
		 */
		void pushClientData(byte *data);

		/**
		 * Pop data pointer you previously pushed.
		 * This does not delete the data pointer, it's owned by caller.
		 * Last pop will reset to data pointer owned by this instance.
		 */
		void popClientData();

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
		void writeServerData(RenderState *rs, GLuint index);

		/**
		 * Write this attribute to the GL server.
		 * @param rs The RenderState.
		 */
		void writeServerData(RenderState *rs);

		/**
		 * Returns true if this attribute is allocated in RAM
		 * or if it was uploaded to GL already.
		 */
		GLboolean hasData();

		/**
		 * Returns true if this attribute is allocated in RAM.
		 */
		GLboolean hasClientData();

		/**
		 * Returns true if this attribute was uploaded to GL already.
		 */
		GLboolean hasServerData();

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
		 * Binds float uniform to the given shader location.
		 */
		void enableUniform1f(GLint loc) const;

		/**
		 * Binds vec2f uniform to the given shader location.
		 */
		void enableUniform2f(GLint loc) const;

		/**
		 * Binds vec3f uniform to the given shader location.
		 */
		void enableUniform3f(GLint loc) const;

		/**
		 * Binds vec4f uniform to the given shader location.
		 */
		void enableUniform4f(GLint loc) const;

		/**
		 * Binds int uniform to the given shader location.
		 */
		void enableUniform1i(GLint loc) const;

		/**
		 * Binds vec2i uniform to the given shader location.
		 */
		void enableUniform2i(GLint loc) const;

		/**
		 * Binds vec3i uniform to the given shader location.
		 */
		void enableUniform3i(GLint loc) const;

		/**
		 * Binds vec4i uniform to the given shader location.
		 */
		void enableUniform4i(GLint loc) const;

		/**
		 * Binds double uniform to the given shader location.
		 */
		void enableUniform1d(GLint loc) const;

		/**
		 * Binds vec2d uniform to the given shader location.
		 */
		void enableUniform2d(GLint loc) const;

		/**
		 * Binds vec3d uniform to the given shader location.
		 */
		void enableUniform3d(GLint loc) const;

		/**
		 * Binds vec4d uniform to the given shader location.
		 */
		void enableUniform4d(GLint loc) const;

		/**
		 * Binds unsigned int uniform to the given shader location.
		 */
		void enableUniform1ui(GLint loc) const;

		/**
		 * Binds vec2ui uniform to the given shader location.
		 */
		void enableUniform2ui(GLint loc) const;

		/**
		 * Binds vec3ui uniform to the given shader location.
		 */
		void enableUniform3ui(GLint loc) const;

		/**
		 * Binds vec4ui uniform to the given shader location.
		 */
		void enableUniform4ui(GLint loc) const;

		/**
		 * Binds mat3 uniform to the given shader location.
		 */
		void enableUniformMat3(GLint loc) const;

		/**
		 * Binds mat4 uniform to the given shader location.
		 */
		void enableUniformMat4(GLint loc) const;

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

	protected:
		std::string name_;
		GLenum dataType_;
		GLuint dataTypeBytes_;
		GLuint stride_;
		GLuint offset_;
		GLuint inputSize_;
		GLuint elementSize_;
		GLsizei elementCount_;
		GLuint numVertices_;
		GLuint numInstances_;
		GLsizei numElements_;
		GLuint valsPerElement_;
		GLuint divisor_;
		GLuint buffer_;
		GLuint bufferStamp_;
		ref_ptr<VBO::Reference> bufferIterator_;
		GLboolean normalize_;
		GLboolean isVertexAttribute_;
		GLboolean transpose_;
		byte *data_;
		Stack<byte *> dataStack_;
		GLuint stamp_;

		GLboolean isConstant_;
		GLboolean isUniformBlock_;
		GLboolean forceArray_;
		GLboolean active_;

		ref_ptr<Animation> dataUpload_;

		void (ShaderInput::*enableAttribute_)(GLint loc) const;

		//void (ShaderInput::*enableUniform_)(GLint loc) const;
		std::function<void(GLint)> enableUniform_;

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
		NamedShaderInput(const ref_ptr<ShaderInput> &in,
						 const std::string &name = "",
						 const std::string &type = "");

		/** the shader input data. */
		ref_ptr<ShaderInput> in_;
		/** the name overwrite. */
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
		 * @param elementCount Number of array elements.
		 * @param normalize Specifies whether fixed-point data values should be normalized.
		 */
		ShaderInputTyped(
				const std::string &name,
				GLuint elementCount,
				GLboolean normalize)
				: ShaderInput(name,
							  TypeValue,
							  sizeof(BaseType),
							  sizeof(ValueType) / sizeof(BaseType),
							  elementCount, normalize) {}

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
		 * Write ShaderInput.
		 */
		virtual std::ostream &operator>>(std::ostream &out) const { return out << *((ValueType *) data_); }

		void write(std::ostream &out) const override { out << *((ValueType *) data_); }

		/**
		 * Set a value for the active stack data.
		 * @param vertexIndex index in data array.
		 * @param val the new value.
		 */
		void setVertex(GLuint vertexIndex, const ValueType &val) {
			auto *v = (ValueType *) dataStack_.top();
			v[vertexIndex] = val;
			stamp_ += 1;
		}

		/**
		 * @param vertexIndex index in data array.
		 * @return data value at given index.
		 */
		const ValueType &getVertex(GLuint vertexIndex) const {
			auto *v = (ValueType *) dataStack_.top();
			return v[vertexIndex];
		}

		ValueType &getVertexPtr(GLuint vertexIndex) const {
			auto *v = (ValueType *) dataStack_.top();
			return v[vertexIndex];
		}

		/**
		 * @param data the uniforminput data.
		 */
		void setUniformData(const ValueType &data) { setUniformDataUntyped((byte *) &data); }

		/**
		 * @return the input data.
		 */
		const ValueType &uniformData() { return getVertex(0); }
	};

	/////////////

	/**
	 * \brief Provides 1D float input to shader programs.
	 */
	class ShaderInput1f : public ShaderInputTyped<GLfloat, GLfloat, GL_FLOAT> {
	public:
		/**
		 * @param name the input name.
		 * @param elementCount number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput1f(
				const std::string &name,
				GLuint elementCount = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 2D float input to shader programs.
	 */
	class ShaderInput2f : public ShaderInputTyped<Vec2f, GLfloat, GL_FLOAT> {
	public:
		/**
		 * @param name the input name.
		 * @param elementCount number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput2f(
				const std::string &name,
				GLuint elementCount = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 3D float input to shader programs.
	 */
	class ShaderInput3f : public ShaderInputTyped<Vec3f, GLfloat, GL_FLOAT> {
	public:
		/**
		 * @param name the input name.
		 * @param elementCount number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput3f(
				const std::string &name,
				GLuint elementCount = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 4D float input to shader programs.
	 */
	class ShaderInput4f : public ShaderInputTyped<Vec4f, GLfloat, GL_FLOAT> {
	public:
		/**
		 * @param name the input name.
		 * @param elementCount number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput4f(
				const std::string &name,
				GLuint elementCount = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 3x3 matrix input to shader programs.
	 */
	class ShaderInputMat3 : public ShaderInputTyped<Mat3f, GLfloat, GL_FLOAT> {
	public:
		/**
		 * @param name the input name.
		 * @param elementCount number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInputMat3(
				const std::string &name,
				GLuint elementCount = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 4x4 matrix input to shader programs.
	 */
	class ShaderInputMat4 : public ShaderInputTyped<Mat4f, GLfloat, GL_FLOAT> {
	public:
		/**
		 * @param name the input name.
		 * @param elementCount number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInputMat4(
				const std::string &name,
				GLuint elementCount = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 1D double input to shader programs.
	 */
	class ShaderInput1d : public ShaderInputTyped<GLdouble, GLdouble, GL_DOUBLE> {
	public:
		/**
		 * @param name the input name.
		 * @param elementCount number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput1d(
				const std::string &name,
				GLuint elementCount = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 2D double input to shader programs.
	 */
	class ShaderInput2d : public ShaderInputTyped<Vec2d, GLdouble, GL_DOUBLE> {
	public:
		/**
		 * @param name the input name.
		 * @param elementCount number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput2d(
				const std::string &name,
				GLuint elementCount = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 3D double input to shader programs.
	 */
	class ShaderInput3d : public ShaderInputTyped<Vec3d, GLdouble, GL_DOUBLE> {
	public:
		/**
		 * @param name the input name.
		 * @param elementCount number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput3d(
				const std::string &name,
				GLuint elementCount = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 4D double input to shader programs.
	 */
	class ShaderInput4d : public ShaderInputTyped<Vec4d, GLdouble, GL_DOUBLE> {
	public:
		/**
		 * @param name the input name.
		 * @param elementCount number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput4d(
				const std::string &name,
				GLuint elementCount = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 1D int input to shader programs.
	 */
	class ShaderInput1i : public ShaderInputTyped<GLint, GLint, GL_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param elementCount number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput1i(
				const std::string &name,
				GLuint elementCount = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 2D int input to shader programs.
	 */
	class ShaderInput2i : public ShaderInputTyped<Vec2i, GLint, GL_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param elementCount number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput2i(
				const std::string &name,
				GLuint elementCount = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 3D int input to shader programs.
	 */
	class ShaderInput3i : public ShaderInputTyped<Vec3i, GLint, GL_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param elementCount number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput3i(
				const std::string &name,
				GLuint elementCount = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 4D int input to shader programs.
	 */
	class ShaderInput4i : public ShaderInputTyped<Vec4i, GLint, GL_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param elementCount number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput4i(
				const std::string &name,
				GLuint elementCount = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 1D unsigned int input to shader programs.
	 */
	class ShaderInput1ui : public ShaderInputTyped<GLuint, GLuint, GL_UNSIGNED_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param elementCount number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput1ui(
				const std::string &name,
				GLuint elementCount = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 2D unsigned int input to shader programs.
	 */
	class ShaderInput2ui : public ShaderInputTyped<Vec2ui, GLuint, GL_UNSIGNED_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param elementCount number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput2ui(
				const std::string &name,
				GLuint elementCount = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 3D unsigned int input to shader programs.
	 */
	class ShaderInput3ui : public ShaderInputTyped<Vec3ui, GLuint, GL_UNSIGNED_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param elementCount number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput3ui(
				const std::string &name,
				GLuint elementCount = 1,
				GLboolean normalize = GL_FALSE);
	};

	/**
	 * \brief Provides 4D unsigned int input to shader programs.
	 */
	class ShaderInput4ui : public ShaderInputTyped<Vec4ui, GLuint, GL_UNSIGNED_INT> {
	public:
		/**
		 * @param name the input name.
		 * @param elementCount number of input elements.
		 * @param normalize should the input be normalized ?
		 */
		explicit ShaderInput4ui(
				const std::string &name,
				GLuint elementCount = 1,
				GLboolean normalize = GL_FALSE);
	};

	class UBO;

	/**
	 * \brief Provides input data via a Uniform Buffer Object (UBO).
	 */
	class UniformBlock : public ShaderInput {
	public:
		/**
		 * @param name the uniform block name.
		 */
		explicit UniformBlock(const std::string &name);

		UniformBlock(const std::string &name, const ref_ptr<UBO> &ubo);

		~UniformBlock() override;

		UniformBlock(const UniformBlock &) = delete;

		/**
		 * @return the list of uniforms.
		 */
		const std::vector<NamedShaderInput> &uniforms() const;

		/**
		 * @param input the uniform to add.
		 */
		void addUniform(const ref_ptr<ShaderInput> &input, const std::string &name = "");

		/**
		 * Binds the uniform block to the given shader location.
		 */
		void enableUniformBlock(GLint loc) const;

		void write(std::ostream &out) const override;

	protected:
		struct UniformBlockData;
		UniformBlockData *priv_;
	};
} // namespace

namespace regen {
	/**
	 * \brief Vertex Array Objects (VAO) are OpenGL Objects that store the
	 * set of bindings between Vertex Attributes and the user's source vertex data.
	 *
	 * VBOs store the actual vertex and index arrays,
	 * while VAOs store the settings for interpreting the data in those arrays.
	 *
	 * The currently bound vertex array object is used for all commands
	 * which modify vertex array state, such as VertexAttribPointer and
	 * EnableVertexAttribArray; all commands which draw from vertex arrays,
	 * such as DrawArrays and DrawElements; and all queries of vertex
	 * array state.
	 */
	class VAO : public GLObject {
	public:
		VAO();

		/**
		 * Clear the VAO state.
		 */
		void resetGL();
	};
} // namespace

#endif /* SHADER_INPUT_H_ */
