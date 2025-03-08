/*
 * font-manager.h
 *
 *  Created on: 15.03.2011
 *      Author: daniel
 */

#ifndef FONT_MANAGER_H_
#define FONT_MANAGER_H_

#include <GL/glew.h>

extern "C" {
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_TRIGONOMETRY_H
}

#include <stdexcept>
#include <vector>
#include <string>
#include <map>

#include <regen/textures/texture.h>
#include <regen/textures/texture-array.h>
#include <regen/math/vector.h>
#include <regen/utility/ref-ptr.h>
#include "regen/scene/loading-context.h"

namespace regen {
	/**
	 * \brief Freetype2 Font class, using texture mapped glyphs.
	 */
	class Font : public Resource {
	public:
		static constexpr const char *TYPE_NAME = "Font";

		/**
		 * \brief A font related error occurred.
		 */
		class Error : public std::runtime_error {
		public:
			/**
			 * @param msg the error message.
			 */
			Error(const std::string &msg) : std::runtime_error(msg) {}
		};

		/**
		 * Defines a glyph face.
		 */
		typedef struct {
			/** face width */
			GLfloat width;
			/** face height */
			GLfloat height;
			/** max uv.x in array texture (min is 0). */
			GLfloat uvX;
			/** max uv.y in array texture (min is 0). */
			GLfloat uvY;
			/** left margin */
			GLfloat left;
			/** top margin */
			GLfloat top;
			/** distance to net glyph */
			GLfloat advanceX;
		} FaceData;

		/**
		 * Get a font.
		 * @param filename path to font
		 * @param size font size, as usual
		 * @param dpi dots per inch for font
		 */
		static ref_ptr<Font> get(const std::string &filename, GLuint size, GLuint dpi = 96);

		/**
		 * Load a font.
		 * @param cfg settings for font
		 */
		static ref_ptr<Font> load(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * Call when you are done using fonts.
		 */
		static void closeLibrary();

		/**
		 * Default constructor.
		 * @param filename path to font
		 * @param size font size, as usual
		 * @param dpi dots per inch for font
		 */
		Font(const std::string &filename, GLuint size, GLuint dpi = 96);

		virtual ~Font();

		/**
		 * Height of a line of text. In unit space (maps font size to 1.0).
		 */
		auto lineHeight() const { return lineHeight_; }

		/**
		 * The font size. In pixels.
		 */
		auto size() const { return size_; }

		/**
		 * The texture used by this font.
		 * You can access the glyphs with the char as index.
		 */
		auto &texture() const { return arrayTexture_; }

		/**
		 * Character to face data.
		 */
		const FaceData &faceData(GLushort ch) const;

	private:
		typedef std::map<std::string, ref_ptr<Font> > FontMap;
		static FT_Library ftlib_;
		static FontMap fonts_;
		static GLboolean isFreetypeInitialized_;

		const std::string fontPath_;
		const GLuint size_;
		const GLuint dpi_;
		ref_ptr<Texture2DArray> arrayTexture_;
		FaceData *faceData_;
		GLfloat lineHeight_;

		Font(const Font &)
				: fontPath_(""),
				  size_(0),
				  dpi_(0),
				  faceData_(nullptr),
				  lineHeight_(0.0f) {}

		Font &operator=(const Font &) { return *this; }

		static GLubyte *invertPixmapWithAlpha(const FT_Bitmap &bitmap, GLuint width, GLuint height);

		void initGlyph(FT_Face face, GLushort ch, GLuint textureWidth, GLuint textureHeight);
	};
} // namespace

#endif /* FONT_MANAGER_H_ */
