#ifndef REGEN_TEXTURE_H_
#define REGEN_TEXTURE_H_

#include <string>
#include <regen/gl/gl-rectangle.h>
#include <regen/gl/render-state.h>
#include "regen/shader/shader-input.h"
#include "regen/shapes/bounds.h"
#include "regen/scene/scene-input.h"
#include "regen/textures/texture-file.h"
#include "regen/textures/image-data.h"

namespace regen {
	class Texture;
	/** minification/magnification */
	typedef Vec2i TextureFilter;
	/** min/max LoD. */
	typedef Vec2f TextureLoD;
	/** rgba swizzle mask. */
	typedef Vec4i TextureSwizzle;
	/** str wrapping mode. */
	typedef Vec3i TextureWrapping;
	/** compare mode/func */
	typedef Vec2i TextureCompare;
	typedef int32_t TextureMaxLevel;
	typedef float TextureAniso;

	/**
	 * \brief A OpenGL Object that contains one or more images
	 * that all have the same image format.
	 *
	 * A texture can be used in two ways.
	 * It can be the source of a texture access from a Shader,
	 * or it can be used as a render target.
	 */
	class Texture : public GLRectangle, public ShaderInput1i {
	public:
		static constexpr const char *TYPE_NAME = "Texture";

		/**
		 * @param numTextures number of texture images.
		 */
		explicit Texture(GLenum textureTarget, uint32_t numTextures = 1);

		/**
		 * Copy constructor.
		 */
		Texture(const Texture &other);

		/**
		 * Destructor.
		 */
		~Texture() override;

