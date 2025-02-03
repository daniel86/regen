#include "ramp-texture.h"

using namespace regen;

RampTexture::RampTexture(
			GLenum format,
			GLenum internalFormat,
			GLuint width)
		: Texture1D() {
	begin(RenderState::get());
	set_rectangleSize(width, 1);
	set_pixelType(GL_UNSIGNED_BYTE);
	set_format(format);
	set_internalFormat(internalFormat);
	filter().push(GL_LINEAR);
	wrapping().push(GL_CLAMP_TO_EDGE);
	end(RenderState::get());
}

RampTexture::RampTexture(GLenum format, const std::vector<GLubyte> &data)
		: RampTexture(format, format, data) {}

RampTexture::RampTexture(
			GLenum format,
			GLenum internalFormat,
			const std::vector<GLubyte> &data)
		: Texture1D() {
	auto elementsPerTexel = glenum::pixelComponents(format);
	auto width = data.size() / elementsPerTexel;
	begin(RenderState::get());
	set_rectangleSize(width, 1);
	set_pixelType(GL_UNSIGNED_BYTE);
	set_format(format);
	set_internalFormat(internalFormat);
	filter().push(GL_LINEAR);
	wrapping().push(GL_CLAMP_TO_EDGE);
	textureData_ = data.data();
	Texture1D::texImage();
	textureData_ = nullptr;
	end(RenderState::get());
}

ref_ptr<RampTexture> RampTexture::darkWhite() {
	return ref_ptr<RampTexture>::alloc(GL_LUMINANCE, std::vector<GLubyte>{80, 255});
}

ref_ptr<RampTexture> RampTexture::darkWhiteSkewed() {
	return ref_ptr<RampTexture>::alloc(GL_LUMINANCE, std::vector<GLubyte>{80, 80, 80, 255, 255});
}

ref_ptr<RampTexture> RampTexture::normal() {
	return ref_ptr<RampTexture>::alloc(GL_LUMINANCE, std::vector<GLubyte>{0, 255});
}

ref_ptr<RampTexture> RampTexture::threeStep() {
	return ref_ptr<RampTexture>::alloc(GL_LUMINANCE, std::vector<GLubyte>{80, 160, 255});
}

ref_ptr<RampTexture> RampTexture::fourStep() {
	return ref_ptr<RampTexture>::alloc(GL_LUMINANCE, std::vector<GLubyte>{80, 140, 200, 255});
}

ref_ptr<RampTexture> RampTexture::fourStepSkewed() {
	return ref_ptr<RampTexture>::alloc(GL_LUMINANCE, std::vector<GLubyte>{80, 80, 80, 80, 140, 200, 255});
}

ref_ptr<RampTexture> RampTexture::blackWhiteBlack() {
	return ref_ptr<RampTexture>::alloc(GL_LUMINANCE, std::vector<GLubyte>{80, 255, 80});
}

ref_ptr<RampTexture> RampTexture::stripes() {
	return ref_ptr<RampTexture>::alloc(GL_LUMINANCE, std::vector<GLubyte>{
			80, 255, 80, 255, 80, 255, 80, 255,
			80, 255, 80, 255, 80, 255, 80, 255,
			80, 255, 80, 255, 80, 255, 80, 255,
			80, 255, 80, 255, 80, 255, 80, 255
	});
}

ref_ptr<RampTexture> RampTexture::stripe() {
	return ref_ptr<RampTexture>::alloc(GL_LUMINANCE, std::vector<GLubyte>{
			80, 80, 80, 80,
			80, 80, 80, 80,
			80, 80, 80, 80,
			0, 0, 255, 255,
			255, 255, 255, 255,
			255, 255, 255, 255,
			255, 255, 255, 255
	});
}

ref_ptr<RampTexture> RampTexture::rgb() {
	return ref_ptr<RampTexture>::alloc(GL_RGB, std::vector<GLubyte>{
			255,	0,		0,
			0,		255,	0,
			0,		0,		255
	});
}
