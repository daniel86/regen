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

/**
 * Provides bone matrices uniform.
 * If a shader is generated before a BonesState
 * then the world position will be transformed
 * by the bone matrices.
 */
class BonesState : public State
{
public:
  BonesState(
      vector< ref_ptr<AnimationNode> > &bones,
      GLuint numBoneWeights);

  virtual void update(GLfloat dt);
  virtual void configureShader(ShaderConfig *cfg);

  virtual string name();
protected:
  ref_ptr<ShaderInputMat4> boneMatrices_;
  vector< ref_ptr<AnimationNode> > bones_;
  GLuint numBoneWeights_;
};

#endif /* BONES_STATE_H_ */
