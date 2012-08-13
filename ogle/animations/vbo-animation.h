/*
 * vbo-animation.h
 *
 *  Created on: 21.03.2011
 *      Author: daniel
 */

#ifndef VBO_ANIMATION_H_
#define VBO_ANIMATION_H_

#include <ogle/animations/animation.h>
#include <ogle/states/mesh-state.h>

class AnimationBuffer;

/**
 * Animation using mapped VBO data.
 */
class VBOAnimation : public Animation {
public:
  VBOAnimation(MeshState &attributeState);

  /**
   * The attribute state associated to this animation.
   */
  MeshState& attributeState();

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
  /**
   * Size in bytes of the snapshot.
   */
  GLuint snapshotSize() const;

  /**
   * Offset in data VBO to the start of
   * the first attribute of the primitive.
   * Offset in bytes returned.
   */
  GLuint destinationOffset();
  /**
   * Sum of all primitive attribute sizes.
   * The whole primitive data takes this space.
   * Size in bytes returned.
   */
  GLuint destinationSize();
  /**
   * Buffer ID of primitive.
   * This buffer is supposed to get animated.
   */
  GLuint destinationBuffer();

  /**
   * Maps VBO data into RAM.
   */
  vector<VecXf> getFloatAttribute(
      AttributeIteratorConst it);
  /**
   * Maps VBO data into RAM.
   */
  vector<VecXf> getFloatAttribute(
      AttributeIteratorConst it, GLfloat *vals);
  /**
   * Maps VBO data into RAM.
   */
  vector<VecXf> getFloatAttribute(
      AttributeIteratorConst it, GLfloat *vals, GLuint offset);

  virtual GLboolean animateVBO(GLdouble dt) = 0;

  // override
  void animate(GLdouble dt);

protected:
  MeshState &attributeState_;

  // animation data
  void *animationData_;
  // offset in animation VBO
  GLuint sourceOffset_;
  // flag indicating if animation changed VBO data last frame
  GLboolean bufferChanged_;

  AnimationBuffer *animationBuffer_;

  ref_ptr<GLfloat> snapshot_;
  GLuint snapshotSize_;

  GLuint destinationOffset_;
  GLuint destinationSize_;

  GLboolean bufferChanged() const;
  void set_bufferChanged(bool);

  virtual void set_data(void *data, GLuint offset);

  void set_animationBuffer(AnimationBuffer *buffer);

  friend class AnimationBuffer;
};

#endif /* VBO_ANIMATION_H_ */
