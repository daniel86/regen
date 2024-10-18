/*
 * texture.h
 *
 *  Created on: 02.02.2011
 *      Author: daniel
 */

#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include <string>
#include <set>
#include <map>
#include <list>

#include <regen/gl-types/gl-object.h>
#include <regen/gl-types/render-state.h>
#include <regen/gl-types/shader-input.h>
#include <regen/gl-types/vbo.h>

namespace regen {
	template<typename T>
	void __lockedTextureParameter(GLenum key, const T &v) {}

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
				apply, __lockedTextureParameter), v_(v) {}

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
		Texture(GLuint numTextures = 1);

		virtual ~Texture();

		/**
		 * @return the texture channel or -1.
		 */
		GLint channel() const;

		/**
		 * Specifies the format of the pixel data.
		 * Accepted values are GL_COLOR_INDEX, GL_RED, GL_GREEN,
		 * GL_BLUE, GL_RGB, GL_BGR, GL_RGBA, GL_BGRA
		 */
		void set_format(GLenum format);

		/**
		 * Specifies the format of the pixel data.
		 * Accepted values are GL_COLOR_INDEX, GL_RED, GL_GREEN,
		 * GL_BLUE, GL_RGB, GL_BGR, GL_RGBA, GL_BGRA
		 */
		GLenum format() const;

		/**
		 * Specifies the number of color components in the texture.
		 * Accepted values are GL_R*, GL_RG*, GL_RGB* GL_RGBA*, GL_DEPTH_COMPONENT*,
		 * GL_SRGB*, GL_COMPRESSED_*.
		 */
		void set_internalFormat(GLint internalFormat);

		/**
		 * Specifies the number of color components in the texture.
		 * Accepted values are GL_R*, GL_RG*, GL_RGB* GL_RGBA*, GL_DEPTH_COMPONENT*,
		 * GL_SRGB*, GL_COMPRESSED_*.
		 */
		GLint internalFormat() const;

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
		void set_pixelType(GLuint pixelType);

		/**
		 * Specifies the data type of the pixel data.
		 */
		GLuint pixelType() const;

		/**
		 * Number of samples used for multisampling
		 */
		GLsizei numSamples() const;

		/**
		 * Number of samples used for multisampling
		 */
		void set_numSamples(GLsizei v);

		/**
		 * Number of components per texel.
		 */
		GLuint numComponents() const;

		/**
		 * Specifies a pointer to the image data in memory.
		 * Initially NULL.
		 */
		void set_data(GLvoid *data);

		/**
		 * Specifies a pointer to the image data in memory.
		 * Initially NULL.
		 */
		GLvoid *data() const;

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
		TextureParameterStack<TextureFilter> &filter() { return *filter_[objectIndex_]; }

		/**
		 * Sets the minimum and maximum level-of-detail parameter.  This value limits the
		 * selection of highest/lowest resolution mipmap. The initial values are -1000/1000.
		 */
		TextureParameterStack<TextureLoD> &lod() { return *lod_[objectIndex_]; }

		/**
		 * Sets the swizzle that will be applied to the rgba components of a texel before it is returned to the shader.
		 * Valid values for param are GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA, GL_ZERO and GL_ONE.
		 */
		TextureParameterStack<TextureSwizzle> &swizzle() { return *swizzle_[objectIndex_]; }

		/**
		 * Sets the wrap parameter for texture coordinates s,t,r to either GL_CLAMP,
		 * GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT, or
		 * GL_REPEAT.
		 */
		TextureParameterStack<TextureWrapping> &wrapping() { return *wrapping_[objectIndex_]; }

		/**
		 * Specifies the texture comparison mode for currently bound depth textures.
		 * That is, a texture whose internal format is GL_DEPTH_COMPONENT_*;
		 * Permissible values are: GL_COMPARE_R_TO_TEXTURE, GL_NONE
		 * And specifies the comparison operator used when
		 * mode is set to GL_COMPARE_R_TO_TEXTURE.
		 */
		TextureParameterStack<TextureCompare> &compare() { return *compare_[objectIndex_]; }

		/**
		 * Sets the index of the highest defined mipmap level. The initial value is 1000.
		 */
		TextureParameterStack<TextureMaxLevel> &maxLevel() { return *maxLevel_[objectIndex_]; }

		/**
		 * Sets GL_TEXTURE_MAX_ANISOTROPY.
		 */
		TextureParameterStack<TextureAniso> &aniso() { return *aniso_[objectIndex_]; }

		/**
		 * Generates mipmaps for the texture.
		 * Make sure to set the base level before.
		 * @param mode: Should be GL_NICEST, GL_DONT_CARE or GL_FASTEST
		 */
		void setupMipmaps(GLenum mode = GL_DONT_CARE) const;

		/**
		 * GLSL sampler type used for this texture.
		 */
		const std::string &samplerType() const;

		/**
		 * GLSL sampler type used for this texture.
		 */
		void set_samplerType(const std::string &samplerType);

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
		 * Specify the texture image.
		 */
		virtual void texImage() const = 0;

	protected:
		GLuint dim_;
		// format of pixel data
		GLenum format_;
		GLint internalFormat_;
		// type for pixels
		GLenum pixelType_;
		GLint border_;
		TextureBind texBind_;

		TextureParameterStack<TextureFilter> **filter_;
		TextureParameterStack<TextureLoD> **lod_;
		TextureParameterStack<TextureSwizzle> **swizzle_;
		TextureParameterStack<TextureWrapping> **wrapping_;
		TextureParameterStack<TextureCompare> **compare_;
		TextureParameterStack<TextureMaxLevel> **maxLevel_;
		TextureParameterStack<TextureAniso> **aniso_;

		// pixel data, or null for empty texture
		GLvoid *data_;
		// true if texture encodes data in tangent space.
		GLboolean isInTSpace_;

		GLuint numSamples_;

		std::string samplerType_;
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
		Texture1D(GLuint numTextures = 1);

		// override
		void texImage() const;
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
		Texture2D(GLuint numTextures = 1);

		// override
		virtual void texImage() const;
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
		TextureRectangle(GLuint numTextures = 1);
	};

	/**
	 * \brief Texture with depth format.
	 */
	class Texture2DDepth : public Texture2D {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		Texture2DDepth(GLuint numTextures = 1);
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
		Texture2DMultisample(
				GLsizei numSamples,
				GLuint numTextures = 1,
				GLboolean fixedLocations = GL_FALSE);

		// override
		void texImage() const;

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
		Texture2DMultisampleDepth(GLsizei numSamples, GLboolean fixedLocations = GL_FALSE);

		// override
		void texImage() const;

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
		TextureCube(GLuint numTextures = 1);

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
		void texImage() const;

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
		TextureCubeDepth(GLuint numTextures = 1);
	};

	/**
	 * \brief A 3 dimensional texture.
	 */
	class Texture3D : public Texture {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		Texture3D(GLuint numTextures = 1);

		/**
		 * @param depth the texture depth.
		 */
		void set_depth(GLuint depth);

		/**
		 * @return the texture depth.
		 */
		GLuint depth();

		/**
		 * Specify a single layer of the 3D texture.
		 * @param layer the texture layer.
		 * @param subData data for the layer.
		 */
		virtual void texSubImage(GLint layer, GLubyte *subData) const;

		// override
		virtual void texImage() const;

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
		Texture3DDepth(GLuint numTextures = 1);
	};

	/**
	 * \brief Array of two dimensional textures.
	 */
	class Texture2DArray : public Texture3D {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		Texture2DArray(GLuint numTextures = 1);
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
		TextureBuffer(GLenum texelFormat);

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

	private:
		GLenum texelFormat_;
		ref_ptr<VBO> attachedVBO_;
		VBOReference attachedVBORef_;

		// override
		virtual void texImage() const;
	};
} // namespace

#endif /* _TEXTURE_H_ */
