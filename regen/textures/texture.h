/*
 * texture.h
 *
 *  Created on: 02.02.2011
 *      Author: daniel
 */

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

namespace regen {
	template<typename T>
	void regen_lockedTextureParameter(GLenum key, const T &v) {}

	/**
	 * \brief State stack for texture parameters.
	 */
	template<typename T>
	class TextureParameterStack
			: public StateStack<TextureParameterStack<T>, T, void (*)(GLenum, const T &)> {
	public:
		/**
		 * @param v the texture target.
		 * @param apply apply a stack value.
		 */
		TextureParameterStack(const TextureBind &v, void (*apply)(GLenum, const T &))
				: StateStack<TextureParameterStack, T, void (*)(GLenum, const T &)>(
				apply, regen_lockedTextureParameter), v_(v) {}

		/**
		 * @param v the new state value
		 */
		void apply(const T &v) { this->apply_(v_.target_, v); }

	protected:
		const TextureBind &v_;
	};
}

namespace regen {
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
	typedef GLint TextureMaxLevel;
	typedef GLfloat TextureAniso;

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
		/**
		 * @param numTextures number of texture images.
		 */
		explicit Texture(GLuint numTextures = 1);

		~Texture() override;

		Texture(const Texture &) = delete;

		/**
		 * @return the texture channel or -1.
		 */
		GLint channel() const;

		/**
		 * Specifies the format of the pixel data.
		 * Accepted values are GL_COLOR_INDEX, GL_RED, GL_GREEN,
		 * GL_BLUE, GL_RGB, GL_BGR, GL_RGBA, GL_BGRA
		 */
		void set_format(GLenum format) { format_ = format; }

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
		void set_internalFormat(GLint internalFormat) { internalFormat_ = internalFormat; }

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
		void set_pixelType(GLuint pixelType) { pixelType_ = pixelType; }

		/**
		 * Specifies the data type of the pixel data.
		 */
		auto pixelType() const { return pixelType_; }

		/**
		 * Number of samples used for multisampling
		 */
		auto numSamples() const { return numSamples_; }

		/**
		 * Number of samples used for multisampling
		 */
		void set_numSamples(GLsizei v) { numSamples_ = v; }

		/**
		 * Number of components per texel.
		 */
		GLuint numComponents() const { return dim_; }

		/**
		 * Specifies a pointer to the image data in memory.
		 * Initially NULL.
		 * @param data the image data.
		 * @param owned if true, the texture will take ownership of the data.
		 */
		void set_textureData(GLubyte *data, bool owned = false);

		/**
		 * Specifies a pointer to the image data in memory.
		 * Initially NULL.
		 */
		auto *textureData() const { return textureData_; }

		/**
		 * Reads the texture data from the server.
		 */
		void readTextureData();

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
		auto &filter() { return *filter_[objectIndex_]; }

		/**
		 * Sets the minimum and maximum level-of-detail parameter.  This value limits the
		 * selection of highest/lowest resolution mipmap. The initial values are -1000/1000.
		 */
		auto &lod() { return *lod_[objectIndex_]; }

		/**
		 * Sets the swizzle that will be applied to the rgba components of a texel before it is returned to the shader.
		 * Valid values for param are GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA, GL_ZERO and GL_ONE.
		 */
		auto &swizzle() { return *swizzle_[objectIndex_]; }

		/**
		 * Sets the wrap parameter for texture coordinates s,t,r to either GL_CLAMP,
		 * GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT, or
		 * GL_REPEAT.
		 */
		auto &wrapping() { return *wrapping_[objectIndex_]; }

		/**
		 * Specifies the texture comparison mode for currently bound depth textures.
		 * That is, a texture whose internal format is GL_DEPTH_COMPONENT_*;
		 * Permissible values are: GL_COMPARE_R_TO_TEXTURE, GL_NONE
		 * And specifies the comparison operator used when
		 * mode is set to GL_COMPARE_R_TO_TEXTURE.
		 */
		auto &compare() { return *compare_[objectIndex_]; }

		/**
		 * Sets the index of the highest defined mipmap level. The initial value is 1000.
		 */
		auto &maxLevel() { return *maxLevel_[objectIndex_]; }

		/**
		 * Sets GL_TEXTURE_MAX_ANISOTROPY.
		 */
		auto &aniso() { return *aniso_[objectIndex_]; }

