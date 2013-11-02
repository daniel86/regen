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
using namespace std;

namespace regen {
  /**
   * \brief ShaderInput plus optional name overwrite.
   */
  struct NamedShaderInput {
    /**
     * @param in the shader input data.
     * @param name the name overwrite.
     */
    NamedShaderInput(ref_ptr<ShaderInput> in, const string &name)
    : in_(in), name_(name) {}
    /** the shader input data. */
    ref_ptr<ShaderInput> in_;
    /** the name overwrite. */
    string name_;
  };
  /**
   * ShaderInput container.
   */
  typedef list<NamedShaderInput> ShaderInputList;

  /**
   * \brief Container for shader input data.
   */
  class ShaderInputContainer
  {
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
    ShaderInputContainer(
        VBO::Usage usage=VBO::USAGE_DYNAMIC);
    /**
     * @param in shader input data.
     * @param name shader input name overwrite.
     * @param usage VBO usage.
     */
    ShaderInputContainer(const ref_ptr<ShaderInput> &in, const string &name="",
        VBO::Usage usage=VBO::USAGE_DYNAMIC);
    ~ShaderInputContainer();

    /**
     * @return VBO that manages the vertex array data.
     */
    const ref_ptr<VBO>& inputBuffer() const;

    /**
     * @return Specifies the number of vertices to be rendered.
     */
    GLuint numVertices() const;
    /**
     * @param v Specifies the number of vertices to be rendered.
     */
    void set_numVertices(GLuint v);
    /**
     * @return Number of instances of added input data.
     */
    GLuint numInstances() const;
    /**
     * @param v Specifies the number of instances to be rendered.
     */
    void set_numInstances(GLuint v);

    /**
     * @param layout Start recording added inputs.
     */
    void begin(DataLayout layout);
    /**
     * Finish previous call to begin(). All recorded inputs are
     * uploaded to VBO memory.
     */
    void end();

    /**
     * @return Previously added shader inputs.
     */
    const ShaderInputList& inputs() const;
    /**
     * @return inputs recorded during begin() and end().
     */
    const ShaderInputList& uploadInputs() const;

    /**
     * @param name the shader input name.
     * @return true if an input data with given name was added before.
     */
    GLboolean hasInput(const string &name) const;

    /**
     * @param name the shader input name.
     * @return input data with specified name.
     */
    ref_ptr<ShaderInput> getInput(const string &name) const;

    /**
     * @param in the shader input data.
     * @param name the shader input name.
     * @return iterator of data container
     */
    ShaderInputList::const_iterator setInput(const ref_ptr<ShaderInput> &in, const string &name="");
    /**
     * Remove previously added shader input.
     */
    void removeInput(const ref_ptr<ShaderInput> &att);

    /**
     * Sets the index attribute.
     * @param indices the index attribute.
     * @param maxIndex maximal index in the index array.
     */
    void setIndices(const ref_ptr<ShaderInput> &indices, GLuint maxIndex);
    /**
     * @return number of indices to vertex data.
     */
    GLuint numIndices() const;
    /**
     * @return the maximal index in the index buffer.
     */
    GLuint maxIndex();
    /**
     * @return indexes to the vertex data of this primitive set.
     */
    const ref_ptr<ShaderInput>& indices() const;
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
    set<string> inputMap_;
    GLuint numVertices_;
    GLuint numInstances_;
    GLuint numIndices_;
    GLuint maxIndex_;
    ref_ptr<ShaderInput> indices_;

    ShaderInputList uploadInputs_;
    list< ref_ptr<ShaderInput> > uploadAttributes_;
    DataLayout uploadLayout_;

    ref_ptr<VBO> inputBuffer_;

    void removeInput(const string &name);
  };

  /**
   * \brief Interface for State's with input.
   */
  class HasInput {
  public:
    /**
     * @param usage VBO usage hint.
     */
    HasInput(VBO::Usage usage)
    { inputContainer_ = ref_ptr<ShaderInputContainer>::alloc(usage); }
    /**
     * @param inputs custom input container.
     */
    HasInput(const ref_ptr<ShaderInputContainer> &inputs)
    { inputContainer_ = inputs; }
    virtual ~HasInput() {}

    /**
     * @return the input container.
     */
    const ref_ptr<ShaderInputContainer>& inputContainer() const
    { return inputContainer_; }
    /**
     * @param inputContainer the input container.
     */
    void set_inputContainer(const ref_ptr<ShaderInputContainer> &inputContainer)
    { inputContainer_ = inputContainer; }

    /**
     * Adds shader input to the input container.
     * @param in shader input
     * @param name name override
     * @return iterator in input container.
     */
    virtual ShaderInputList::const_iterator setInput(
        const ref_ptr<ShaderInput> &in, const string &name="")
    { return inputContainer_->setInput(in, name); }
    /**
     * Sets the index data.
     * @param in index data input.
     * @param maxIndex max index in index array.
     */
    void setIndices(const ref_ptr<ShaderInput> &in, GLuint maxIndex)
    { inputContainer_->setIndices(in, maxIndex); }

  protected:
    ref_ptr<ShaderInputContainer> inputContainer_;
  };
} // namespace

#endif /* ATTRIBUTE_STATE_H_ */
