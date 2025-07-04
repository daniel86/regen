#ifndef REGEN_TEXTURE_H_
#define REGEN_TEXTURE_H_

#include <string>
#include <set>
#include <map>
#include <list>

#include <regen/gl-types/gl-rectangle.h>
#include <regen/gl-types/render-state.h>
#include <regen/gl-types/shader-input.h>
#include <regen/gl-types/vbo.h>
#include "regen/shapes/bounds.h"
#include "regen/scene/scene-input.h"
#include "regen/textures/texture-file.h"

namespace regen {
	class Texture;
	/** minification/magnifiction */
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
		explicit Texture(GLenum textureTarget, GLuint numTextures = 1);

		~Texture() override;

		Texture(const Texture &) = delete;

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
		inline uint32_t numComponents() const { return dim_; }

		/**
		 * @return the texture depth.
		 */
		inline uint32_t depth() const { return imageDepth_; }

		/**
		 * Specifies a pointer to the image data in memory.
		 * Initially NULL.
		 * @param data the image data.
		 * @param owned if true, the texture will take ownership of the data.
		 */
		void setTextureData(const GLubyte *data, bool owned = false);

		/**
		 * Specifies a pointer to the image data in memory.
		 * Initially NULL.
		 */
		inline auto *textureData() const { return textureData_; }

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
		void set_samplerType(const std::string &samplerType) { samplerType_ = samplerType; }

		/**
		 * @return true if the texture has a file name.
		 */
		bool hasTextureFile() const { return textureFile_.has_value(); }

		/**
		 * @return the texture file name.
		 */
		void set_textureFile(const std::string &fileName);

		/**
		 * @return the texture file name.
		 */
		void set_textureFile(const std::string &directory, const std::string &namePattern);

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
		void updateSubImage(GLint layer, GLubyte *subData);

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
		 * @param numComponents number of components per texel.
		 * @return average value.
		 */
		template<class T>
		T sampleAverage(const Vec2f &texco, const Vec2f &regionTS, const GLubyte *textureData) const {
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
		 * @param numComponents number of components per texel.
		 * @return max value.
		 */
		template<class T>
		T sampleMax(const Vec2f &texco, const Vec2f &regionTS, const GLubyte *textureData) const {
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
		 * Sample the nearest texel.
		 * @param uv texture coordinates.
		 * @param textureData texture data.
		 * @param numComponents number of components per texel.
		 * @return value.
		 */
		template<class T>
		T sampleNearest(const Vec2f &uv, const GLubyte *textureData) const {
			return sample<T>(texelIndex(uv), textureData);
		}

		/**
		 * Sample the nearest texel.
		 * @param coordinate texture coordinates.
		 * @param textureData texture data.
		 * @param numComponents number of components per texel.
		 * @return value.
		 */
		template<class T>
		T sampleNearest(const Vec2ui &coordinate, const GLubyte *textureData) const {
			return sample<T>(coordinate.y * width() + coordinate.x, textureData);
		}

		/**
		 * Sample linearly between closest texels.
		 * @param texco texture coordinates.
		 * @param textureData texture data.
		 * @param numComponents number of components per texel.
		 * @return value.
		 */
		template<class T>
		T sampleLinear(const Vec2f &uv, const GLubyte *textureData) const {
			auto w = static_cast<float>(width());
			auto h = static_cast<float>(height());
			float x = uv.x * w;
			float y = uv.y * h;
			float x0 = std::floor(x);
			float y0 = std::floor(y);
			auto i_x0 = static_cast<uint32_t>(x0);
			auto i_y0 = static_cast<uint32_t>(y0);
			uint32_t i_x1 = std::min(i_x0 + 1, width() - 1);
			uint32_t i_y1 = std::min(i_y0 + 1, height() - 1);

			T v00 = sampleNearest<T>(Vec2ui(i_x0, i_y0), textureData);
			T v01 = sampleNearest<T>(Vec2ui(i_x0, i_y1), textureData);
			T v10 = sampleNearest<T>(Vec2ui(i_x1, i_y0), textureData);
			T v11 = sampleNearest<T>(Vec2ui(i_x1, i_y1), textureData);

			auto dx = x - x0;
			auto dy = y - y0;
			T v0 = v00 * (1.0f - dx) + v10 * dx;
			T v1 = v01 * (1.0f - dx) + v11 * dx;
			return v0 * (1.0f - dy) + v1 * dy;
		}

		static void configure(ref_ptr<Texture> &tex, scene::SceneInputNode &input);

		static Vec3i getSize(const ref_ptr<ShaderInput2i> &viewport,
							 const std::string &sizeMode, const Vec3f &size);

	protected:
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
		GLboolean fixedSampleLocations_ = GL_TRUE;
		int32_t textureChannel_ = -1;
		TextureWrapping wrappingMode_ = Vec3i(GL_CLAMP_TO_EDGE);
		std::string samplerType_;
		std::optional<TextureFile> textureFile_;

		// client data, or null
		const GLubyte *textureData_;
		uint32_t numTexel_ = 0u;
		bool isTextureDataOwned_;

		void (Texture::*allocTexture_)();
		void (Texture::*updateImage_)(GLubyte *subData);
		void (Texture::*updateSubImage_)(GLint layer, GLubyte *subData);
		Vec3ui allocatedSize_ = Vec3ui(0u);

		void allocTexture1D();

		void allocTexture2D();

		void allocTexture2D_Multisample();

		void allocTexture3D();

		void allocTexture_noop() {}

		void updateImage1D(GLubyte *subData);

		void updateImage2D(GLubyte *subData);

		void updateImage3D(GLubyte *subData);

		void updateImage_noop(GLubyte*) {}

		void updateSubImage1D(GLint layer, GLubyte *subData);

		void updateSubImage2D(GLint layer, GLubyte *subData);

		void updateSubImage3D(GLint layer, GLubyte *subData);

		void updateSubImage_noop(GLint, GLubyte*) {}

		Bounds<Vec2ui> getRegion(const Vec2f &texco, const Vec2f &regionTS) const;

		unsigned int texelIndex(const Vec2f &texco) const;

		template<class T>
		T sample(unsigned int texelIndex, const GLubyte *textureData) const {
			auto *dataOffset = textureData + texelIndex * numComponents_;
			T v(0.0f);
			auto *typedData = (float *) &v;
			for (unsigned int i = 0; i < numComponents_; ++i) {
				typedData[i] = static_cast<float>(dataOffset[i]) / 255.0f;
			}
			return v;
		}
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
		explicit Texture1D(GLuint numTextures = 1);
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
				GLuint numTextures = 1);
	};

