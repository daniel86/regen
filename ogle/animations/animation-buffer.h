/*
 * animation-buffer.h
 *
 *  Created on: 21.03.2011
 *      Author: daniel
 */

#ifndef ANIMATION_BUFFER_H_
#define ANIMATION_BUFFER_H_

#include <list>
using namespace std;

#include <ogle/animations/vbo-animation.h>
#include <ogle/gl-types/vbo.h>

typedef list<VBOAnimation*>::iterator AnimationIterator;

/**
 * AnimationBuffer is a list of primitive set animations.
 * Data of all primitives saved in the same vbo.
 */
class AnimationBuffer {
public:
  AnimationBuffer(GLenum bufferAccess=GL_MAP_READ_BIT|GL_MAP_WRITE_BIT);
  ~AnimationBuffer();

  /**
   * Copy AnimationBuffer into VBO destination.
   */
  void copy(GLuint dst);

  GLuint numAnimations() const;
  bool bufferChanged() const;

  AnimationIterator add(VBOAnimation *animation, GLuint primitiveBuffer);
  void remove(AnimationIterator it, GLuint primitiveBuffer);

  void map();
  void unmap();

  void lock();
  void unlock();

  bool mapped() const {
    return mapped_;
  }

protected:
  BufferData data_; // stores data pointer to VBO

  GLenum bufferAccess_;
  VertexBufferObject animationVBO_;
  bool mapped_;
  bool bufferDataChanged_;

  unsigned int animationBufferSizeBytes_;

  list<VBOAnimation*> animations_;

  void sizeChanged(GLuint primitiveBuffer);
};

#endif /* ANIMATION_BUFFER_H_ */
