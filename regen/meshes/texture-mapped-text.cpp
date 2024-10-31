/*
 * texture-mapped-text.cpp
 *
 *  Created on: 23.03.2011
 *      Author: daniel
 */

#include <GL/glew.h>

#include <boost/algorithm/string.hpp>
#include <regen/states/texture-state.h>

#include "texture-mapped-text.h"

using namespace regen;

TextureMappedText::TextureMappedText(const ref_ptr<Font> &font, const GLfloat &height)
		: Mesh(GL_TRIANGLES, VBO::USAGE_DYNAMIC),
		  font_(font),
		  value_(),
		  height_(height),
		  centerAtOrigin_(true),
		  numCharacters_(0) {
	textColor_ = ref_ptr<ShaderInput4f>::alloc("textColor");
	textColor_->setUniformData(Vec4f(1.0));
	joinShaderInput(textColor_);

	ref_ptr<Texture> tex = font_->texture();
	ref_ptr<TextureState> texState = ref_ptr<TextureState>::alloc(tex, "fontTexture");
	texState->set_mapTo(TextureState::MAP_TO_COLOR);
	texState->set_blendMode(BLEND_MODE_SRC_ALPHA);
	joinStates(texState);

	posAttribute_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	norAttribute_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	texcoAttribute_ = ref_ptr<ShaderInput3f>::alloc("texco0");
}

void TextureMappedText::set_color(const Vec4f &color) {
	textColor_->setVertex(0, color);
}

void TextureMappedText::set_height(GLfloat height) { height_ = height; }

const std::list<std::wstring> &TextureMappedText::value() const { return value_; }

void TextureMappedText::set_value(
		const std::list<std::wstring> &value,
		Alignment alignment,
		GLfloat maxLineWidth) {
	value_ = value;
	numCharacters_ = 0;
	for (auto it = value.begin(); it != value.end(); ++it) {
		numCharacters_ += it->size();
	}
	updateAttributes(alignment, maxLineWidth);
}

void TextureMappedText::set_value(
		const std::wstring &value,
		Alignment alignment,
		GLfloat maxLineWidth) {
	std::list<std::wstring> v;
	boost::split(v, value, boost::is_any_of("\n"));
	set_value(v, alignment, maxLineWidth);
}

