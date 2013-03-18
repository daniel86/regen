
#ifndef TEXTURE_UPDATE_OPERATION_H_
#define TEXTURE_UPDATE_OPERATION_H_

#include <string>
#include <map>
using namespace std;

#include <ogle/states/texture-state.h>
#include <ogle/states/fbo-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/utility/xml.h>

namespace ogle {
/**
 * \brief Compute texture animations on the GPU.
 *
 * Each operation has a set of associated shader inputs, an output buffer
 * and a set of general configurations that are used by each individual
 * operation.
 */
class TextureUpdateOperation : public State
{
public:
  /**
   * Sets up operation and tries to load a shader
   * program from the default shader resource file.
   */
  TextureUpdateOperation(const ref_ptr<FrameBufferObject> &outputBuffer);

  void operator>>(rapidxml::xml_node<> *node);

  void createShader(const ShaderState::Config &cfg, const string &key);

  /**
   * Activates blending before execution.
   */
  void set_blendMode(BlendMode blendMode);

  /**
   * Number of executions per frame.
   */
  void set_numIterations(GLuint numIterations);

  /**
   * Clear the buffer texture before execution.
   */
  void set_clearColor(const Vec4f &clearColor);

  /**
   * Adds a sampler that is used in the shader program.
   */
  void addInputBuffer(const ref_ptr<FrameBufferObject> &buffer, const string &nameInShader);

  /**
   * The render target.
   */
  const ref_ptr<FrameBufferObject>& outputBuffer();
  /**
   * The operation shader program.
   * NULL if no shader loaded yet.
   */
  const ref_ptr<Shader>& shader();

  /**
   * Draw to output buffer.
   */
  void executeOperation(RenderState *rs);

protected:
  struct TextureBuffer {
    GLint loc;
    GLint channel;
    ref_ptr<FrameBufferObject> buffer;
    string nameInShader;
  };

  ref_ptr<Mesh> textureQuad_;
  ref_ptr<ShaderState> shader_;

  list<TextureBuffer> inputBuffer_;
  ref_ptr<FBOState> outputBuffer_;
  ref_ptr<Texture> outputTexture_;

  GLuint numIterations_;
  GLuint numInstances_;

  BlendMode blendMode_;
  ref_ptr<State> blendState_;

  ref_ptr<State> swapState_;
};

} // end ogle namespace

#endif /* TEXTURE_UPDATE_OPERATION_H_ */
