/*
 * shader-input-container.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef SHADER_INPUT_CONTAINER_H_
#define SHADER_INPUT_CONTAINER_H_

#include <regen/gl-types/shader-input.h>
#include <regen/gl-types/vbo.h>

#include <set>

namespace regen {
	/**
	 * \brief Container for shader input data.
	 */
	class ShaderInputContainer {
	public:
		/**
		 * \brief Vertex array data layout.
		 */
		enum DataLayout {
			INTERLEAVED, SEQUENTIAL, LAYOUT_LAST
		};

		/**
		 * @param usage VBO usage.
		 */
		explicit ShaderInputContainer(
				VBO::Usage usage = VBO::USAGE_DYNAMIC);

		/**
		 * @param in shader input data.
		 * @param name shader input name overwrite.
		 * @param usage VBO usage.
		 */
		explicit ShaderInputContainer(const ref_ptr<ShaderInput> &in, const std::string &name = "",
							 VBO::Usage usage = VBO::USAGE_DYNAMIC);

		~ShaderInputContainer();

		/**
		 * @return VBO that manages the vertex array data.
		 */
		auto &inputBuffer() const { return inputBuffer_; }

		/**
		 * @return Specifies the number of vertices to be rendered.
		 */
		auto numVertices() const { return numVertices_; }

		/**
		 * @param v Specifies the number of vertices to be rendered.
		 */
		void set_numVertices(GLuint v) { numVertices_ = v; }

		/**
		 * @return Specifies the number of vertices to be rendered.
		 */
		void set_vertexOffset(GLuint v) { vertexOffset_ = v; }

		/**
		 * @return Specifies the number of vertices to be rendered.
		 */
		auto vertexOffset() const { return vertexOffset_; }

		/**
		 * @return Specifies the number of indices to be rendered.
		 */
		void set_numIndices(GLuint v) { numIndices_ = v; }

		/**
		 * @return Specifies the offset to the index buffer in bytes.
		 */
		void set_indexOffset(GLuint v);

		/**
		 * @return Number of instances of added input data.
		 */
		auto numInstances() const { return numInstances_; }

		/**
		 * @param v Specifies the number of instances to be rendered.
		 */
		void set_numInstances(GLuint v) { numInstances_ = v; }

		/**
		 * @param layout Start recording added inputs.
		 */
		void begin(DataLayout layout);

		/**
		 * Finish previous call to begin(). All recorded inputs are
		 * uploaded to VBO memory.
		 */
		VBOReference end();

		/**
		 * @return Previously added shader inputs.
		 */
		auto &inputs() const { return inputs_; }

		/**
		 * @return inputs recorded during begin() and end().
		 */
		auto &uploadInputs() const { return uploadInputs_; }

		/**
		 * @param name the shader input name.
		 * @return true if an input data with given name was added before.
		 */
		GLboolean hasInput(const std::string &name) const;

		/**
		 * @param name the shader input name.
		 * @return input data with specified name.
		 */
		ref_ptr<ShaderInput> getInput(const std::string &name) const;

		/**
		 * @param in the shader input data.
		 * @param name the shader input name.
		 * @return iterator of data container
		 */
		ShaderInputList::const_iterator setInput(const ref_ptr<ShaderInput> &in, const std::string &name = "");

		/**
		 * Remove previously added shader input.
		 */
		void removeInput(const ref_ptr<ShaderInput> &att);

		/**
		 * Sets the index attribute.
		 * @param indices the index attribute.
		 * @param maxIndex maximal index in the index array.
		 */
		ref_ptr<VBO::Reference> setIndices(const ref_ptr<ShaderInput> &indices, GLuint maxIndex);

		/**
		 * @return number of indices to vertex data.
		 */
		auto numIndices() const { return numIndices_; }

		/**
		 * @return the maximal index in the index buffer.
		 */
		auto maxIndex() const { return maxIndex_; }

		/**
		 * @return indexes to the vertex data of this primitive set.
		 */
		auto &indices() const { return indices_; }

		/**
		 * @return index buffer used by this mesh.
		 */
		GLuint indexBuffer() const;

		/**
		 * render primitives from array data.
		 * @param primitive Specifies what kind of primitives to render.
		 */
		void drawArrays(GLenum primitive);

		/**
		 * draw multiple instances of a range of elements.
		 * @param primitive Specifies what kind of primitives to render.
		 */
		void drawArraysInstanced(GLenum primitive);

		/**
		 * render primitives from array data.
		 * @param primitive Specifies what kind of primitives to render.
		 */
		void drawElements(GLenum primitive);

		/**
		 * draw multiple instances of a set of elements.
		 * @param primitive Specifies what kind of primitives to render.
		 */
		void drawElementsInstanced(GLenum primitive);

	protected:
		ShaderInputList inputs_;
		std::set<std::string> inputMap_;
		GLint numVertices_;
		GLint vertexOffset_;
		GLint numInstances_;
		GLint numIndices_;
		GLuint maxIndex_;
		ref_ptr<ShaderInput> indices_;

		ShaderInputList uploadInputs_;
		std::list<ref_ptr<ShaderInput> > uploadAttributes_;
		DataLayout uploadLayout_;

		ref_ptr<VBO> inputBuffer_;

		void removeInput(const std::string &name);
	};

	/**
	 * \brief Interface for State's with input.
	 */
	class HasInput {
	public:
		/**
		 * @param usage VBO usage hint.
		 */
		explicit HasInput(VBO::Usage usage) { inputContainer_ = ref_ptr<ShaderInputContainer>::alloc(usage); }

		/**
		 * @param inputs custom input container.
		 */
		explicit HasInput(const ref_ptr<ShaderInputContainer> &inputs) { inputContainer_ = inputs; }

		virtual ~HasInput() = default;

		/**
		 * Begin recording ShaderInput's.
		 * @param layout Start recording added inputs.
		 */
		void begin(ShaderInputContainer::DataLayout layout) { inputContainer_->begin(layout); }

		/**
		 * Finish previous call to begin(). All recorded inputs are
		 * uploaded to VBO memory.
		 */
		VBOReference end() { return inputContainer_->end(); }

		/**
		 * @return the input container.
		 */
		const ref_ptr<ShaderInputContainer> &inputContainer() const { return inputContainer_; }

		/**
		 * @param inputContainer the input container.
		 */
		void
		set_inputContainer(const ref_ptr<ShaderInputContainer> &inputContainer) { inputContainer_ = inputContainer; }

		/**
		 * Adds shader input to the input container.
		 * @param in shader input
		 * @param name name override
		 * @return iterator in input container.
		 */
		ShaderInputList::const_iterator setInput(
				const ref_ptr<ShaderInput> &in, const std::string &name = "") {
			return inputContainer_->setInput(in, name);
		}

		/**
		 * Sets the index data.
		 * @param in index data input.
		 * @param maxIndex max index in index array.
		 */
		ref_ptr<VBO::Reference> setIndices(const ref_ptr<ShaderInput> &in, GLuint maxIndex) {
			return inputContainer_->setIndices(in, maxIndex);
		}

	protected:
		ref_ptr<ShaderInputContainer> inputContainer_;
	};
} // namespace

#endif /* SHADER_INPUT_CONTAINER_H_ */
