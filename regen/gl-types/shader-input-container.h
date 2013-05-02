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
  enum DataLayout {
    INTERLEAVED, SEQUENTIAL, LAYOUT_LAST
  };

  /**
   * @param usage VBO usage.
   */
  ShaderInputContainer(
      VertexBufferObject::Usage usage=VertexBufferObject::USAGE_DYNAMIC);
  /**
   * @param in shader input data.
   * @param name shader input name overwrite.
   * @param usage VBO usage.
   */
  ShaderInputContainer(const ref_ptr<ShaderInput> &in, const string &name="",
      VertexBufferObject::Usage usage=VertexBufferObject::USAGE_DYNAMIC);
  ~ShaderInputContainer();

  /**
   * @return VBO that manages the vertex array data.
   */
  const ref_ptr<VertexBufferObject>& inputBuffer() const;

  /**
   * @return Number of vertices of added input data.
   */
  GLuint numVertices() const;
  void set_numVertices(GLuint v);
  /**
   * @return Number of instances of added input data.
   */
  GLuint numInstances() const;

  /**
   * @return Previously added shader inputs.
   */
  const ShaderInputList& inputs() const;

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

  void beginUpload(DataLayout layout);
  void endUpload();

  /**
   * @param in the shader input data.
   * @param name the shader input name.
   * @return iterator of data container
   */
  ShaderInputList::const_iterator setInput(const ref_ptr<ShaderInput> &in, const string &name="");

  void removeInput(const ref_ptr<ShaderInput> &att);

  /**
   * Sets the index attribute.
   * @param indices the index attribute.
   * @param maxIndex maximal index in the index array.
   */
  void setIndices(const ref_ptr<VertexAttribute> &indices, GLuint maxIndex);
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
  const ref_ptr<VertexAttribute>& indices() const;
  /**
   * @return index buffer used by this mesh.
   */
  GLuint indexBuffer() const;

protected:
  ShaderInputList inputs_;
  set<string> inputMap_;
  GLuint numVertices_;
  GLuint numInstances_;
  GLuint numIndices_;
  GLuint maxIndex_;
  ref_ptr<VertexAttribute> indices_;

  list< ref_ptr<VertexAttribute> > uploadInputs_;
  DataLayout uploadLayout_;

  ref_ptr<VertexBufferObject> inputBuffer_;

  void removeInput(const string &name);
};

class HasInput {
public:
  HasInput(VertexBufferObject::Usage usage)
  { inputContainer_ = ref_ptr<ShaderInputContainer>::manage(new ShaderInputContainer(usage)); }
  HasInput(const ref_ptr<ShaderInputContainer> &inputs)
  { inputContainer_ = inputs; }

  const ref_ptr<ShaderInputContainer>& inputContainer() const
  { return inputContainer_; }
  void set_inputContainer(const ref_ptr<ShaderInputContainer> &inputContainer)
  { inputContainer_ = inputContainer; }

  ShaderInputList::const_iterator setInput(const ref_ptr<ShaderInput> &in, const string &name="")
  { return inputContainer_->setInput(in, name); }
  void setIndices(const ref_ptr<VertexAttribute> &in, GLuint maxIndex)
  { inputContainer_->setIndices(in, maxIndex); }

protected:
  ref_ptr<ShaderInputContainer> inputContainer_;
};

} // namespace

#endif /* ATTRIBUTE_STATE_H_ */
