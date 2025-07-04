/*
 * texture-node.h
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#ifndef TEXTURE_NODE_H_
#define TEXTURE_NODE_H_

#include <regen/states/state.h>
#include <regen/states/blend-state.h>
#include <regen/textures/texture.h>
#include "regen/gl-types/shader-function.h"

namespace regen {
	/**
	 * \brief A State Object that contains one or more images
	 * that all have the same image format.
	 */
	class TextureState : public State {
	public:
		/**
		 * \brief Defines what is affected by the texture.
		 */
		enum MapTo {
			/** The texture is combined with the fragment color. */
			MAP_TO_COLOR,
			/** The texture is combined with the result of the diffuse lighting equation. */
			MAP_TO_DIFFUSE,
			/** The texture is combined with the result of the ambient lighting equation. */
			MAP_TO_AMBIENT,
			/** The texture is combined with the result of the specular lighting equation. */
			MAP_TO_SPECULAR,
			/** The texture defines the glossiness of the material. */
			MAP_TO_SHININESS,
			/** The texture is added to the result of the lighting calculation. */
			MAP_TO_EMISSION,
			/** Lightmap texture (aka Ambient Occlusion). */
			MAP_TO_LIGHT,
			/** The texture defines per-pixel opacity. */
			MAP_TO_ALPHA,
			/** The texture is a normal map. */
			MAP_TO_NORMAL,
			/** The texture is a height map.. */
			MAP_TO_HEIGHT,
			/** Displacement texture. The exact purpose and format is application-dependent.. */
			MAP_TO_DISPLACEMENT,
			/** The texture is a mask which is applied to vertices. */
			MAP_TO_VERTEX_MASK,
			/** user defined. */
			MAP_TO_CUSTOM
		};
		/**
		 * \brief Defines how a texture should be mapped on geometry.
		 */
		enum Mapping {
			/** Texture coordinate mapping. */
			MAPPING_TEXCO,
			/** Flat mapping. */
			MAPPING_FLAT,
			/** Cube mapping. */
			MAPPING_CUBE,
			/** Tube mapping. */
			MAPPING_TUBE,
			/** Sphere mapping. */
			MAPPING_SPHERE,
			/** Reflection mapping (for Cube Maps). */
			MAPPING_CUBE_REFLECTION,
			/** Reflection mapping (for Planar reflection).
			 *  Requires an uniform named 'reflectionMatrix' that
			 *  was used to render the reflected scene (proj*view*reflection). */
			MAPPING_PLANAR_REFLECTION,
			/** Reflection mapping (for Paraboloid Maps). */
			MAPPING_PARABOLOID_REFLECTION,
			/** Refraction mapping. */
			MAPPING_REFRACTION,
			MAPPING_CUBE_REFRACTION,
			MAPPING_INSTANCE_REFRACTION,
			/** Use XZ coordinates for mapping. */
			MAPPING_XZ_PLANE,
			/** User defined mapping. */
			MAPPING_CUSTOM
		};
		/**
		 * \brief some default transfer functions for texco values
		 */
		enum TransferTexco {
			/** parallax mapping. */
			TRANSFER_TEXCO_PARALLAX,
			/** parallax occlusion mapping. */
			TRANSFER_TEXCO_PARALLAX_OCC,
			/** relief mapping. */
			TRANSFER_TEXCO_RELIEF,
			/** fisheye mapping. */
			TRANSFER_TEXCO_FISHEYE,
			/** noise mapping. */
			TRANSFER_TEXCO_NOISE
		};
		/**
		 * \brief Specifies how a texel should be transferred before returning it.
		 */
		enum TexelTransfer {
			/** no transfer. */
			TEXEL_TRANSFER_IDENTITY,
			/** eye normal mapping. */
			TEXEL_TRANSFER_EYE_NORMAL,
			/** world normal mapping. */
			TEXEL_TRANSFER_WORLD_NORMAL,
			/** tangent normal mapping. */
			TEXEL_TRANSFER_TANGENT_NORMAL,
			/** unity-style normal mapping. */
			TEXEL_TRANSFER_UNITY_NORMAL,
			/** out = 1 - in */
			TEXEL_TRANSFER_INVERT,
			TEXEL_TRANSFER_GRAYSCALE,
			TEXEL_TRANSFER_BRIGHTNESS,
			TEXEL_TRANSFER_CONTRAST,
			TEXEL_TRANSFER_SATURATION,
			TEXEL_TRANSFER_HUE,
			TEXEL_TRANSFER_GAMMA
		};
		/**
		 * \brief Specifies if texture coordinates should be flipped.
		 */
		enum TexcoFlipping {
			/** flip x. */
			TEXCO_FLIP_X,
			/** flip y. */
			TEXCO_FLIP_Y,
			/** no flipping. */
			TEXCO_FLIP_NONE
		};

		TextureState();

		/**
		 * @param tex the associates texture.
		 * @param name the name of the texture in shader programs.
		 */
		explicit TextureState(const ref_ptr<Texture> &tex, const std::string &name = "");

		static ref_ptr<TextureState> load(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * @return used to get unique names in shaders.
		 */
		auto stateID() const { return stateID_; }

		/**
		 * @param tex the associates texture.
		 */
		void set_texture(const ref_ptr<Texture> &tex);

		/**
		 * @return the associates texture.
		 */
		auto &texture() const { return texture_; }

		/**
		 * @param name the name of this texture in shader programs.
		 */
		void set_name(const std::string &name);

		/**
		 * @return the name of this texture in shader programs.
		 */
		auto &name() const { return name_; }

		/**
		 * @param samplerType the sampler name of this texture in shader programs.
		 */
		void set_samplerType(const std::string &samplerType) { samplerType_ = samplerType; }

		/**
		 * @return the name of this texture in shader programs.
		 */
		auto &samplerType() const { return samplerType_; }

		/**
		 * @param channel  the texture coordinate channel.
		 */
		void set_texcoChannel(GLuint channel);

		/**
		 * @return the texture coordinate channel.
		 */
		auto texcoChannel() const { return texcoChannel_; }

		/**
		 * @param blendMode Specifies how this texture should be mixed with existing
		 * values.
		 */
		void set_blendMode(BlendMode blendMode);

		/**
		 * Specifies how this texture should be mixed with existing values.
		 * @param function specification of a shader function that should be used.
		 */
		void set_blendMode(const ref_ptr<ShaderFunction> &function);

		/**
		 * @param factor Specifies how this texture should be mixed with existing
		 * pixels.
		 */
		void set_blendFactor(GLfloat factor);

		/**
		 * @param mapping Specifies how a texture should be mapped on geometry.
		 */
		void set_mapping(Mapping mapping);

		/**
		 * Specifies how a texture should be mapped on geometry.
		 * @param function specification of a shader function that should be used.
		 */
		void set_mapping(const ref_ptr<ShaderFunction> &function);

		/**
		 * @param id Defines what is affected by the texture.
		 */
		void set_mapTo(MapTo id);

		/**
		 * @return true if the texture has a custom transfer function.
		 */
		bool hasTexelTransfer() const { return texelTransfer_.get(); }

		/**
		 * Specifies how texture samples are transferred before returning them.
		 * @param transfer Specifies how texture samples are transfererd before returning them.
		 */
		void set_texelTransfer(TextureState::TexelTransfer transfer);

		/**
		 * Specifies how a texture should be sampled.
		 * @param function specification of a shader function that should be used.
		 */
		void set_texelTransfer(const ref_ptr<ShaderFunction> &function);

		/**
		 * @param mode Specifies how texture coordinates are transferred before sampling.
		 */
		void set_texcoTransfer(TransferTexco mode);

		/**
		 * Specifies how texture coordinates are transferred before sampling.
		 * @param function specification of a shader function that should be used.
		 */
		void set_texcoTransfer(const ref_ptr<ShaderFunction> &function);

		/**
		 * Specifies how texture coordinates should be flipped.
		 * @param mode Specifies how texture coordinates should be flipped.
		 */
		void set_texcoFlipping(TexcoFlipping mode);

		/**
		 * Explicit request to the application to ignore the alpha channel
		 * of the texture.
		 */
		void set_ignoreAlpha(GLboolean v);

		/**
		 * Explicit request to the application to ignore the alpha channel
		 * of the texture.
		 */
		auto ignoreAlpha() const { return ignoreAlpha_; }

		/**
		 * @return true if the texture is a normal map.
		 */
		bool isNormalMap() const { return mapTo_ == MAP_TO_NORMAL; }

		/**
		 * @return true if the texture is a height map.
		 */
		bool isHeightMap() const { return mapTo_ == MAP_TO_HEIGHT; }

		/**
		 * @return true if the texture is a displacement map.
		 */
		bool isDisplacementMap() const { return mapTo_ == MAP_TO_DISPLACEMENT; }

		// override
		void enable(RenderState *rs) override;

		static ref_ptr<Texture> getTexture(
				scene::SceneLoader *scene,
				scene::SceneInputNode &input,
				const std::string &idKey = "id",
				const std::string &bufferKey = "fbo",
				const std::string &attachmentKey = "attachment");

	protected:
		static GLuint idCounter_;

		GLuint stateID_;

		ref_ptr<Texture> texture_;
		std::string name_;
		std::string samplerType_;

		BlendMode blendMode_;
		Mapping mapping_;
		MapTo mapTo_;
		GLfloat blendFactor_;

		GLuint texcoChannel_;
		GLint lastTexChannel_;

		GLboolean ignoreAlpha_;

		ref_ptr<ShaderFunction> blendFunction_;
		ref_ptr<ShaderFunction> mappingFunction_;
		ref_ptr<ShaderFunction> texelTransfer_;
		ref_ptr<ShaderFunction> texcoTransfer_;
	};

	std::ostream &operator<<(std::ostream &out, const TextureState::Mapping &v);

	std::istream &operator>>(std::istream &in, TextureState::Mapping &v);

	std::ostream &operator<<(std::ostream &out, const TextureState::MapTo &v);

	std::istream &operator>>(std::istream &in, TextureState::MapTo &v);

	std::ostream &operator<<(std::ostream &out, const TextureState::TransferTexco &v);

	std::istream &operator>>(std::istream &in, TextureState::TransferTexco &v);

	std::ostream &operator<<(std::ostream &out, const TextureState::TexelTransfer &v);

	std::istream &operator>>(std::istream &in, TextureState::TexelTransfer &v);
} // namespace

namespace regen {
	class TextureIndexState : public State {
	public:
		TextureIndexState() = default;

		static ref_ptr<State> load(LoadingContext &ctx, scene::SceneInputNode &input);
	};

	/**
	 * \brief Activates texture image when enabled.
	 */
	class TextureSetIndex : public TextureIndexState {
	public:
		/**
		 * @param tex texture reference.
		 * @param objectIndex the buffer index that should be activated.
		 */
		TextureSetIndex(const ref_ptr<Texture> &tex, GLuint objectIndex)
				: tex_(tex), objectIndex_(objectIndex) {}

		// override
		void enable(RenderState *rs) override { tex_->set_objectIndex(objectIndex_); }

	protected:
		ref_ptr<Texture> tex_;
		GLuint objectIndex_;
	};

	/**
	 * \brief Activates texture image when enabled.
	 */
	class TextureNextIndex : public TextureIndexState {
	public:
		/**
		 * @param tex texture reference.
		 */
		explicit TextureNextIndex(const ref_ptr<Texture> &tex) : tex_(tex) {}

		// override
		void enable(RenderState *rs) override { tex_->nextObject(); }

	protected:
		ref_ptr<Texture> tex_;
	};
} // namespace

#endif /* TEXTURE_NODE_H_ */
