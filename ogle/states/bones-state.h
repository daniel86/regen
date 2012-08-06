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

class BonesState : public State
{
public:
  BonesState();
  /**
   * Set data used for calculating bone matrices.
   */
  void setBones(
      ref_ptr<Bone> rootNode,
      vector< ref_ptr<Bone> > bones);
  /**
   * Calculates the bone matrices for this mesh.
   */
  void calculateBoneMatrices();

  virtual void configureShader(ShaderConfiguration *cfg);
protected:
  ref_ptr<UniformMat4> boneMatrices_;
  ref_ptr<Bone> rootBoneNode_;
  vector< ref_ptr<Bone> > bones_;
  bool hasBones_;
};

#endif /* BONES_STATE_H_ */
