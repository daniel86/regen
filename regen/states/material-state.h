#ifndef REGEN_MATERIAL_H_
#define REGEN_MATERIAL_H_

#include <regen/states/state.h>
#include <regen/textures/texture-state.h>
#include "regen/glsl/shader-input.h"
#include <regen/utility/ref-ptr.h>

namespace regen {
	typedef unsigned int MaterialVariant;

	/**
	 * \brief Describes a material.
	 *
	 * This class is used to describe a material. It contains the material colors,
	 * texture files, and other properties.
	 */
	struct MaterialDescription {
		/**
		 * \brief Constructor.
		 *
		 * @param materialName The name of the material.
		 * @param variant The variant of the material.
		 */
		MaterialDescription(std::string_view materialName, std::string_view variant);

		Vec3f ambient = Vec3f(0.0f);
		Vec3f diffuse = Vec3f(1.0f);
		Vec3f specular = Vec3f(0.0f);
		Vec3f emission = Vec3f(0.0f);
		float shininess = 128.0f;
		std::map<TextureState::MapTo, std::vector<std::string>> textureFiles;

		/**
		 * \brief Checks if the material has a texture file for the given map type.
		 *
		 * @param mapType The map type to check.
		 * @return True if the material has a texture file for the given map type, false otherwise.
		 */
		bool hasMap(TextureState::MapTo mapMode) const { return textureFiles.find(mapMode) != textureFiles.end(); }

		/**
		 * \brief Gets the texture file for the given map type.
		 *
		 * @param mapType The map type to get the texture file for.
		 * @return The texture file for the given map type.
		 */
		std::vector<std::string> getMaps(TextureState::MapTo mapMode) const { return textureFiles.at(mapMode); }

	protected:
		void addTexture(const boost::filesystem::path &path);
	};

	/**
	 * \brief Provides material related uniforms.
	 */
	class Material : public State {
	public:
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

		Material(const BufferUpdateFlags &updateFlags = { BUFFER_UPDATE_NEVER, BUFFER_UPDATE_PARTIALLY });

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
		 * Sets whether to use mipmaps for textures.
		 * @param useMipmap true to use mipmaps, false otherwise.
		 */
		void set_useMipmaps(bool useMipmap) { useMipmap_ = useMipmap; }

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
		void set_jade(std::string_view variant = "");

		/**
		 * Sets default material colors for ruby.
		 */
		void set_ruby(std::string_view variant = "");

		/**
		 * Sets default material colors for chrome.
		 */
		void set_chrome(std::string_view variant = "");

		/**
		 * Sets default material colors for leather.
		 */
		void set_leather(std::string_view variant = "");

		/**
		 * Sets default material colors for stone.
		 */
		void set_stone(std::string_view variant = "");

		/**
		 * Sets default material colors for gold.
		 */
		void set_gold(std::string_view variant = "");

		/**
		 * Sets default material colors for copper.
		 */
		void set_copper(std::string_view variant = "");

		/**
		 * Sets default material colors for silver.
		 */
		void set_silver(std::string_view variant = "");

		/**
		 * Sets default material colors for pewter.
		 */
		void set_pewter(std::string_view variant = "");

		/**
		 * Sets default material colors for iron.
		 */
		void set_iron(std::string_view variant = "");

		/**
		 * Sets default material colors for steel.
		 */
		void set_steel(std::string_view variant = "");

		/**
		 * Sets default material colors for metal.
		 */
		void set_metal(std::string_view variant = "");

		/**
		 * Sets default material colors for wood.
		 */
		void set_wood(std::string_view variant = "");

		/**
		 * Sets default material colors for marble.
		 */
		void set_marble(std::string_view variant = "");

		/**
		 * Loads textures for the given material name and variant.
		 * @param materialName a material name.
		 * @param variant a variant.
		 */
		bool set_textures(std::string_view materialName, std::string_view variant);

		ref_ptr<TextureState> set_texture(const ref_ptr<Texture> &tex, TextureState::MapTo mapTo);

		ref_ptr<TextureState> set_texture(
				const ref_ptr<Texture> &tex,
				TextureState::MapTo mapTo,
				const std::string &name);

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
		ref_ptr<UBO> materialUniforms_;

		bool useMipmap_ = true;
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

		void set_textureState(const ref_ptr<TextureState> &tex, TextureState::MapTo mapTo);

		friend class FillModeState;
	};

	std::ostream &operator<<(std::ostream &out, const Material::HeightMapMode &v);

	std::istream &operator>>(std::istream &in, Material::HeightMapMode &v);
} // namespace

#endif /* REGEN_MATERIAL_H_ */
