/*
 * particle-state.h
 *
 *  Created on: 03.11.2012
 *      Author: daniel
 */

#ifndef PARTICLE_STATE_H_
#define PARTICLE_STATE_H_

#include <regen/meshes/mesh-state.h>
#include <regen/states/shader-state.h>

namespace regen {
	/**
	 * \brief Point sprite particle system.
	 *
	 * Using the geometry shader for updating
	 * and emitting particles and using transform feedback to
	 * stream updated particle attributes to a ping pong VBO.
	 */
	class Particles : public Mesh, public Animation {
	public:
		/**
		 * @param numParticles particle count.
		 * @param updateShaderKey shader for updating particles.
		 */
		Particles(GLuint numParticles, const std::string &updateShaderKey);

		/**
		 * @return gravity constant.
		 */
		const ref_ptr<ShaderInput3f> &gravity() const;

		/**
		 * @return damping factor.
		 */
		const ref_ptr<ShaderInput1f> &dampingFactor() const;

		/**
		 * @return noise factor.
		 */
		const ref_ptr<ShaderInput1f> &noiseFactor() const;

		/**
		 * @return number of maximum particle emits per frame.
		 */
		const ref_ptr<ShaderInput1i> &maxNumParticleEmits() const;

		// override
		void glAnimate(RenderState *rs, GLdouble dt) override;

		/**
		 * Begin recording ShaderInput's using interleaved layout.
		 */
		void begin();

		VBOReference end();

	protected:
		const std::string updateShaderKey_;
		ref_ptr<VBO> feedbackBuffer_;
		VBOReference feedbackRef_;
		VBOReference particleRef_;
		BufferRange bufferRange_;
		std::list<ShaderInputLocation> particleAttributes_;

		ref_ptr<ShaderInput3f> gravity_;
		ref_ptr<ShaderInput1f> dampingFactor_;
		ref_ptr<ShaderInput1f> noiseFactor_;

		ref_ptr<ShaderInput1f> deltaT_;
		ref_ptr<ShaderState> updateState_;

		// override
		void begin(ShaderInputContainer::DataLayout layout);

		void createUpdateShader(const ShaderInputList &inputs);

	};
} // namespace

#endif /* PARTICLE_STATE_H_ */
