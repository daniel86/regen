
#ifndef TEXTURE_UPDATER_H_
#define TEXTURE_UPDATER_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <map>
using namespace std;

#include <ogle/animations/animation.h>
#include <ogle/gl-types/fbo.h>
#include <ogle/algebra/vector.h>
#include <ogle/meshes/mesh-state.h>
#include <ogle/states/fbo-state.h>
#include <ogle/states/shader-state.h>

namespace ogle {
/**
 * \brief A simple GPU operation that updates a Texture.
 *
 * Each operation has a set of associated shader inputs, an output buffer
 * and a set of general configurations that are used by each individual
 * operation.
 */
class TextureUpdateOperation : public State
{
public:
  /**
   * @param outputBuffer the render target.
   */
  TextureUpdateOperation(const ref_ptr<FrameBufferObject> &outputBuffer);
  /**
   * @param cfg shader configuration
   * @param key include key
   */
  void createShader(const ShaderState::Config &cfg, const string &key);

  /**
   * @param blendMode Describes how a texture will be mixed with existing pixels.
   */
  void set_blendMode(BlendMode blendMode);
  /**
   * @param numIterations number of operation iterations per call.
   */
  void set_numIterations(GLuint numIterations);
  /**
   * Used to clear the render target before the operation is executed.
   * @param clearColor the clear color.
   */
  void set_clearColor(const Vec4f &clearColor);
  /**
   * @param buffer operation input buffer.
   * @param nameInShader input name in shader.
   */
  void addInputBuffer(const ref_ptr<FrameBufferObject> &buffer, const string &nameInShader);

  /**
   * @return render target FrameBufferObject.
   */
  const ref_ptr<FrameBufferObject>& outputBuffer();
  /**
   * @return render target Texture.
   */
  const ref_ptr<Texture>& outputTexture();
  /**
   * @return the operation shader program.
   */
  const ref_ptr<Shader>& shader();

  /**
   * Execute this operation.
   * @param rs the render state.
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
};
} // namespace

namespace ogle {
/**
 * \brief Simple framework for updating Texture's using a sequence of TextureUpdateroperation's.
 */
class TextureUpdater : public Animation
{
public:
  /**
   * Sequence of texture update operations.
   */
  typedef list< ref_ptr<TextureUpdateOperation> > OperationList;

  TextureUpdater();
  /**
   * Read from XML file.
   * @param xmlFile the XML file.
   */
  void operator>>(const string &xmlFile);

  /**
   * @param framerate texture update framerate.
   */
  void set_framerate(GLint framerate);

  //////////

  /**
   */
  /**
   * Adds an operation to the sequence of operations
   * to be executed.
   * @param operation the update operation.
   * @param isInitial initial operations are only executed once.
   */
  void addOperation(const ref_ptr<TextureUpdateOperation> &operation, GLboolean isInitial=GL_FALSE);
  /**
   * Removes an previously added operation.
   */
  void removeOperation(TextureUpdateOperation *operation);
  /**
   * @return sequence of initial operations.
   */
  const OperationList& initialOperations();
  /**
   * @return sequence of operations.
   */
  const OperationList& operations();

  /**
   * @return last render target Texture.
   */
  ref_ptr<Texture> outputTexture();
  /**
   * @return last render target FrameBufferObject.
   */
  ref_ptr<FrameBufferObject> outputBuffer();

  /**
   * Execute sequence of operations.
   * @param rs the render state.
   * @param operations the sequence of operations to execute.
   */
  void executeOperations(RenderState *rs, const OperationList &operations);

  // override
  void glAnimate(RenderState *rs, GLdouble dt);

protected:
  GLdouble dt_;
  GLint framerate_;

  OperationList operations_;
  OperationList initialOperations_;
};
} // namespace

#endif /* TEXTURE_UPDATER_H_ */
