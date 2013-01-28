/*
 * hair-particles.h
 *
 *  Created on: 27.01.2013
 *      Author: daniel
 */

#ifndef HAIR_PARTICLES_H_
#define HAIR_PARTICLES_H_

#include <ogle/meshes/particles.h>

class HairParticles : public ParticleState
{
public:
  HairParticles(
      const ref_ptr<ShaderInput> &hairRoots,
      const ref_ptr<ShaderInput> &normals);

  void createShader(ShaderConfig &shaderCfg);

  const ref_ptr<ShaderInput2f>& hairLength() const;
  const ref_ptr<ShaderInput1f>& hairMass() const;
  const ref_ptr<ShaderInput1f>& hairRigidity() const;

protected:
  ref_ptr<ShaderInput2f> hairLengthFuzzy_;
  ref_ptr<ShaderInput1f> hairMass_;
  ref_ptr<ShaderInput1f> hairRigidity_;

  ref_ptr<ShaderInput> normalInput_;
  ref_ptr<ShaderInput> hairRootInput_;
  ref_ptr<ShaderInput3f> controlPointInput_;
  ref_ptr<ShaderInput3f> endPointInput_;
  ref_ptr<ShaderInput1f> lifetimeInput_;
  ref_ptr<ShaderInput1f> hairLength_;
};

#endif /* HAIR_PARTICLES_H_ */
