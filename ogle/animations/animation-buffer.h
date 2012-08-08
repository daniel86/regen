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
 * Provides VBO for animation data.
 * You can add VBOAnimation's to the buffer.
 */
class AnimationBuffer
{
public:
  AnimationBuffer(GLenum bufferAccess=GL_MAP_READ_BIT|GL_MAP_WRITE_BIT);

  /**
   * Copy AnimationBuffer into VBO destination.
   */
  void copy(GLuint dst);

  /**
   * True is no animation is added.
   */
  GLboolean isEmpty() const;

  /**
   * True if the animation did something that
   * requires a copy of the data to the
   * VBO used for rendering.
   */
  GLboolean bufferChanged() const;

  /**
   * Adds a VBOAnimation to this buffer.
   */
  AnimationIterator add(VBOAnimation *animation, GLuint primitiveBuffer);
  /**
   * Removes previously added VBOAnimation from this buffer.
   */
  void remove(AnimationIterator it, GLuint primitiveBuffer);

  /**
   * Map animation data into RAM.
   */
  void map();
  /**
   * Unmap previously mapped data.
   */
  void unmap();
  /**
   * True if the animation buffer is currently mapped.
   */
  GLboolean mapped() const;

  /**
   * Animation buffer mutex lock.
   */
  void lock();
  /**
   * Animation buffer mutex unlock.
   */
  void unlock();

protected:
  VertexBufferObject animationVBO_;
  // mapped data from animation VBO
  GLvoid *animationData_;
  GLuint bufferOffset_;
  GLuint bufferSize_;

  GLenum bufferAccess_;
  GLboolean mapped_;

  list<VBOAnimation*> animations_;

  void sizeChanged(GLuint primitiveBuffer);
};

#endif /* ANIMATION_BUFFER_H_ */
