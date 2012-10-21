
#ifndef TEXTURE_UPDATER_H_
#define TEXTURE_UPDATER_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <map>
using namespace std;

#include <ogle/animations/animation.h>
#include <ogle/animations/texture-update-operation.h>
#include <ogle/gl-types/texture-buffer.h>

#include <ogle/algebra/vector.h>
#include <ogle/states/mesh-state.h>


class TextureUpdater : public Animation
{
public:
  static TextureUpdater* readFromXML(MeshState *textureQuad, const string &xmlFile);

  TextureUpdater(const string &name);
  ~TextureUpdater();

  const string& name();

  GLint framerate();
  void set_framerate(GLint framerate);

  /**
   * A quad used for updating textures.
   */
  MeshState *textureQuad();
  /**
   * A quad used for updating textures.
   */
  void set_textureQuad(MeshState*);

  //////////

  /**
   * Add a named buffer to the list of known buffers.
   */
  void addBuffer(TextureBuffer *buffer);
  /**
   * Get buffer by name.
   */
  TextureBuffer* getBuffer(const string &name);

  //////////

  void addOperation(
      TextureUpdateOperation *operation,
      GLboolean isInitial=GL_FALSE);
  void removeOperation(TextureUpdateOperation *operation);

  list<TextureUpdateOperation*>& initialOperations();
  list<TextureUpdateOperation*>& operations();

  /**
   * Execute sequence of operations.
   */
  void executeOperations(const list<TextureUpdateOperation*>&);

  virtual void animate(GLdouble dt);
  virtual void updateGraphics(GLdouble dt);

protected:
  const string &name_;
  MeshState *textureQuad_;
  GLdouble dt_;

  GLint framerate_;

  list<TextureUpdateOperation*> operations_;
  list<TextureUpdateOperation*> initialOperations_;
  map<string,TextureBuffer*> buffers_;

private:
  TextureUpdater(const TextureUpdater&);
};

#endif /* TEXTURE_UPDATER_H_ */
