
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
#include <ogle/states/mesh-state.h>

/**
 * Executes a sequence of operations for updating a
 * texture.
 */
class TextureUpdater : public Animation
{
public:
  TextureUpdater();
  ~TextureUpdater();

  const string& name() const;

  /**
   * The desired animation framerate.
   */
  GLint framerate() const;
  void set_framerate(GLint framerate);

  /**
   * Serializing.
   */
  friend void operator>>(istream &in, TextureUpdater &v);
  void parseConfig(const map<string,string> &cfg);

  //////////

  /**
   * Add a named buffer to the list of known buffers.
   */
  void addBuffer(SimpleRenderTarget *buffer);
  /**
   * Get buffer by name.
   */
  SimpleRenderTarget* getBuffer(const string &name);

  //////////

  void addOperation(
      TextureUpdateOperation *operation,
      GLboolean isInitial=GL_FALSE);
  void removeOperation(TextureUpdateOperation *operation);

  list<TextureUpdateOperation*>& initialOperations();
  list<TextureUpdateOperation*>& operations();
  map<string,SimpleRenderTarget*>& buffers();

  /**
   * Execute sequence of operations.
   */
  void executeOperations(const list<TextureUpdateOperation*>&);

  // Override
  virtual void animate(GLdouble dt);
  virtual void glAnimate(GLdouble dt);
  virtual GLboolean useAnimation() const;
  virtual GLboolean useGLAnimation() const;

protected:
  string name_;
  GLdouble dt_;

  GLint framerate_;

  list<TextureUpdateOperation*> operations_;
  list<TextureUpdateOperation*> initialOperations_;
  map<string,SimpleRenderTarget*> buffers_;

private:
  TextureUpdater(const TextureUpdater&);
};
ostream& operator<<(ostream& os, const TextureUpdater& v);

#endif /* TEXTURE_UPDATER_H_ */
