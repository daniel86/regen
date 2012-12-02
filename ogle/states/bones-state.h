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
      vector< ref_ptr<AnimationNode> > &bones,
      GLuint numBoneWeights);
  ~BonesState();

  virtual void animate(GLdouble dt);
  virtual void updateGraphics(GLdouble dt);
  virtual void enable(RenderState *rs);
  virtual void disable(RenderState *rs);
  virtual void configureShader(ShaderConfig *cfg);
protected:
  vector< ref_ptr<AnimationNode> > bones_;
  GLuint numBoneWeights_;

  ref_ptr<TextureBufferObject> boneMatrixTBO_;
  ref_ptr<ShaderInputMat4> boneMatrices_;

  GLuint boneMatrixVBO_;
  Mat4f *boneMatrixData_;

  Mat4f *lastBoneMatrices_;
  GLuint lastBoneWeights_;
  GLuint lastBoneCount_;
};

#endif /* BONES_STATE_H_ */
