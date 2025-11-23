#ifndef REGEN_TEXTURE_MAPPED_TEXT_H
#define REGEN_TEXTURE_MAPPED_TEXT_H

#include "regen/shader/shader-state.h"
#include <regen/objects/mesh.h>
#include <regen/objects/text/font.h>

namespace regen {
	/**
	 * \brief Implements texture mapped text rendering.
	 *
	 * This is done using texture mapped fonts.
	 * The Font is saved in a texture array, the glyphs are
	 * accessed by the w texture coordinate.
	 */
	class TextureMappedText : public Mesh {
	public:
		/**
		 * Defines how text is aligned.
		 */
		enum Alignment {
			ALIGNMENT_LEFT,
			ALIGNMENT_RIGHT,
			ALIGNMENT_CENTER
		};

		/**
		 * @param font font for the text.
		 * @param height text height.
		 */
		TextureMappedText(const ref_ptr<Font> &font, const float &height);

		/**
		 * @param color the text color.
		 */
		void set_color(const Vec4f &color);

		/**
		 * @return text as list of lines.
		 */
		const std::list<std::wstring> &value() const;

		/**
		 * Sets the text to be displayed.
		 */
		void set_value(
				const std::wstring &value,
				Alignment alignment = ALIGNMENT_LEFT,
				float maxLineWidth = 0.0f);

		/**
		 * Sets the text to be displayed.
		 */
		void set_value(
				const std::list<std::wstring> &lines,
				Alignment alignment = ALIGNMENT_LEFT,
				float maxLineWidth = 0.0f);

		/**
		 * Sets the y value of the primitive-set topmost point.
		 */
		void set_height(float height);

		/**
		 * Sets the centerAtOrigin flag.
		 */
		void set_centerAtOrigin(bool centerAtOrigin) { centerAtOrigin_ = centerAtOrigin; }

	protected:
		ref_ptr<Font> font_;
		std::list<std::wstring> value_;
		float height_;
		bool centerAtOrigin_;
		uint32_t numCharacters_;

		ref_ptr<ShaderInput4f> textColor_;

		ref_ptr<ShaderInput3f> posAttribute_;
		ref_ptr<ShaderInput3f> norAttribute_;
		ref_ptr<ShaderInput3f> texcoAttribute_;

		void updateAttributes(Alignment alignment, float maxLineWidth);

		void makeGlyphGeometry(
				const Font::FaceData &data,
				const Vec3f &translation,
				float layer,
				ClientData_rw<Vec3f> &posAttribute,
				ClientData_rw<Vec3f> &norAttribute,
				ClientData_rw<Vec3f> &texcoAttribute,
				uint32_t *vertexCounter);
	};
} // namespace

#endif /* REGEN_TEXTURE_MAPPED_TEXT_H */