		/**
		 * Loads a texture from the given input node.
		 * @param ctx the loading context.
		 * @param input the input node.
		 * @return a new texture object.
		 */
		static ref_ptr<Texture> load(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * Assigns a texture channel (unit) to the texture.
		 * @param channel the texture channel.
		 */
		void setTextureChannel(int32_t channel);

		/**
		 * Sets the texture channel to -1.
		 * This means that the texture is not used in a shader.
		 */
		void clearTextureChannel() { setTextureChannel(-1); }

		/**
		 * @return the texture channel or -1.
		 */
		int textureChannel() const { return textureChannel_; }

		/**
		 * Specifies the format of the pixel data.
		 * Accepted values are GL_COLOR_INDEX, GL_RED, GL_GREEN,
		 * GL_BLUE, GL_RGB, GL_BGR, GL_RGBA, GL_BGRA
		 */
		void set_format(GLenum format) {
			format_ = format;
			numComponents_ = glenum::pixelComponents(format);
		}

		/**
		 * Specifies the format of the pixel data.
		 * Accepted values are GL_COLOR_INDEX, GL_RED, GL_GREEN,
		 * GL_BLUE, GL_RGB, GL_BGR, GL_RGBA, GL_BGRA
		 */
		auto format() const { return format_; }

		/**
		 * Specifies the number of color components in the texture.
		 * Accepted values are GL_R*, GL_RG*, GL_RGB* GL_RGBA*, GL_DEPTH_COMPONENT*,
		 * GL_SRGB*, GL_COMPRESSED_*.
		 */
		void set_internalFormat(GLenum internalFormat) { internalFormat_ = internalFormat; }

		/**
		 * Specifies the number of color components in the texture.
		 * Accepted values are GL_R*, GL_RG*, GL_RGB* GL_RGBA*, GL_DEPTH_COMPONENT*,
		 * GL_SRGB*, GL_COMPRESSED_*.
		 */
		auto internalFormat() const { return internalFormat_; }

		/**
		 * Binds a named texture to a texturing target.
		 */
		const TextureBind &textureBind();

		/**
		 * Specifies the target texture. Accepted values are GL_TEXTURE_2D,
		 * GL_PROXY_TEXTURE_2D, GL_TEXTURE_CUBE_MAP_POSITIVE*,
		 * GL_PROXY_TEXTURE_CUBE_MAP.
		 */
		GLenum targetType() const;

		/**
		 * Specifies the target texture. Accepted values are GL_TEXTURE_2D,
		 * GL_PROXY_TEXTURE_2D, GL_TEXTURE_CUBE_MAP_POSITIVE*,
		 * GL_PROXY_TEXTURE_CUBE_MAP.
		 */
		void set_targetType(GLenum targetType);

		/**
		 * Specifies the data type of the pixel data.
		 */
		void set_pixelType(uint32_t pixelType) { pixelType_ = pixelType; }

		/**
		 * Specifies the data type of the pixel data.
		 */
		inline uint32_t pixelType() const { return pixelType_; }

		/**
		 * Number of samples used for multisampling
		 */
		inline int32_t numSamples() const { return numSamples_; }

		/**
		 * Number of samples used for multisampling
		 */
		void set_numSamples(int32_t v) { numSamples_ = v; }

		/**
		 * Number of components per texel.
		 */
		inline uint32_t textureDimension() const { return dim_; }

		/**
		 * @return the texture depth.
		 */
		inline uint32_t depth() const { return imageDepth_; }

		/**
		 * Sets the image data for the texture.
		 * @param data the image data.
		 */
		void setTextureData(const ref_ptr<ImageData> &data);

		/**
		 * Unsets the image data for the texture.
		 */
		void unsetTextureData();

		/**
		 * @return image data for the texture, if any stored in RAM.
		 */
		const ref_ptr<ImageData> &textureData() const { return textureData_; }

		/**
		 * Reads the texture data from the server.
		 */
		void updateTextureData();

		/**
		 * Ensures that the texture data is available.
		 * If the texture data is not available, it will be read from the server.
		 */
		void ensureTextureData();

		/**
		 * Sets magnification and minifying parameters.
		 *
		 * The texture magnification function is used when the pixel being textured
		 * maps to an area less than or equal to one texture element.
		 * It sets the texture magnification function to
		 * either GL_NEAREST or GL_LINEAR.
		 *
		 * The texture minifying function is used whenever the pixel
		 * being textured maps to an area greater than one texture element.
		 * There are six defined minifying functions.
		 * Two of them use the nearest one or nearest four texture elements
		 * to compute the texture value. The other four use mipmaps.
		 * Accepted values are GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST,
		 * GL_LINEAR_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR,
		 * GL_LINEAR_MIPMAP_LINEAR.
		 */
		void set_filter(const TextureFilter &v);

		/**
		 * Sets the minimum and maximum level-of-detail parameter.  This value limits the
		 * selection of highest/lowest resolution mipmap. The initial values are -1000/1000.
		 */
		void set_lod(const TextureLoD &v);

		/**
		 * Sets the swizzle that will be applied to the rgba components of a texel before it is returned to the shader.
		 * Valid values for param are GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA, GL_ZERO and GL_ONE.
		 */
		void set_swizzle(const TextureSwizzle &v);

		/**
		 * Sets the wrap parameter for texture coordinates s,t,r to either GL_CLAMP,
		 * GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT, or
		 * GL_REPEAT.
		 */
		void set_wrapping(const TextureWrapping &v);

		/**
		 * Specifies the texture comparison mode for currently bound depth textures.
		 * That is, a texture whose internal format is GL_DEPTH_COMPONENT_*;
		 * Permissible values are: GL_COMPARE_R_TO_TEXTURE, GL_NONE
		 * And specifies the comparison operator used when
		 * mode is set to GL_COMPARE_R_TO_TEXTURE.
		 */
		void set_compare(const TextureCompare &v);

		/**
		 * Sets the index of the highest defined mipmap level. The initial value is 1000.
		 */
		void set_maxLevel(const TextureMaxLevel &v);

		/**
		 * Sets GL_TEXTURE_MAX_ANISOTROPY.
		 */
		void set_aniso(const TextureAniso &v);

		/**
		 * GLSL sampler type used for this texture.
		 */
		const std::string &samplerType() const { return samplerType_; }

		/**
		 * GLSL sampler type used for this texture.
		 */
		void set_samplerType(std::string_view samplerType) { samplerType_ = samplerType; }

		/**
		 * @return true if the texture has a file name.
		 */
		bool hasTextureFile() const { return textureFile_.has_value(); }

		/**
		 * @return the texture file name.
		 */
		void set_textureFile(std::string_view fileName);

		/**
		 * @return the texture file name.
		 */
		void set_textureFile(std::string_view directory, std::string_view namePattern);

		/**
		 * @return the texture file name.
		 */
		auto &textureFile() const { return textureFile_; }

		/**
		 * Specify the texture image.
		 */
		void allocTexture();

		/**
		 * Allocates the texture image.
		 * This will allocate the texture data on the GPU.
		 */
		void updateImage(GLubyte *data);

		/**
		 * Allocates the texture image.
		 * This will allocate the texture data on the GPU.
		 */
		void updateSubImage(int32_t layer, GLubyte *subData);

		/**
		 * Sets the number of mipmaps.
		 * If the value is negative, it will be computed from the texture size.
		 * @param numMips number of mipmaps.
		 */
		void setNumMipmaps(int32_t numMips);

		/**
		 * @return number of mipmaps.
		 */
		int32_t getNumMipmaps();

		/**
		 * Generates mipmaps for the texture.
		 * Make sure to set the base level before, and that the texture is allocated.
		 */
		void updateMipmaps();

		/**
		 * @return number of texel.
		 */
		inline uint32_t numTexel() const { return numTexel_; }

		/**
		 * Ensures the texture is bound to a texture unit.
		 */
		void bind();

		/**
		 * Sample a region, and return the average value.
		 * @param texco texture coordinates.
		 * @param regionTS region size in texture space.
		 * @param textureData texture data.
		 * @return average value.
		 */
		template<class T>
		T sampleAverage(const Vec2f &texco, const Vec2f &regionTS, const ref_ptr<ImageData> &textureData) const {
			auto bounds = getRegion(texco, regionTS);
			T avg = T(0.0f);
			int numSamples = 0;
			for (unsigned int y = bounds.min.y; y <= bounds.max.y; ++y) {
				for (unsigned int x = bounds.min.x; x <= bounds.max.x; ++x) {
					unsigned int index = (y * width() + x);
					avg += sample<T>(index, textureData);
					numSamples++;
				}
			}
			if (numSamples > 0) {
				return avg / static_cast<float>(numSamples);
			} else {
				return avg;
			}
		}

		/**
		 * Sample a region, and return the max value.
		 * @param texco texture coordinates.
		 * @param regionTS region size in texture space.
		 * @param textureData texture data.
		 * @return max value.
		 */
		template<class T>
		T sampleMax(const Vec2f &texco, const Vec2f &regionTS, const ref_ptr<ImageData> &textureData) const {
			auto bounds = getRegion(texco, regionTS);
			T maxVal = T(0.0f);
			for (unsigned int y = bounds.min.y; y <= bounds.max.y; ++y) {
				for (unsigned int x = bounds.min.x; x <= bounds.max.x; ++x) {
					unsigned int index = (y * width() + x);
					maxVal = std::max(maxVal, sample<T>(index, textureData));
				}
			}
			return maxVal;
		}

		/**
		 * Sample a region, and return the max value.
		 * @param texco texture coordinates.
		 * @param regionTS region size in texture space.
		 * @param textureData texture data.
		 * @return max value.
		 */
		template<class T>
		float sampleMax(const Vec2f &texco, const Vec2f &regionTS, const ref_ptr<ImageData> &textureData, uint32_t componentIdx) const {
			auto bounds = getRegion(texco, regionTS);
			float maxVal = 0.0f;
			for (unsigned int y = bounds.min.y; y <= bounds.max.y; ++y) {
				for (unsigned int x = bounds.min.x; x <= bounds.max.x; ++x) {
					unsigned int index = (y * width() + x);
					float v = sample<T>(index, textureData)[componentIdx];
					maxVal = std::max(maxVal, v);
				}
			}
			return maxVal;
		}

		/**
		 * Sample the nearest texel.
		 * @param uv texture coordinates.
		 * @param textureData texture data.
		 * @return value.
		 */
		template<class T>
		T sampleNearest(const Vec2f &uv, const ref_ptr<ImageData> &textureData) const {
			return sample<T>(texelIndex(uv), textureData);
		}

		/**
		 * Sample the nearest texel.
		 * @param coordinate texture coordinates.
		 * @param textureData texture data.
		 * @return value.
		 */
		template<class T>
		T sampleNearest(const Vec2ui &coordinate, const ref_ptr<ImageData> &textureData) const {
			return sample<T>(coordinate.y * width() + coordinate.x, textureData);
		}

		/**
		 * Sample linearly between closest texels.
		 * @param uv texture coordinates.
		 * @param textureData texture data.
		 * @return value.
		 */
		template<class T, uint32_t NumComponents>
		T sampleLinear(const Vec2f &uv, const ref_ptr<ImageData> &textureData) const {
			const auto i_w = static_cast<int32_t>(width());
			const auto i_h = static_cast<int32_t>(height());
			float f_x = std::clamp(uv.x,0.0f,1.0f) * static_cast<float>(i_w);
			float f_y = std::clamp(uv.y,0.0f,1.0f) * static_cast<float>(i_h);

			const auto i_x0 = std::min(static_cast<int32_t>(f_x), i_w - 1);
			const auto i_y0 = std::min(static_cast<int32_t>(f_y), i_h - 1);
			const int32_t i_x1 = std::min(i_x0 + 1, i_w - 1);
			const int32_t i_y1 = std::min(i_y0 + 1, i_h - 1);
			const int32_t indices[4] = {
				i_y0 * i_w + i_x0,
				i_y1 * i_w + i_x0,
				i_y0 * i_w + i_x1,
				i_y1 * i_w + i_x1 };

			T vals[4] = {
				Vec::create<T>(0.0f),
				Vec::create<T>(0.0f),
				Vec::create<T>(0.0f),
				Vec::create<T>(0.0f)
			};
			auto *vals_f = reinterpret_cast<float *>(&vals[0]);
			for (uint32_t valIdx=0; valIdx < 4; ++valIdx) {
				float *vals_ff = vals_f + valIdx * NumComponents;
				auto *textureData_ff = textureData->floatPixels + indices[valIdx] * NumComponents;
				for (unsigned int i = 0; i < NumComponents; ++i) {
					vals_ff[i] = textureData_ff[i];
				}
			}

			f_x -= static_cast<float>(i_x0);
			f_y -= static_cast<float>(i_y0);
			return
				(vals[0] * (1.0f - f_x) + vals[2] * f_x) * (1.0f - f_y) +
				(vals[1] * (1.0f - f_x) + vals[3] * f_x) * f_y;
		}

		/**
		 * Sample a texel at the given index.
		 * @param texelIndex index of the texel.
		 * @param textureData texture data.
		 * @return value.
		 */
		template<class T>
		T sample(unsigned int texelIndex, const ref_ptr<ImageData> &textureData) const {
			T v = Vec::create<T>(0.0f);
			auto *typedData = (float *) &v;
			auto *floatData = textureData->floatPixels + texelIndex * numComponents_;
			for (unsigned int i = 0; i < numComponents_; ++i) {
				typedData[i] = floatData[i];
			}
			return v;
		}

		Bounds<Vec2ui> getRegion(const Vec2f &texco, const Vec2f &regionTS) const;

		static void configure(ref_ptr<Texture> &tex, scene::SceneInputNode &input);

		static Vec3i getSize(const Vec2i &viewport,
							 const std::string &sizeMode, const Vec3f &size);

	protected:
		// an atomic copy counter, for protecting deletion of shared data
		uint32_t dim_;
		// format of pixel data
		GLenum format_;
		GLenum internalFormat_;
		uint32_t numComponents_ = 4; // number of components per texel
		uint32_t imageDepth_ = 1;
		int32_t numMips_ = -1;
		// type for pixels
		GLenum pixelType_;
		int32_t border_ = 0;
		TextureBind texBind_;
		int32_t numSamples_ = 1;
		bool fixedSampleLocations_ = true;
		int32_t textureChannel_ = -1;
		std::string samplerType_;
		std::optional<TextureFile> textureFile_;

		// texture state
		TextureFilter texFilter_ = Vec2i(GL_LINEAR, GL_LINEAR);
		TextureWrapping wrappingMode_ = Vec3i(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
		std::optional<TextureLoD> texLoD_ = std::nullopt;
		std::optional<TextureSwizzle> texSwizzle_ = std::nullopt;
		std::optional<TextureCompare> texCompare_ = std::nullopt;
		std::optional<TextureMaxLevel> texMaxLevel_ = std::nullopt;
		std::optional<TextureAniso> texAniso_ = std::nullopt;

		// client data, or null
		ref_ptr<ImageData> textureData_;
		uint32_t numTexel_ = 0u;

		void (Texture::*allocTexture_)();
		void (Texture::*updateImage_)(GLubyte *subData);
		void (Texture::*updateSubImage_)(int layer, GLubyte *subData);
		Vec3ui allocatedSize_ = Vec3ui::zero();

		void allocTexture1D();

		void allocTexture2D();

		void allocTexture2D_Multisample();

		void allocTexture3D();

		void allocTexture_noop() {}

		void updateImage1D(GLubyte *subData);

		void updateImage2D(GLubyte *subData);

		void updateImage3D(GLubyte *subData);

		void updateImage_noop(GLubyte*) {}

		void updateSubImage1D(int layer, GLubyte *subData);

		void updateSubImage2D(int layer, GLubyte *subData);

		void updateSubImage3D(int layer, GLubyte *subData);

		void updateSubImage_noop(int, GLubyte*) {}

		unsigned int texelIndex(const Vec2f &texco) const;

	private:
		ref_ptr<std::atomic<uint32_t>> texCopyCounter_;
	};

	/**
	 * \brief Images in this texture are all 1-dimensional.
	 *
	 * They have width, but no height or depth.
	 */
	class Texture1D : public Texture {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		explicit Texture1D(uint32_t numTextures = 1);
	};

	/**
	 * \brief Images in this texture all are 2-dimensional.
	 *
	 * They have width and height, but no depth.
	 */
	class Texture2D : public Texture {
	public:
		static constexpr const char *TYPE_NAME = "Texture2D";

		/**
		 * @param numTextures number of texture images.
		 */
		explicit Texture2D(
				GLenum textureTarget = GL_TEXTURE_2D,
				uint32_t numTextures = 1);
	};

	/**
	 * \brief A texture with multiple mipmap textures.
	 *
	 * Note this is not using the GL mipmapping feature,
	 * downscaling must be done manually.
	 */
	class TextureMips2D : public Texture2D {
	public:
		explicit TextureMips2D(uint32_t numMips = 4);

		std::vector<Texture *> &mipTextures() { return mipTextures_; }

		std::vector<ref_ptr<Texture2D>> &mipRefs() { return mipRefs_; }

		uint32_t numCustomMips() const { return mipTextures_.size(); }

	protected:
		std::vector<Texture *> mipTextures_;
		std::vector<ref_ptr<Texture2D>> mipRefs_;
	};

	/**
	 * \brief The image in this texture (only one image. No mipmapping)
	 * is 2-dimensional.
	 *
	 * Texture coordinates used for these
	 * textures are not normalized.
	 */
	class TextureRectangle : public Texture2D {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		explicit TextureRectangle(uint32_t numTextures = 1);
	};

	/**
	 * \brief Texture with depth format.
	 */
	class Texture2DDepth : public Texture2D {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		explicit Texture2DDepth(
				GLenum textureTarget = GL_TEXTURE_2D,
				uint32_t numTextures = 1);
	};

	/**
	 * \brief The image in this texture (only one image. No mipmapping) is 2-dimensional.
	 *
	 * Each pixel in these images contains multiple samples instead
	 * of just one value.
	 */
	class Texture2DMultisample : public Texture2D {
	public:
		/**
		 * @param numSamples number of samples per texel.
		 * @param numTextures number of texture images.
		 * @param fixedLocations use fixed locations.
		 */
		explicit Texture2DMultisample(
				GLsizei numSamples,
				uint32_t numTextures = 1,
				bool fixedLocations = true);
	};

	/**
	 * \brief The image in this texture (only one image. No mipmapping) is 2-dimensional.
	 *
	 * Each pixel in these images contains multiple samples instead
	 * of just one value.
	 * Uses a depth format.
	 */
	class Texture2DMultisampleDepth : public Texture2DDepth {
	public:
		/**
		 * @param numSamples number of samples per texel.
		 * @param fixedLocations use fixed locations.
		 */
		explicit Texture2DMultisampleDepth(GLsizei numSamples, bool fixedLocations = true);
	};

	/**
	 * \brief A 3 dimensional texture.
	 */
	class Texture3D : public Texture {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		explicit Texture3D(
				GLenum textureTarget = GL_TEXTURE_3D,
				uint32_t numTextures = 1);

		/**
		 * @param depth the texture depth.
		 */
		void set_depth(uint32_t depth);
	};

	/**
	 * \brief A 3 dimensional depth texture.
	 */
	class Texture3DDepth : public Texture3D {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		explicit Texture3DDepth(uint32_t numTextures = 1);
	};

	/**
	 * \brief Array of two dimensional textures.
	 */
	class Texture2DArray : public Texture3D {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		explicit Texture2DArray(
				GLenum textureTarget = GL_TEXTURE_2D_ARRAY,
				uint32_t numTextures = 1);
	};

	class Texture2DArrayDepth : public Texture2DArray {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		explicit Texture2DArrayDepth(uint32_t numTextures = 1);
	};

	/**
	 * \brief Array of two dimensional textures with multiple samples.
	 */
	class Texture2DArrayMultisample : public Texture2DArray {
	public:
		/**
		 * @param numSamples number of samples per texel.
		 * @param numTextures number of texture images.
		 * @param fixedLocations use fixed locations.
		 */
		explicit Texture2DArrayMultisample(
				int32_t numSamples,
				uint32_t numTextures = 1,
				bool fixedLocations = false);
	};

	/**
	 * \brief Array of two dimensional textures with multiple samples.
	 */
	class Texture2DArrayMultisampleDepth : public Texture2DArray {
	public:
		/**
		 * @param numSamples number of samples per texel.
		 * @param numTextures number of texture images.
		 * @param fixedLocations use fixed locations.
		 */
		explicit Texture2DArrayMultisampleDepth(
				int32_t numSamples,
				uint32_t numTextures = 1,
				bool fixedLocations = false);
	};

	/**
	 * \brief Texture with exactly 6 distinct sets of 2D images,
	 * all of the same size.
	 *
	 * They act as 6 faces of a cube.
	 */
	class TextureCube : public Texture2D {
	public:
		/**
		 * \brief Defines the sides of a cube.
		 */
		enum CubeSide {
			RIGHT = 0,//!< the right side
			LEFT,    //!< the left side
			TOP,    //!< the top side
			BOTTOM, //!< the bottom side
			FRONT,  //!< the front side
			BACK,   //!< the back side
		};

		/**
		 * @param numTextures number of texture images.
		 */
		explicit TextureCube(uint32_t numTextures = 1);
	};

	/**
	 * \brief Texture with exactly 6 distinct sets of 2D images,
	 * all of the same size.
	 */
	class TextureCubeDepth : public TextureCube {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		explicit TextureCubeDepth(uint32_t numTextures = 1);
	};
} // namespace
#endif /* REGEN_TEXTURE_H_ */
