#include <GL/glew.h>

#include <boost/algorithm/string.hpp>
#include <regen/textures/texture-state.h>

#include "texture-mapped-text.h"

using namespace regen;

TextureMappedText::TextureMappedText(const ref_ptr<Font> &font, const float &height)
		: Mesh(GL_TRIANGLES, BufferUpdateFlags::FULL_RARELY),
		  font_(font),
		  value_(),
		  height_(height),
		  centerAtOrigin_(true),
		  numCharacters_(0) {
	textColor_ = ref_ptr<ShaderInput4f>::alloc("textColor");
	textColor_->setUniformData(Vec4f::one());
	textColor_->setSchema(InputSchema::color());
	setInput(textColor_);

	ref_ptr<Texture> tex = font_->texture();
	ref_ptr<TextureState> texState = ref_ptr<TextureState>::alloc(tex, "fontTexture");
	texState->set_mapTo(TextureState::MAP_TO_COLOR);
	texState->set_blendMode(BLEND_MODE_SRC_ALPHA);
	joinStates(texState);

	posAttribute_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	norAttribute_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	texcoAttribute_ = ref_ptr<ShaderInput3f>::alloc("texco0");
	posAttribute_->setVertexData(6, nullptr);
	norAttribute_->setVertexData(6, nullptr);
	texcoAttribute_->setVertexData(6, nullptr);
	setInput(posAttribute_);
	setInput(norAttribute_);
	setInput(texcoAttribute_);
}

void TextureMappedText::set_color(const Vec4f &color) {
	textColor_->setVertex(0, color);
}

void TextureMappedText::set_height(float height) { height_ = height; }

const std::list<std::wstring> &TextureMappedText::value() const { return value_; }

void TextureMappedText::set_value(
		const std::list<std::wstring> &value,
		Alignment alignment,
		float maxLineWidth) {
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
		float maxLineWidth) {
	std::list<std::wstring> v;
	boost::split(v, value, boost::is_any_of("\n"));
	set_value(v, alignment, maxLineWidth);
}

void TextureMappedText::updateAttributes(Alignment alignment, float maxLineWidth) {
	uint32_t vertexCounter = 0u;

	float actualMaxLineWidth = 0.0;
	const bool numCharactersChanged = (numCharacters_ != lastNumCharacters_);
	const uint32_t numRequiredVertices = numCharacters_ * 6;

	if (numRequiredVertices > posData_.size()) {
		posData_.resize(numRequiredVertices + 36);
		norData_.resize(numRequiredVertices + 36);
		texcoData_.resize(numRequiredVertices + 36);
	}

	// map client data for writing
	Vec3f* v_pos = posData_.data();
	Vec3f* v_texco = texcoData_.data();
	Vec3f* v_nor = norData_.data();

	auto translation = Vec3f::zero();
	auto glyphTranslation = Vec3f::zero();

	for (auto it = value_.begin(); it != value_.end(); ++it) {
		translation.y -= font_->lineHeight() * height_;

		// actual width for this line
		float lineWidth = 0.0;
		// remember space for splitting string at words
		int lastSpaceIndex = 0;
		float lastSpaceWidth = 0.0;

		// get line width and split the line
		// where it exceeds the width limit
		for (uint32_t i = 0; i < it->size(); ++i) {
			const wchar_t &ch = (*it)[i];
			const float buf = lineWidth + font_->faceData(ch).advanceX * height_;
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
		for (uint32_t i = 0; i < it->size(); ++i) {
			const wchar_t &ch = (*it)[i];
			const Font::FaceData &data = font_->faceData(ch);

			glyphTranslation = Vec3f(
					data.left * height_,
					(data.top - data.height) * height_,
					0.001 * (i + 1));
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
		float centerOffset = actualMaxLineWidth * 0.5f;
		for (uint32_t i = 0; i < vertexCounter; ++i) {
			v_pos[i].x -= centerOffset;
		}
	}

	// set center and extends for bounding box
	minPosition_ = v_pos[0];
	maxPosition_ = v_pos[0];
	for (uint32_t i = 1; i < vertexCounter; ++i) {
		minPosition_.setMin(v_pos[i]);
		maxPosition_.setMax(v_pos[i]);
	}

	if (numCharactersChanged) {
		if (bufferRef_.get()) {
			BufferObject::orphanBufferRange(bufferRef_.get());
		}
		posAttribute_->setVertexData(numRequiredVertices, (const byte*)v_pos);
		texcoAttribute_->setVertexData(numRequiredVertices, (const byte*)v_texco);
		norAttribute_->setVertexData(numRequiredVertices, (const byte*)v_nor);
		set_numVertices(numRequiredVertices);
		lastNumCharacters_ = numCharacters_;
		bufferRef_ = updateVertexData();
		updateVAO();
	}

	auto m_pos = posAttribute_->mapClientDataRaw(BUFFER_GPU_WRITE);
	auto m_nor = norAttribute_->mapClientDataRaw(BUFFER_GPU_WRITE);
	auto m_texco = texcoAttribute_->mapClientDataRaw(BUFFER_GPU_WRITE);
	std::memcpy(m_pos.w, v_pos, numRequiredVertices * sizeof(Vec3f));
	std::memcpy(m_nor.w, v_nor, numRequiredVertices * sizeof(Vec3f));
	std::memcpy(m_texco.w, v_texco, numRequiredVertices * sizeof(Vec3f));
	m_pos.unmap();
	m_nor.unmap();
	m_texco.unmap();
}

void TextureMappedText::makeGlyphGeometry(
		const Font::FaceData &data,
		const Vec3f &translation,
		float layer,
		Vec3f *posAttribute,
		Vec3f *norAttribute,
		Vec3f *texcoAttribute,
		uint32_t *vertexCounter) {
	uint32_t &i = *vertexCounter;
	Vec3f p0 = translation + Vec3f(0.0, data.height * height_, 0.0);
	Vec3f p1 = translation + Vec3f(0.0, 0.0, 0.0);
	Vec3f p2 = translation + Vec3f(data.width * height_, 0.0, 0.0);
	Vec3f p3 = translation + Vec3f(data.width * height_, data.height * height_, 0.0);
	Vec3f n = Vec3f(0.0, 0.0, 1.0);
	Vec3f texco0(0.0, 0.0, layer);
	Vec3f texco1(0.0, data.uvY, layer);
	Vec3f texco2(data.uvX, data.uvY, layer);
	Vec3f texco3(data.uvX, 0.0, layer);

	posAttribute[i] = p0;
	posAttribute[i + 1] = p1;
	posAttribute[i + 2] = p2;
	posAttribute[i + 3] = p2;
	posAttribute[i + 4] = p3;
	posAttribute[i + 5] = p0;

	norAttribute[i] = n;
	norAttribute[i + 1] = n;
	norAttribute[i + 2] = n;
	norAttribute[i + 3] = n;
	norAttribute[i + 4] = n;
	norAttribute[i + 5] = n;

	texcoAttribute[i] = texco0;
	texcoAttribute[i + 1] = texco1;
	texcoAttribute[i + 2] = texco2;
	texcoAttribute[i + 3] = texco2;
	texcoAttribute[i + 4] = texco3;
	texcoAttribute[i + 5] = texco0;

	*vertexCounter += 6;
}
