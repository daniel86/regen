/*
 * material.h
 *
 *  Created on: 02.02.2011
 *      Author: daniel
 */

#ifndef REGEN_MATERIAL_H_
#define REGEN_MATERIAL_H_

#include <regen/states/state.h>
#include <regen/states/texture-state.h>
#include <regen/gl-types/shader-input-container.h>
#include <regen/gl-types/shader-input.h>
#include <regen/utility/ref-ptr.h>

namespace regen {
	/**
	 * \brief Provides material related uniforms.
	 */
	class Material : public HasInputState {
	public:
		Material();

		/**
		 * @return Ambient material color.
		 */
		auto &ambient() const { return materialAmbient_; }

		/**
		 * @return Diffuse material color.
		 */
		auto &diffuse() const { return materialDiffuse_; }

		/**
		 * @return Specular material color.
		 */
		auto &specular() const { return materialSpecular_; }

		/**
		 * @return The shininess exponent.
		 */
		auto &shininess() const { return materialShininess_; }

		/**
		 * @return The emission color.
		 */
		auto &emission() const { return materialEmission_; }

		/**
		 * @return The material alpha.
		 */
		auto &alpha() const { return materialAlpha_; }

		/**
		 * Index of refraction of the material. This is used by some shading models,
		 * e.g. Cook-Torrance. The value is the ratio of the speed of light in a
		 * vacuum to the speed of light in the material (always >= 1.0 in the real world).
		 */
		auto &refractionIndex() const { return materialRefractionIndex_; }

		/**
		 * Sets the emission color.
		 * @param emission The emission color.
		 */
		void set_emission(const Vec3f &emission);

		/**
		 * Defines how faces are shaded (FILL/LINE/POINT).
		 */
		void set_fillMode(GLenum mode);

		/**
		 * Defines how faces are shaded (FILL/LINE/POINT).
		 */
		auto fillMode() const { return fillMode_; }

		/**
		 * Indicates if the material should be rendered two-sided.
		 */
		void set_twoSided(GLboolean v);

		/**
		 * Indicates if the material should be rendered two-sided.
		 */
		GLboolean twoSided() const;

		/**
		 * Sets default material colors for jade.
		 */
		void set_jade();

		/**
		 * Sets default material colors for ruby.
		 */
		void set_ruby();

		/**
		 * Sets default material colors for chrome.
		 */
		void set_chrome();

		/**
		 * Sets default material colors for gold.
		 */
		void set_gold();

		/**
		 * Sets default material colors for copper.
		 */
		void set_copper();

		/**
		 * Sets default material colors for silver.
		 */
		void set_silver();

		/**
		 * Sets default material colors for pewter.
		 */
		void set_pewter();

	private:
		GLenum fillMode_;

		std::vector<ref_ptr<Texture> > textures_;
		ref_ptr<ShaderInput3f> materialDiffuse_;
		ref_ptr<ShaderInput3f> materialAmbient_;
		ref_ptr<ShaderInput3f> materialSpecular_;
		ref_ptr<ShaderInput1f> materialShininess_;
		ref_ptr<ShaderInput3f> materialEmission_;
		ref_ptr<ShaderInput1f> materialRefractionIndex_;
		ref_ptr<ShaderInput1f> materialAlpha_;

		ref_ptr<State> twoSidedState_;
		ref_ptr<State> fillModeState_;

		Material(const Material &);

		Material &operator=(const Material &other);

		friend class FillModeState;
	};
} // namespace

#endif /* REGEN_MATERIAL_H_ */
