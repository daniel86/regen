
#ifndef TEXTURE_UPDATER_H_
#define TEXTURE_UPDATER_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <map>
using namespace std;

#include <ogle/animations/animation.h>
#include <ogle/textures/texture-update-operation.h>
#include <ogle/gl-types/fbo.h>

#include <ogle/algebra/vector.h>
#include <ogle/meshes/mesh-state.h>
#include <ogle/utility/xml.h>

namespace ogle {
/**
 * \brief Executes a sequence of operations for updating a
 * Texture.
 */
class TextureUpdater : public Animation
{
public:
  /**
   * Sequence of texture update operations.
   */
  typedef list< ref_ptr<TextureUpdateOperation> > OperationList;

  TextureUpdater();

  void operator>>(rapidxml::xml_node<> *doc);

  /**
   * The desired animation framerate.
   */
  GLint framerate() const;
  /**
   * The desired animation framerate.
   */
  void set_framerate(GLint framerate);

  //////////

  /**
   * Add a named buffer to the list of known buffers.
   */
  void addBuffer(const ref_ptr<FrameBufferObject> &buffer, const string &name);
  /**
   * Get buffer by name.
   */
  ref_ptr<FrameBufferObject> getBuffer(const string &name);

  //////////

  /**
   * Adds an operation to the sequence of operations
   * to be executed.
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
   * Execute sequence of operations.
   */
  void executeOperations(RenderState *rs, const OperationList &operations);

  // override
  void glAnimate(RenderState *rs, GLdouble dt);

protected:
  GLdouble dt_;
  GLint framerate_;

  OperationList operations_;
  OperationList initialOperations_;
  map<string, ref_ptr<FrameBufferObject> > buffers_;
};

} // namespace

#endif /* TEXTURE_UPDATER_H_ */
