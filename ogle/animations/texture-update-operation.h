
#ifndef TEXTURE_UPDATE_OPERATION_H_
#define TEXTURE_UPDATE_OPERATION_H_

#include <string>
#include <map>
using namespace std;

#include <ogle/gl-types/shader.h>
#include <ogle/states/state.h>
#include <ogle/states/mesh-state.h>

#include <ogle/gl-types/texture-buffer.h>

/**
 * TextureUpdateOperation are using an associated shader program
 * to compute fluid simulations on the GPU.
 * Each operation has a set of associated shader inputs, an output buffer
 * and a set of general configurations that are used by each individual
 * operation.
 */
class TextureUpdateOperation : public State
{
public:
  /**
   * Defines how to handle the output buffer.
   */
  enum Mode {
    MODIFY_STATE,
    NEW_STATE
  };

  /**
   * Sets up operation and tries to load a shader
   * program from the default shader resource file.
   */
  TextureUpdateOperation(
      map<GLenum,string> shaderNames,
      TextureBuffer *outputBuffer,
      MeshState *textureQuad,
      map<string,string> &shaderConfig);

  string fsName();

  /**
   * The operation shader program.
   * NULL if no shader loaded yet.
   */
  Shader* shader();

  /**
   * Defines how to handle the output buffer.
   */
  void set_mode(Mode mode);
  /**
   * Defines how to handle the output buffer.
   */
  Mode mode() const;

  /**
   * Activates blending before execution.
   */
  void set_blendMode(TextureBlendMode blendMode);
  /**
   * Activates blending before execution.
   */
  TextureBlendMode blendMode() const;

  /**
   * Number of executions per frame.
   */
  void set_numIterations(GLuint numIterations);
  /**
   * Number of executions per frame.
   */
  GLuint numIterations() const;

  /**
   * Clear the buffer texture before execution.
   */
  void set_clearColor(const Vec4f &clearColor);
  /**
   * Clear the buffer texture before execution.
   */
  GLboolean clear() const;

  void addInputBuffer(TextureBuffer *buffer, GLint loc);

  void set_outputBuffer(TextureBuffer *outputBuffer);
  TextureBuffer* outputBuffer();

  void execute(RenderState *rs, GLint lastShaderID);

protected:
  map<GLenum,string> shaderNames_;

  MeshState *textureQuad_;
  ref_ptr<ShaderInput> posInput_;

  ref_ptr<Shader> shader_;
  map<string,string> shaderConfig_;
  GLuint posLoc_;

  Mode mode_;
  TextureBlendMode blendMode_;
  GLuint numIterations_;

  GLboolean clear_;
  Vec4f clearColor_;

  GLuint numInstances_;

  TextureBuffer *outputBuffer_;
  Texture *outputTexture_;
  struct PositionedTextureBuffer {
    GLint loc;
    TextureBuffer *buffer;
  };
  list<PositionedTextureBuffer> inputBuffer_;

  ref_ptr<State> blendState_;
  ref_ptr<State> operationModeState_;
};

#endif /* TEXTURE_UPDATE_OPERATION_H_ */
