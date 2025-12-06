#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <regen/utility/strings.h>
#include <regen/gl/gl-util.h>
#include <regen/gl/render-state.h>

#include "font.h"
#include "regen/utility/filesystem.h"

using namespace regen;

// number of glyphs that will be loaded for each face
#define NUMBER_OF_GLYPHS 256

FT_Library Font::ftlib_ = FT_Library();
Font::FontMap Font::fonts_ = FontMap();
bool Font::isFreetypeInitialized_ = false;

ref_ptr<Font> Font::get(const std::string &f, uint32_t size, uint32_t dpi) {
	if (!isFreetypeInitialized_) {
		if (FT_Init_FreeType(&ftlib_)) { throw Font::Error("FT_Init_FreeType failed."); }
		isFreetypeInitialized_ = true;
	}
	// unique font identifier
	std::string fontKey = REGEN_STRING(f << size << "_" << dpi);
	// check for cached font
	auto result = fonts_.find(fontKey);
	if (result != fonts_.end()) { return result->second; }

	// create the font
	ref_ptr<Font> font = ref_ptr<Font>::alloc(f, size, dpi);
	fonts_[fontKey] = font;

	return font;
}

ref_ptr<Font> Font::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	if (!input.hasAttribute("file")) {
		REGEN_WARN("Ignoring Font '" << input.getDescription() << "' without file.");
		return {};
	}
	return regen::Font::get(
			resourcePath(input.getValue("file")),
			input.getValue<uint32_t>("size", 16u),
			input.getValue<uint32_t>("dpi", 96u));
}

void Font::closeLibrary() {
	FT_Done_FreeType(ftlib_);
	isFreetypeInitialized_ = false;
}

Font::Font(const std::string &fontPath, uint32_t size, uint32_t dpi)
		: fontPath_(fontPath),
		  size_(size),
		  dpi_(dpi),
		  lineHeight_(1.0f / .83f) {
	// The object in which Freetype holds information on a given
	// font is called a "face".
	FT_Face face;

	if (!boost::filesystem::exists(fontPath)) {
		throw Error(REGEN_STRING(
							"Unable to font file at '" << fontPath << "'."));
	}

	// This is where we load in the font information from the file.
	// Of all the places where the code might die, this is the most likely,
	// as FT_New_Face will die if the font file does not exist or is somehow broken.
	if (FT_New_Face(ftlib_, fontPath.c_str(), 0, &face)) {
		throw Error(REGEN_STRING(
							"FT_New_Face failed. " <<
												   "There is probably a problem with the font file at " << fontPath
												   << "."));
	}
	// Freetype measures font size in terms of 1/64ths of pixels.
	// Thus, to make a font h pixels high, we need to request a size of h*64.
	// (h << 6 is just a prettier way of writing h*64)
	FT_Set_Char_Size(face, size << 6, size << 6, dpi, dpi);

	// find the bounding box that can hold any glyph of this font,
	// we need to find this box because all glyph textures must have the same size
	// for Texture2DArray.
	int pixels_x = ::FT_MulFix((face->bbox.xMax - face->bbox.xMin), face->size->metrics.x_scale);
	int pixels_y = ::FT_MulFix((face->bbox.yMax - face->bbox.yMin), face->size->metrics.y_scale);
	auto textureWidth = (uint32_t) (pixels_x / 64);
	auto textureHeight = (uint32_t) (pixels_y / 64);
	// make sure texture dimensions are multiple of 2
	if (textureWidth % 2 != 0) textureWidth += 1;
	if (textureHeight % 2 != 0) textureHeight += 1;

	// remembers some geometry infos about glyphs
	faceData_ = new FaceData[NUMBER_OF_GLYPHS];

	// create a array texture for the glyphs
	arrayTexture_ = ref_ptr<Texture2DArray>::alloc(GL_TEXTURE_2D_ARRAY, 1);
	arrayTexture_->set_format(GL_RED);
	arrayTexture_->set_internalFormat(GL_R8);
	arrayTexture_->set_pixelType(GL_UNSIGNED_BYTE);
	arrayTexture_->set_rectangleSize(textureWidth, textureHeight);
	arrayTexture_->set_depth(NUMBER_OF_GLYPHS);
	arrayTexture_->allocTexture();
	// GL expects 4byte aligned rows
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	for (unsigned short i = 0; i < NUMBER_OF_GLYPHS; i++) {
		initGlyph(face, i, textureWidth, textureHeight);
	}
	// reset to default
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	arrayTexture_->set_wrapping(TextureWrapping::create(GL_CLAMP_TO_BORDER));
	arrayTexture_->set_filter(TextureFilter::create(GL_LINEAR));
	arrayTexture_->set_swizzle(TextureSwizzle::create(GL_RED));

	FT_Done_Face(face);
}

