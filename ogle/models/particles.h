/*
 * particles.h
 *
 *  Created on: 02.11.2012
 *      Author: daniel
 */

#ifndef PARTICLES_H_
#define PARTICLES_H_

class Particles : public MeshState, public Animation
{
public:
  Particles(GLuint numParticles);

  virtual void animate(GLdouble dt);
  virtual void updateGraphics(GLdouble dt);

protected:
  GLuint maxNumParticles_;
  GLuint numActiveParticles_;

  // Ping pong VBO for particles
  ref_ptr<VertexBufferObject> particleBuffer_, feedbackBuffer_;

  // used to update particle attributes
  ref_ptr<Shader> updateShader_;
  // used to draw particles
  ref_ptr<Shader> drawShader_;
};

#endif /* PARTICLES_H_ */
