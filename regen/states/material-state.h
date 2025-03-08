/*
 * material.h
 *
 *  Created on: 02.02.2011
 *      Author: daniel
 */

#ifndef REGEN_MATERIAL_H_
#define REGEN_MATERIAL_H_

#include <regen/states/state.h>
#include <regen/textures/texture-state.h>
#include <regen/gl-types/shader-input-container.h>
#include <regen/gl-types/shader-input.h>
#include <regen/utility/ref-ptr.h>

namespace regen {
	/**
	 * \brief Provides material related uniforms.
	 */
	class Material : public HasInputState {
	public:
		typedef unsigned int Variant;

		/**
		 * Defines how height maps are used.
		 */
		enum HeightMapMode {
			// map height value to vertex position either in vertex shader or in tessellation control shader
			HEIGHT_MAP_VERTEX = 0,
			// use height map for relief mapping
			HEIGHT_MAP_RELIEF,
			// use height map for parallax mapping
			HEIGHT_MAP_PARALLAX,
			// use height map for parallax occlusion mapping
			HEIGHT_MAP_PARALLAX_OCCLUSION
		};

		Material();

		static ref_ptr<Material> load(LoadingContext &ctx, scene::SceneInputNode &input);

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
		 * Sets the wrapping mode for all textures.
		 */
		void set_wrapping(GLenum wrapping) { wrapping_ = wrapping; }

		/**
		 * Sets the blending mode for color and diffuse maps
		 * added to this material.
		 */
		void set_colorBlendMode(BlendMode mode) { colorBlendMode_ = mode; }

		/**
		 * Sets the blending factor for color and diffuse maps
		 * added to this material.
		 */
		void set_colorBlendFactor(GLfloat factor) { colorBlendFactor_ = factor; }

		/**
		 * Sets the maximum height offset for height and displacement maps.
		 */
		void set_maxOffset(GLfloat offset);

		/**
		 * Sets the height map mode.
		 */
		void set_heightMapMode(HeightMapMode mode) { heightMapMode_ = mode; }

		/**
		 * Indicates if the material should be rendered two-sided.
		 */
		GLboolean twoSided() const { return twoSidedState_.get() != nullptr; }

		/**
		 * Sets default material colors for jade.
		 */
		void set_jade(Variant variant = 0);

		/**
		 * Sets default material colors for ruby.
		 */
		void set_ruby(Variant variant = 0);

		/**
		 * Sets default material colors for chrome.
		 */
		void set_chrome(Variant variant = 0);

		/**
		 * Sets default material colors for leather.
		 */
		void set_leather(Variant variant = 0);

		/**
		 * Sets default material colors for stone.
		 */
		void set_stone(Variant variant = 0);

		/**
		 * Sets default material colors for gold.
		 */
		void set_gold(Variant variant = 0);

		/**
		 * Sets default material colors for copper.
		 */
		void set_copper(Variant variant = 0);

		/**
		 * Sets default material colors for silver.
		 */
		void set_silver(Variant variant = 0);

		/**
		 * Sets default material colors for pewter.
		 */
		void set_pewter(Variant variant = 0);

		/**
		 * Sets default material colors for iron.
		 */
		void set_iron(Variant variant = 0);

		/**
		 * Sets default material colors for steel.
		 */
		void set_steel(Variant variant = 0);

		/**
		 * Sets default material colors for metal.
		 */
		void set_metal(Variant variant = 0);

		/**
		 * Sets default material colors for wood.
		 */
		void set_wood(Variant variant = 0);

		/**
		 * Sets default material colors for marble.
		 */
		void set_marble(Variant variant = 0);

		/**
		 * Loads textures for the given material name and variant.
		 * @param materialName a material name.
		 * @param variant a variant.
		 */
		bool set_textures(std::string_view materialName, Variant variant);

		/**
		 * Loads textures for the given material name and variant.
		 * @param materialName a material name.
		 * @param variant a variant.
		 */
		bool set_textures(std::string_view materialName, std::string_view variant);

	private:
		GLenum fillMode_;

		std::map<TextureState::MapTo, std::vector<ref_ptr<TextureState>>> textures_;
		ref_ptr<ShaderInput3f> materialDiffuse_;
		ref_ptr<ShaderInput3f> materialAmbient_;
		ref_ptr<ShaderInput3f> materialSpecular_;
		ref_ptr<ShaderInput1f> materialShininess_;
		ref_ptr<ShaderInput3f> materialEmission_;
		ref_ptr<ShaderInput1f> materialRefractionIndex_;
		ref_ptr<ShaderInput1f> materialAlpha_;
		ref_ptr<UniformBlock> materialUniforms_;

		GLenum mipmapFlag_;
		GLenum forcedInternalFormat_;
		GLenum forcedFormat_;
		Vec3ui forcedSize_;
		GLfloat maxOffset_;
		HeightMapMode heightMapMode_;
		BlendMode colorBlendMode_;
		GLfloat colorBlendFactor_;
		std::optional<GLenum> wrapping_;

		ref_ptr<State> twoSidedState_;
		ref_ptr<State> fillModeState_;

		Material(const Material &);

		Material &operator=(const Material &other);

		void set_texture(const ref_ptr<TextureState> &tex, TextureState::MapTo mapTo);

		bool getMapTo(std::string_view fileName, TextureState::MapTo &mapTo);

		friend class FillModeState;
	};

	std::ostream &operator<<(std::ostream &out, const Material::HeightMapMode &v);

	std::istream &operator>>(std::istream &in, Material::HeightMapMode &v);
} // namespace

#endif /* REGEN_MATERIAL_H_ */
