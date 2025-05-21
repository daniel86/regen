/*
 * texture-mapped-text.cpp
 *
 *  Created on: 23.03.2011
 *      Author: daniel
 */

#include <GL/glew.h>

#include <boost/algorithm/string.hpp>
#include <regen/textures/texture-state.h>

#include "texture-mapped-text.h"

using namespace regen;

TextureMappedText::TextureMappedText(const ref_ptr<Font> &font, const GLfloat &height)
		: Mesh(GL_TRIANGLES, BUFFER_USAGE_DYNAMIC_DRAW),
		  font_(font),
		  value_(),
		  height_(height),
		  centerAtOrigin_(true),
		  numCharacters_(0) {
	textColor_ = ref_ptr<ShaderInput4f>::alloc("textColor");
	textColor_->setUniformData(Vec4f(1.0));
	textColor_->setSchema(InputSchema::color());
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
	inputContainer_->set_numVertices(numCharacters_ * 6);
	// map client data for writing
	auto v_pos = posAttribute_->mapClientData<Vec3f>(ShaderData::WRITE);
	auto v_texco = texcoAttribute_->mapClientData<Vec3f>(ShaderData::WRITE);
	auto v_nor = norAttribute_->mapClientData<Vec3f>(ShaderData::WRITE);

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
							  v_pos,
							  v_nor,
							  v_texco,
							  &vertexCounter);

			// move cursor to next glyph
			translation.x += data.advanceX * height_;
		}

		translation.x = 0.0;
	}

	// apply offset to each vertex
	if (centerAtOrigin_) {
		GLfloat centerOffset = actualMaxLineWidth * 0.5f;
		for (GLuint i = 0; i < vertexCounter; ++i) {
			v_pos.w[i].x -= centerOffset;
		}
	}

	v_pos.unmap();
	v_nor.unmap();
	v_texco.unmap();

	begin(InputContainer::INTERLEAVED);
	setInput(posAttribute_);
	setInput(norAttribute_);
	setInput(texcoAttribute_);
	end();
	updateVAO();

	// set center and extends for bounding box
	minPosition_ = v_pos.w[0];
	maxPosition_ = v_pos.w[0];
	for (GLuint i = 1; i < vertexCounter; ++i) {
		minPosition_.setMin(v_pos.w[i]);
		maxPosition_.setMax(v_pos.w[i]);
	}
}

void TextureMappedText::makeGlyphGeometry(
		const Font::FaceData &data,
		const Vec3f &translation,
		GLfloat layer,
		ShaderData_rw<Vec3f> &posAttribute,
		ShaderData_rw<Vec3f> &norAttribute,
		ShaderData_rw<Vec3f> &texcoAttribute,
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

	posAttribute.w[i] = p0;
	posAttribute.w[i + 1] = p1;
	posAttribute.w[i + 2] = p2;
	posAttribute.w[i + 3] = p2;
	posAttribute.w[i + 4] = p3;
	posAttribute.w[i + 5] = p0;

	norAttribute.w[i] = n;
	norAttribute.w[i + 1] = n;
	norAttribute.w[i + 2] = n;
	norAttribute.w[i + 3] = n;
	norAttribute.w[i + 4] = n;
	norAttribute.w[i + 5] = n;

	texcoAttribute.w[i] = texco0;
	texcoAttribute.w[i + 1] = texco1;
	texcoAttribute.w[i + 2] = texco2;
	texcoAttribute.w[i + 3] = texco2;
	texcoAttribute.w[i + 4] = texco3;
	texcoAttribute.w[i + 5] = texco0;

	*vertexCounter += 6;
}
