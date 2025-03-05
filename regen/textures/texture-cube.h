#ifndef REGEN_TEXTURE_CUBE_H_
#define REGEN_TEXTURE_CUBE_H_

#include <regen/textures/texture-2d.h>

namespace regen {
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
} // namespace

#endif /* REGEN_TEXTURE_CUBE_H_ */