void TextureMappedText::updateAttributes(Alignment alignment, GLfloat maxLineWidth) {
	Vec3f translation, glyphTranslation;
	GLuint vertexCounter = 0u;

	GLfloat actualMaxLineWidth = 0.0;

	posAttribute_->setVertexData(numCharacters_ * 6);
	texcoAttribute_->setVertexData(numCharacters_ * 6);
	norAttribute_->setVertexData(numCharacters_ * 6);

	translation = Vec3f(0.0, 0.0, 0.0);
	glyphTranslation = Vec3f(0.0, 0.0, 0.0);

	for (auto it = value_.begin(); it != value_.end(); ++it) {
		translation.y -= font_->lineHeight() * height_;

		GLfloat buf;
		// actual width for this line
		GLfloat lineWidth = 0.0;
		// remember space for splitting string at words
		GLint lastSpaceIndex = 0;
		GLfloat lastSpaceWidth = 0.0;

		// get line width and split the line
		// where it exceeds the width limit
		for (GLuint i = 0; i < it->size(); ++i) {
			const wchar_t &ch = (*it)[i];
			buf = lineWidth + font_->faceData(ch).advanceX * height_;
			if (maxLineWidth > 0.0 && buf > maxLineWidth && lastSpaceIndex != 0) {
				// maximal line length reached
				// split string at remembered space

				// insert the rest after current iterator
				// it cannot be placed in one line with
				// the words before lastSpaceIndex
				auto nextIt = it;
				nextIt++;
				value_.insert(nextIt, it->substr(lastSpaceIndex + 1,
												 it->size() - lastSpaceIndex - 1));
				// set the string width validated width
				*it = it->substr(0, lastSpaceIndex);

				lineWidth = lastSpaceWidth;
				break;
			} else if (ch == ' ' || ch == '\t') {
				// remember spaces
				lastSpaceIndex = i;
				lastSpaceWidth = lineWidth;
			}
			lineWidth = buf;
		}
		if (lineWidth > actualMaxLineWidth) {
			actualMaxLineWidth = lineWidth;
		}

		// calculate line advance!
		switch (alignment) {
			case ALIGNMENT_CENTER:
				translation.x = 0.5f * (maxLineWidth - lineWidth);
				break;
			case ALIGNMENT_RIGHT:
				translation.x = maxLineWidth - lineWidth;
				break;
			case ALIGNMENT_LEFT:
				translation.x = 0.0f;
				break;
		}

		// create the geometry with appropriate
		// translation and size for each glyph
		for (GLuint i = 0; i < it->size(); ++i) {
			const wchar_t &ch = (*it)[i];
			const Font::FaceData &data = font_->faceData(ch);

			glyphTranslation = Vec3f(
					data.left * height_, (data.top - data.height) * height_, 0.001 * (i + 1)
			);
			makeGlyphGeometry(data, translation + glyphTranslation, (float) ch,
							  posAttribute_.get(), norAttribute_.get(),
							  texcoAttribute_.get(), &vertexCounter);

			// move cursor to next glyph
			translation.x += data.advanceX * height_;
		}

		translation.x = 0.0;
	}

	// apply offset to each vertex
	if(centerAtOrigin_) {
		GLfloat centerOffset = actualMaxLineWidth*0.5f;
		for (GLuint i = 0; i < vertexCounter; ++i) {
			auto pos = posAttribute_->getVertex(i);
			pos.x -= centerOffset;
			posAttribute_->setVertex(i, pos);
		}
	}

	begin(ShaderInputContainer::INTERLEAVED);
	setInput(posAttribute_);
	setInput(norAttribute_);
	setInput(texcoAttribute_);
	end();
	updateVAO(RenderState::get());
}

void TextureMappedText::makeGlyphGeometry(
		const Font::FaceData &data,
		const Vec3f &translation,
		GLfloat layer,
		ShaderInput3f *posAttribute,
		ShaderInput3f *norAttribute,
		ShaderInput3f *texcoAttribute,
		GLuint *vertexCounter) {
	GLuint &i = *vertexCounter;
	Vec3f p0 = translation + Vec3f(0.0, data.height * height_, 0.0);
	Vec3f p1 = translation + Vec3f(0.0, 0.0, 0.0);
	Vec3f p2 = translation + Vec3f(data.width * height_, 0.0, 0.0);
	Vec3f p3 = translation + Vec3f(data.width * height_, data.height * height_, 0.0);
	Vec3f n = Vec3f(0.0, 0.0, 1.0);
	Vec3f texco0(0.0, 0.0, layer);
	Vec3f texco1(0.0, data.uvY, layer);
	Vec3f texco2(data.uvX, data.uvY, layer);
	Vec3f texco3(data.uvX, 0.0, layer);

	posAttribute->setVertex(i, p0);
	posAttribute->setVertex(i + 1, p1);
	posAttribute->setVertex(i + 2, p2);
	posAttribute->setVertex(i + 3, p2);
	posAttribute->setVertex(i + 4, p3);
	posAttribute->setVertex(i + 5, p0);

	norAttribute->setVertex(i, n);
	norAttribute->setVertex(i + 1, n);
	norAttribute->setVertex(i + 2, n);
	norAttribute->setVertex(i + 3, n);
	norAttribute->setVertex(i + 4, n);
	norAttribute->setVertex(i + 5, n);

	texcoAttribute->setVertex(i, texco0);
	texcoAttribute->setVertex(i + 1, texco1);
	texcoAttribute->setVertex(i + 2, texco2);
	texcoAttribute->setVertex(i + 3, texco2);
	texcoAttribute->setVertex(i + 4, texco3);
	texcoAttribute->setVertex(i + 5, texco0);

	*vertexCounter += 6;
}
