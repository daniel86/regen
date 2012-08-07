/*
 * bones-state.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef BONES_STATE_H_
#define BONES_STATE_H_

#include <ogle/states/state.h>
#include <ogle/animations/bone.h>

/**
 * Provides bone matrices uniform.
 * If a shader is generated before a BonesState
 * then the world position will be transformed
 * by the bone matrices.
 */
class BonesState : public State
{
public:
  BonesState();
  /**
   * Set data used for calculating bone matrices.
   */
  void setBones(
      ref_ptr<Bone> &rootNode,
      vector< ref_ptr<Bone> > &bones);

  virtual void update(GLfloat dt);
  virtual void configureShader(ShaderConfiguration *cfg);
protected:
  ref_ptr<UniformMat4> boneMatrices_;
  ref_ptr<Bone> rootBoneNode_;
  vector< ref_ptr<Bone> > bones_;
  bool hasBones_;
};

#endif /* BONES_STATE_H_ */