Font::~Font() {
	if (isFreetypeInitialized_) {
		auto needle = fonts_.find(
				REGEN_STRING(fontPath_ << size_ << "_" << dpi_));
		if (needle != fonts_.end()) { fonts_.erase(needle); }
	}
	delete[] faceData_;
}

const Font::FaceData &Font::faceData(GLushort ch) const { return faceData_[ch]; }

GLubyte *Font::invertPixmapWithAlpha(
		const FT_Bitmap &bitmap, uint32_t width, uint32_t height) {
	const unsigned int arraySize = width * height;
	auto *inverse = new GLubyte[arraySize];
	auto *inverse_ptr = inverse;

	memset(inverse, 0x0, sizeof(GLubyte) * arraySize);

	for (auto r = 0u; r < bitmap.rows; ++r) {
		GLubyte *bitmap_ptr = &bitmap.buffer[bitmap.pitch * r];
		for (auto p = 0u; p < bitmap.width; ++p) {
			*inverse_ptr++ = *bitmap_ptr++;
		}
		inverse_ptr += (width - bitmap.width);
	}

	return inverse;
}

void Font::initGlyph(FT_Face face, GLushort ch, uint32_t textureWidth, uint32_t textureHeight) {
	FaceData &glyphData = faceData_[ch];
	FT_Glyph glyph;
	GLubyte *inverted;

	// Load the Glyph for our character.
	if (FT_Load_Glyph(face, FT_Get_Char_Index(face, ch), FT_LOAD_DEFAULT)) {
		throw Error("FT_Load_Glyph failed");
	}
	// Move the face's glyph into a Glyph object.
	if (FT_Get_Glyph(face->glyph, &glyph)) {
		throw Error("FT_Get_Glyph failed");
	}
	// Convert the glyph to a bitmap.
	FT_Glyph_To_Bitmap(&glyph, ft_render_mode_normal, 0, 1);

	auto bitmap_glyph = (FT_BitmapGlyph) glyph;
	// This reference will make accessing the bitmap easier
	FT_Bitmap &bitmap = bitmap_glyph->bitmap;

	{
		auto bitmapWith = (float) bitmap.width;
		auto bitmapHeight = (float) bitmap.rows;
		float sizeFactor = 1.0f / (float) size();

		glyphData.uvX = bitmapWith / ((float) textureWidth);
		glyphData.uvY = bitmapHeight / ((float) textureHeight);
		glyphData.height = bitmapHeight * sizeFactor;
		glyphData.width = bitmapWith * sizeFactor;
		glyphData.advanceX = (face->glyph->advance.x >> 6) * sizeFactor;
		glyphData.left = bitmap_glyph->left * sizeFactor;
		glyphData.top = bitmap_glyph->top * sizeFactor;
	}

	{
		inverted = invertPixmapWithAlpha(bitmap, textureWidth, textureHeight);
		arrayTexture_->updateSubImage((int) ch, inverted);
		delete[]inverted;
	}

	FT_Done_Glyph(glyph);
}
