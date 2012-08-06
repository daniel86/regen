/*
 * vbo-animation.h
 *
 *  Created on: 21.03.2011
 *      Author: daniel
 */

#ifndef VBO_ANIMATION_H_
#define VBO_ANIMATION_H_

#include <ogle/animations/animation.h>
#include <ogle/states/attribute-state.h>

class AnimationBuffer;

/**
 * Animation using mapped VBO data.
 */
class VBOAnimation : public Animation {
public:
  VBOAnimation(GLuint vbo, AttributeState &mesh);

  void set_bufferChanged(bool bufferChanged);
  bool bufferChanged() const;

  void set_animationBuffer(AnimationBuffer *buffer);
  virtual void set_data(BufferData *data);
  BufferData* data();

  /**
   * Saves current primitive VBO data in RAM.
   */
  void makeSnapshot();
  /**
   * Deletes any previously saved snapshot.
   */
  void clearSnapshot();
  /**
   * Returns snapshot VBO data or null if no snapshot taken.
   */
  ref_ptr<GLfloat> snapshot();
  GLuint snapshotSize() const;

  /**
   * Offset in data VBO to the start of
   * the first attribute of the primitive.
   * Offset in bytes returned.
   */
  GLuint offsetInDataBufferToPrimitiveStart();
  /**
   * Sum of all primitive attribute sizes.
   * The whole primitive data takes this space.
   * Size in bytes returned.
   */
  GLuint primitiveSetBufferSize();
  /**
   * Buffer ID of primitive.
   * This buffer is supposed to get animated.
   */
  GLuint primitiveBuffer();

  AttributeState& mesh() {
    return mesh_;
  }

  /**
   * Maps VBO data into a std::vector holding StructXf instances.
   * StructXf instances contain the VRAM data of the attribute.
   * NOTE: it might be better to write the data sequentially.
   * Meaning that you should not jump on the buffer when writing data.
   * The Data returned by this function might not be contiguous if
   * a interleaved VBO was used.
   */
  vector< VecXf > getFloatAttribute(AttributeIteratorConst it);
  vector< VecXf > getFloatAttribute(AttributeIteratorConst it, float *vals);
  vector< VecXf > getFloatAttribute(AttributeIteratorConst it, float *vals, GLuint offset);

protected:
  BufferData *data_;
  AnimationBuffer *animationBuffer_;
  ref_ptr<GLfloat> snapshot_;
  GLuint snapshotSize_;
  GLuint offsetInDataBuffer_;
  GLuint primitiveSetBufferSize_;
  GLuint primitiveBuffer_;
  AttributeState &mesh_;
};

#endif /* VBO_ANIMATION_H_ */