		/**
		 * Generates mipmaps for the texture.
		 * Make sure to set the base level before.
		 * @param mode: Should be GL_NICEST, GL_DONT_CARE or GL_FASTEST
		 */
		void setupMipmaps(GLenum mode = GL_DONT_CARE) const;

		/**
		 * GLSL sampler type used for this texture.
		 */
		const std::string &samplerType() const { return samplerType_; }

		/**
		 * GLSL sampler type used for this texture.
		 */
		void set_samplerType(const std::string &samplerType) { samplerType_ = samplerType; }

		/**
		 * Activates and binds this texture.
		 * Call end when you are done.
		 */
		void begin(RenderState *rs, GLint channel = 7);

		/**
		 * Complete previous call to begin.
		 */
		void end(RenderState *rs, GLint channel = 7);

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
		 * @param texco texture coordinates.
		 * @param textureData texture data.
		 * @param numComponents number of components per texel.
		 * @return value.
		 */
		template<class T>
		T sampleNearest(const Vec2f &texco, const GLubyte *textureData) const {
			return sample<T>(texelIndex(texco), textureData);
		}

		/**
		 * Sample linearly between closest texels.
		 * @param texco texture coordinates.
		 * @param textureData texture data.
		 * @param numComponents number of components per texel.
		 * @return value.
		 */
		template<class T>
		T sampleLinear(const Vec2f &texco, const GLubyte *textureData) const {
			auto w = static_cast<float>(width());
			auto h = static_cast<float>(height());
			auto x = texco.x * w;
			auto y = texco.y * h;
			auto x0 = std::floor(x);
			auto y0 = std::floor(y);
			auto x1 = x0 + 1;
			auto y1 = y0 + 1;

			T v00 = sampleNearest<T>(Vec2f(x0 / w, y0 / h), textureData);
			T v01 = sampleNearest<T>(Vec2f(x0 / w, y1 / h), textureData);
			T v10 = sampleNearest<T>(Vec2f(x1 / w, y0 / h), textureData);
			T v11 = sampleNearest<T>(Vec2f(x1 / w, y1 / h), textureData);

			auto dx = x - x0;
			auto dy = y - y0;
			T v0 = v00 * (1.0f - dx) + v10 * dx;
			T v1 = v01 * (1.0f - dx) + v11 * dx;
			return v0 * (1.0f - dy) + v1 * dy;
		}

		/**
		 * Specify the texture image.
		 */
		virtual void texImage() const = 0;

		/**
		 * @return number of texel.
		 */
		virtual unsigned int numTexel() const = 0;

		/**
		 * Resize the texture.
		 */
		virtual void resize(unsigned int width, unsigned int height);

	protected:
		GLuint dim_;
		// format of pixel data
		GLenum format_;
		GLint internalFormat_;
		// type for pixels
		GLenum pixelType_;
		GLint border_;
		TextureBind texBind_;
		GLuint numSamples_;
		std::string samplerType_;

		TextureParameterStack<TextureFilter> **filter_;
		TextureParameterStack<TextureLoD> **lod_;
		TextureParameterStack<TextureSwizzle> **swizzle_;
		TextureParameterStack<TextureWrapping> **wrapping_;
		TextureParameterStack<TextureCompare> **compare_;
		TextureParameterStack<TextureMaxLevel> **maxLevel_;
		TextureParameterStack<TextureAniso> **aniso_;

		// client data, or null
		const GLubyte *textureData_;
		bool isTextureDataOwned_;

		Bounds<Vec2ui> getRegion(const Vec2f &texco, const Vec2f &regionTS) const;

		unsigned int texelIndex(const Vec2f &texco) const;