	/**
	 * \brief A texture with multiple mipmap textures.
	 *
	 * Note this is not using the GL mipmapping feature,
	 * downscaling must be done manually.
	 */
	class TextureMips2D : public Texture2D {
	public:
		explicit TextureMips2D(GLuint numMips = 4);

		auto &mipTextures() { return mipTextures_; }

		auto &mipRefs() { return mipRefs_; }

		auto numMips() const { return numMips_; }

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
		explicit TextureRectangle(GLuint numTextures = 1);
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
				GLuint numTextures = 1);
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
				GLuint numTextures = 1,
				GLboolean fixedLocations = GL_TRUE);
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
		explicit Texture2DMultisampleDepth(GLsizei numSamples, GLboolean fixedLocations = GL_TRUE);
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
				GLuint numTextures = 1);

		/**
		 * @param depth the texture depth.
		 */
		void set_depth(GLuint depth);
	};

	/**
	 * \brief A 3 dimensional depth texture.
	 */
	class Texture3DDepth : public Texture3D {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		explicit Texture3DDepth(GLuint numTextures = 1);
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
				GLuint numTextures = 1);
	};

	class Texture2DArrayDepth : public Texture2DArray {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		explicit Texture2DArrayDepth(GLuint numTextures = 1);
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
				GLuint numTextures = 1,
				GLboolean fixedLocations = GL_FALSE);
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
				GLuint numTextures = 1,
				GLboolean fixedLocations = GL_FALSE);
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
		explicit TextureCube(GLuint numTextures = 1);
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
		explicit TextureCubeDepth(GLuint numTextures = 1);
	};
} // namespace
#endif /* REGEN_TEXTURE_H_ */
