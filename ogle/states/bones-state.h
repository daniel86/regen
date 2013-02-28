/*
 * bones-state.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef BONES_STATE_H_
#define BONES_STATE_H_

#include <ogle/states/state.h>
#include <ogle/animations/animation-node.h>
#if 1
#include <ogle/gl-types/tbo.h>
#include <ogle/gl-types/vbo.h>
#include <ogle/states/texture-state.h>
#endif

/**
 * Provides bone matrices uniform.
 * If a shader is generated before a BonesState
 * then the world position will be transformed
 * by the bone matrices.
 */
class BonesState : public State, public Animation
{
public:
  BonesState(
      list< ref_ptr<AnimationNode> > &bones,
      GLuint numBoneWeights);
  ~BonesState();

  GLint numBoneWeights() const;

  // Animation override
  virtual void animate(GLdouble dt);
  virtual void glAnimate(RenderState *rs, GLdouble dt);
  virtual GLboolean useAnimation() const;
  virtual GLboolean useGLAnimation() const;
protected:
  list< ref_ptr<AnimationNode> > bones_;
  ref_ptr<ShaderInput1i> numBoneWeights_;

  ref_ptr<VertexBufferObject> boneMatrixVBO_;
  Mat4f *boneMatrixData_;

  GLuint lastBoneWeights_;
  GLuint lastBoneCount_;
};

#endif /* BONES_STATE_H_ */