		template<class T>
		T sample(unsigned int texelIndex, const GLubyte *textureData) const {
			auto numComponents = glenum::pixelComponents(format());
			auto *dataOffset = textureData + texelIndex * numComponents;
			T v(0.0f);
			auto *typedData = (float *) &v;
			for (unsigned int i = 0; i < numComponents; ++i) {
				typedData[i] = static_cast<float>(dataOffset[i]) / 255.0f;
			}
			return v;
		}
	};

	/**
	 * \brief Scoped activation of a texture.
	 */
	class ScopedTextureActivation {
	public:
		ScopedTextureActivation(Texture &tex, RenderState *rs, GLint channel = 7)
				: tex_(tex), rs_(rs), channel_(channel) {
			wasActive_ = tex_.active();
			if (!wasActive_) {
				tex_.begin(rs_, channel_);
			}
		}

		~ScopedTextureActivation() {
			if (!wasActive_) {
				tex_.end(rs_, channel_);
			}
		}

	private:
		Texture &tex_;
		RenderState *rs_;
		GLint channel_;
		bool wasActive_;
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

		// override
		void texImage() const override;

		// override
		unsigned int numTexel() const override { return width(); }
	};

	/**
	 * \brief Images in this texture all are 2-dimensional.
	 *
	 * They have width and height, but no depth.
	 */
	class Texture2D : public Texture {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		explicit Texture2D(GLuint numTextures = 1);

		// override
		void texImage() const override;

		// override
		unsigned int numTexel() const override { return width() * height(); }
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
		std::vector<Texture*> mipTextures_;
		std::vector<ref_ptr<Texture2D>> mipRefs_;
		GLuint numMips_;
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
		explicit Texture2DDepth(GLuint numTextures = 1);
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
				GLboolean fixedLocations = GL_FALSE);

		// override
		void texImage() const override;

	private:
		GLboolean fixedsamplelocations_;
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
		explicit Texture2DMultisampleDepth(GLsizei numSamples, GLboolean fixedLocations = GL_FALSE);

		// override
		void texImage() const override;

	private:
		GLboolean fixedsamplelocations_;
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
			FRONT,//!< the front side
			BACK, //!< the back side
			LEFT, //!< the left side
			RIGHT,//!< the right side
			TOP,  //!< the top side
			BOTTOM//!< the bottom side
		};

		/**
		 * @param numTextures number of texture images.
		 */
		explicit TextureCube(GLuint numTextures = 1);

		/**
		 * Sets texture data for a single cube side.
		 * @param side
		 * @param data
		 */
		void set_data(CubeSide side, void *data);

		/**
		 * Uploads data of a single cube side to GL.
		 * @param side
		 */
		void cubeTexImage(CubeSide side) const;

		/**
		 * Array of texture data for each cube side.
		 */
		void **cubeData();

		// override
		void texImage() const override;

		// override
		unsigned int numTexel() const override { return width() * height() * 6; }

	protected:
		void *cubeData_[6];
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

	/**
	 * \brief A 3 dimensional texture.
	 */
	class Texture3D : public Texture {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		explicit Texture3D(GLuint numTextures = 1);

		/**
		 * @param depth the texture depth.
		 */
		void set_depth(GLuint depth);

		/**
		 * @return the texture depth.
		 */
		GLuint depth() const { return numTextures_; }

		/**
		 * Specify a single layer of the 3D texture.
		 * @param layer the texture layer.
		 * @param subData data for the layer.
		 */
		virtual void texSubImage(GLint layer, GLubyte *subData) const;

		// override
		void texImage() const override;

		// override
		unsigned int numTexel() const override { return width() * height() * depth(); }

	protected:
		GLuint numTextures_;
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
		explicit Texture2DArray(GLuint numTextures = 1);
	};
} // namespace

namespace regen {
	/**
	 * \brief One-dimensional arrays of texels whose storage
	 * comes from an attached buffer object.
	 *
	 * When a buffer object is bound to a buffer texture,
	 * a format is specified, and the data in the buffer object
	 * is treated as an array of texels of the specified format.
	 */
	class TextureBuffer : public Texture {
	public:
		/**
		 * Accepted values are GL_R*, GL_RG*, GL_RGB* GL_RGBA*, GL_DEPTH_COMPONENT*,
		 * GL_SRGB*, GL_COMPRESSED_*.
		 */
		explicit TextureBuffer(GLenum texelFormat);

		/**
		 * Attach VBO to TextureBuffer and keep a reference on the VBO.
		 */
		void attach(const ref_ptr<VBO> &vbo, VBOReference &ref);

		/**
		 * Attach the storage for a buffer object to the active buffer texture.
		 */
		void attach(GLuint storage);

		/**
		 * Attach the storage for a buffer object to the active buffer texture.
		 */
		void attach(GLuint storage, GLuint offset, GLuint size);

		// override
		unsigned int numTexel() const override;


	private:
		GLenum texelFormat_;
		ref_ptr<VBO> attachedVBO_;
		VBOReference attachedVBORef_;

		// override
		void texImage() const override;
	};
} // namespace

#endif /* REGEN_TEXTURE_H_ */
