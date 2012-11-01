
#ifndef TEXTURE_UPDATE_OPERATION_H_
#define TEXTURE_UPDATE_OPERATION_H_

#include <string>
#include <map>
using namespace std;

#include <ogle/gl-types/shader.h>
#include <ogle/states/state.h>
#include <ogle/states/mesh-state.h>
#include <ogle/states/texture-state.h>

#include <ogle/gl-types/texture-buffer.h>

/**
 * TextureUpdateOperation are using an associated shader program
 * to compute texture animations on the GPU.
 * Each operation has a set of associated shader inputs, an output buffer
 * and a set of general configurations that are used by each individual
 * operation.
 */
class TextureUpdateOperation : public State
{
public:
  struct PositionedTextureBuffer {
    GLint loc;
    TextureBuffer *buffer;
    string nameInShader;
  };

  /**
   * Sets up operation and tries to load a shader
   * program from the default shader resource file.
   */
  TextureUpdateOperation(
      TextureBuffer *outputBuffer,
      MeshState *textureQuad,
      const map<string,string> &operationConfig,
      const map<string,string> &shaderConfig);

  /**
   * Apply configuration specified in key-value pair.
   */
  void parseConfig(const map<string,string> &cfg);

  /**
   * Effect key of fragment shader.
   */
  string fsName();
  map<GLenum,string>& shaderNames();
  map<string,string>& shaderConfig();

  /**
   * The operation shader program.
   * NULL if no shader loaded yet.
   */
  Shader* shader();

  /**
   * Activates blending before execution.
   */
  void set_blendMode(BlendMode blendMode);
  /**
   * Activates blending before execution.
   */
  const BlendMode& blendMode() const;

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
  const Vec4f& clearColor() const;

  /**
   * Adds a sampler that is used in the shader program.
   */
  void addInputBuffer(TextureBuffer *buffer, GLint loc, const string &nameInShader);
  list<PositionedTextureBuffer>& inputBuffer();

  /**
   * Sets the render target.
   */
  void set_outputBuffer(TextureBuffer *outputBuffer);
  /**
   * The render target.
   */
  TextureBuffer* outputBuffer();

  /**
   * Draw to output buffer.
   */
  void updateTexture(RenderState *rs, GLint lastShaderID);

  virtual const string& name() const;

protected:
  MeshState *textureQuad_;

  ref_ptr<Shader> shader_;
  map<string,string> shaderConfig_;
  map<GLenum,string> shaderNames_;
  ref_ptr<ShaderInput> posInput_;
  GLuint posLoc_;

  BlendMode blendMode_;
  ref_ptr<State> blendState_;

  TextureBuffer *outputBuffer_;
  Texture *outputTexture_;

  ref_ptr<State> swapState_;

  list<PositionedTextureBuffer> inputBuffer_;

  GLboolean clear_;
  Vec4f clearColor_;

  GLuint numIterations_;
  GLuint numInstances_;
};

#endif /* TEXTURE_UPDATE_OPERATION_H_ */
