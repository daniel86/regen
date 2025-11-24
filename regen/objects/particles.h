/*
 * particle-state.h
 *
 *  Created on: 03.11.2012
 *      Author: daniel
 */

#ifndef PARTICLE_STATE_H_
#define PARTICLE_STATE_H_

#include <regen/objects/mesh.h>
#include "regen/shader/shader-state.h"
#include "regen/gl-types/atomic-counter.h"

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
		 * Advance mode for particle attributes.
		 * This influences how the attribute is updated in each frame,
		 * the functions are mostly equivalent to the OpenGL blend modes.
		 * Custom mode allows to use a custom shader function.
		 */
		enum AdvanceMode {
			ADVANCE_MODE_CUSTOM = 0,
			ADVANCE_MODE_SRC, 			    //!< c = c1
			ADVANCE_MODE_MULTIPLY, 		    //!< c = c0*c1
			ADVANCE_MODE_ADD, 			    //!< c = c0+c1
			ADVANCE_MODE_SMOOTH_ADD,        //!< c = c0 + factor*(c0+c1)
			ADVANCE_MODE_SUBTRACT, 	        //!< c = c0-c1
			ADVANCE_MODE_REVERSE_SUBTRACT,  //!< c = c1-c0
			ADVANCE_MODE_DIFFERENCE, 	    //!< c = abs(c0-c1)
			ADVANCE_MODE_LIGHTEN, 		    //!< c = max(c0,c1)
			ADVANCE_MODE_DARKEN, 		    //!< c = min(c0,c1)
			ADVANCE_MODE_MIX, 			    //!< c = mix(c0,c1,factor)
		};

		/**
		 * Ramp mode for particle attributes.
		 * This influences how the attribute is updated in each frame
		 * using a ramp texture.
		 * The mode determines how the ramp position is calculated at each frame.
		 * Custom mode allows to use a custom shader function.
		 */
		enum RampMode {
			// use a custom ramp position function
			RAMP_MODE_CUSTOM = 0,
			// ramp based on time
			RAMP_MODE_TIME,
			// ramp based on particle lifetime
			RAMP_MODE_LIFETIME,
			// ramp based on emitter distance
			RAMP_MODE_EMITTER_DISTANCE,
			// ramp based on distance to camera
			RAMP_MODE_CAMERA_DISTANCE,
			// ramp based on length of velocity vector
			RAMP_MODE_VELOCITY,
		};

		/**
		 * @param numParticles particle count.
		 * @param updateShaderKey shader for updating particles.
		 */
		explicit Particles(uint32_t numParticles,
			const std::string &updateShaderKey="regen.particles.emitter");

		/**
		 * Set the maximum number of particles to emit per frame.
		 * This has only an effect initially.
		 * @param maxEmits maximum number of particles to emit.
		 */
		void setMaxEmits(uint32_t maxEmits) { maxEmits_ = maxEmits; }

		/**
		 * Set the default value for a particle attribute when it is emitted.
		 * @param attributeName name of the attribute.
		 * @param value default value.
		 */
		template <class InputType, class ValueType>
		void setDefault(std::string_view attributeName, const ValueType &value) {
			auto x = ref_ptr<InputType>::alloc(REGEN_STRING(attributeName << "Default"));
			x->setUniformData(value);
			setInput(x);
		}

		/**
		 * Set the variance for a particle attribute when it is emitted.
		 * @param attributeName name of the attribute.
		 * @param value variance.
		 */
		template <class InputType, class ValueType>
		void setVariance(std::string_view attributeName, const ValueType &value) {
			auto x = ref_ptr<InputType>::alloc(REGEN_STRING(attributeName << "Variance"));
			x->setUniformData(value);
			setInput(x);
		}

		/**
		 * Set the advance constant for a particle attribute.
		 * This is a constant value which is multiplied by dt and blended with the current value
		 * at each frame.
		 * Will be ignored in case a ramp texture is set.
		 * @param attributeName name of the attribute.
		 * @param value advance constant.
		 */
		template <class InputType, class ValueType>
		void setAdvanceConstant(std::string_view attributeName, const ValueType &value) {
			auto x = ref_ptr<InputType>::alloc(REGEN_STRING(attributeName << "AdvanceConstant"));
			x->setUniformData(value);
			setInput(x);
		}

		/**
		 * Set the advance mode for a particle attribute.
		 * @param attributeName name of the attribute.
		 * @param mode advance mode.
		 */
		void setAdvanceMode(const std::string &attributeName, AdvanceMode mode) { advanceModes_.emplace(attributeName, mode); }

		/**
		 * Set the advance factor for a particle attribute.
		 * This is the factor used in the blending equation.
		 * @param attributeName name of the attribute.
		 * @param factor advance factor.
		 */
		void setAdvanceFactor(const std::string &attributeName, float factor) { advanceFactors_.emplace(attributeName, factor); }

		/**
		 * Set the advance function for a particle attribute.
		 * This is the shader function used in custom advance mode.
		 * @param attributeName name of the attribute.
		 * @param shaderFunction advance function.
		 */
		void setAdvanceFunction(const std::string &attributeName, const std::string &shaderFunction);

		/**
		 * Set the ramp texture for a particle attribute.
		 * @param attributeName name of the attribute.
		 * @param texture ramp texture.
		 * @param mode ramp mode.
		 */
		void setAdvanceRamp(const std::string &attributeName, const ref_ptr<Texture> &texture, RampMode mode);

		/**
		 * Set the ramp function for a particle attribute.
		 * This is the shader function used in custom ramp mode.
		 * @param attributeName name of the attribute.
		 * @param shaderFunction ramp function.
		 */
		void setRampFunction(const std::string &attributeName, const std::string &shaderFunction) { rampFunctions_.emplace(attributeName, shaderFunction); }

		// override
		void gpuUpdate(RenderState *rs, double dt) override;

		void begin();

		ref_ptr<BufferReference> end();

	protected:
		const std::string updateShaderKey_;
		ref_ptr<VBO> feedbackBuffer_;
		ref_ptr<BufferReference> vboRef_[2];
		uint32_t updateIdx_ = 0;
		BufferRange bufferRange_;
		//ref_ptr<BoundingBoxCounter> boundingBoxCounter_;
		std::list<InputLocation> particleAttributes_;
		ref_ptr<ShaderState> updateState_;
		VAO particleVAO_;

		uint32_t numParticles_;
		uint32_t maxEmits_;

		std::map<std::string, AdvanceMode> advanceModes_;
		std::map<std::string, std::string> advanceFunctions_;
		std::map<std::string, float> advanceFactors_;
		struct Ramp {
			ref_ptr<Texture> texture;
			RampMode mode;
		};
		std::map<std::string, Ramp> ramps_;
		std::map<std::string, std::string> rampFunctions_;

		void createUpdateShader();

		void configureAdvancing(const ref_ptr<ShaderInput> &in, uint32_t counter, AdvanceMode mode);
	};

	std::ostream &operator<<(std::ostream &out, const Particles::AdvanceMode &v);

	std::istream &operator>>(std::istream &in, Particles::AdvanceMode &v);

	std::ostream &operator<<(std::ostream &out, const Particles::RampMode &v);

	std::istream &operator>>(std::istream &in, Particles::RampMode &v);
} // namespace

#endif /* PARTICLE_STATE_H_ */
