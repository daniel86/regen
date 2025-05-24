/*
 * fbo.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include <regen/gl-types/render-state.h>
#include <regen/gl-types/gl-util.h>
#include <regen/gl-types/shader-input.h>
#include <regen/textures/texture-cube.h>
#include <regen/textures/texture-3d.h>
#include <regen/textures/texture-array.h>
#include <regen/config.h>

#include "fbo.h"
#include "regen/scene/scene.h"

using namespace regen;
using namespace regen::scene;

static inline void REGEN_DrawBuffers(const DrawBuffers &v) { glDrawBuffers(v.buffers_.size(), &v.buffers_[0]); }

#ifdef WIN32
static inline void REGEN_DrawBuffer(GLenum v)
{ glDrawBuffer(v); }
static inline void REGEN_ReadBuffer(GLenum v)
{ glReadBuffer(v); }
#else
#define REGEN_DrawBuffer glDrawBuffer
#define REGEN_ReadBuffer glReadBuffer
#endif

static inline void attachTexture(
		const ref_ptr<Texture> &tex, GLenum target) {
	if (tex->numSamples() > 1) {
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
								target,
								tex->textureBind().target_,
								tex->id(), 0);
	} else {
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER,
							 target, tex->id(), 0);
	}
}

static inline void attachRenderBuffer(
		const ref_ptr<RenderBuffer> &rbo, GLenum target) {
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER,
							  target, GL_RENDERBUFFER, rbo->id());
}

FBO::Screen::Screen()
		: drawBuffer_(REGEN_DrawBuffer),
		  readBuffer_(REGEN_ReadBuffer) {
	RenderState *rs = RenderState::get();
	rs->drawFrameBuffer().push(0);
	drawBuffer_.push(GL_FRONT);
	rs->drawFrameBuffer().pop();
}

FBO::FBO(GLuint width, GLuint height, GLuint depth)
		: GLRectangle(glGenFramebuffers, glDeleteFramebuffers),
		  drawBuffers_(REGEN_DrawBuffers),
		  readBuffer_(REGEN_ReadBuffer),
		  depthAttachmentTarget_(GL_NONE),
		  depthAttachmentFormat_(GL_NONE),
		  depthAttachmentType_(GL_NONE) {
	RenderState *rs = RenderState::get();
	set_rectangleSize(width, height);
	depth_ = depth;

	viewport_ = ref_ptr<ShaderInput2f>::alloc("viewport");
	viewport_->setUniformData(
			Vec2f((GLfloat) width, (GLfloat) height));
	inverseViewport_ = ref_ptr<ShaderInput2f>::alloc("inverseViewport");
	inverseViewport_->setUniformData(
			Vec2f(1.0 / (GLfloat) width, 1.0 / (GLfloat) height));
	glViewport_ = Vec4ui(0, 0, width, height);

	rs->readFrameBuffer().push(id());
	readBuffer_.push(GL_COLOR_ATTACHMENT0);
	rs->readFrameBuffer().pop();

	uniforms_ = ref_ptr<UBO>::alloc("FBO");
	uniforms_->addBlockInput(viewport_);
	uniforms_->addBlockInput(inverseViewport_);
	GL_ERROR_LOG();
}

void FBO::set_depthAttachment(const ref_ptr<Texture> &tex) {
	attachTexture(tex, GL_DEPTH_ATTACHMENT);
	depthTexture_ = tex;
}

void FBO::set_depthAttachment(const ref_ptr<RenderBuffer> &rbo) {
	attachRenderBuffer(rbo, GL_DEPTH_ATTACHMENT);
	depthTexture_ = ref_ptr<Texture>();
}

void FBO::createDepthTexture(GLenum target, GLenum format, GLenum type, bool isStencil, uint32_t numSamples) {
	RenderState *rs = RenderState::get();
	depthAttachmentTarget_ = target;
	depthAttachmentFormat_ = format;
	depthAttachmentType_ = type;

	ref_ptr<Texture> depth;
	if (target == GL_TEXTURE_CUBE_MAP) {
		depth = ref_ptr<TextureCubeDepth>::alloc();
	} else if (target == GL_TEXTURE_2D_ARRAY) {
		ref_ptr<Texture3D> depth3D;
		if (numSamples > 1) {
			depth3D = ref_ptr<Texture2DArrayMultisampleDepth>::alloc(numSamples);
		} else {
			depth3D = ref_ptr<Texture2DArrayDepth>::alloc();
		}
		depth3D->set_depth(depth_);
		depth = depth3D;
	} else if (depth_ > 1) {
		ref_ptr<Texture3DDepth> depth3D = ref_ptr<Texture3DDepth>::alloc();
		depth3D->set_depth(depth_);
		depth = depth3D;
	} else if (numSamples > 1) {
		depth = ref_ptr<Texture2DMultisampleDepth>::alloc(numSamples);
	} else {
		depth = ref_ptr<Texture2DDepth>::alloc();
	}
	depth->set_rectangleSize(width(), height());
	depth->set_internalFormat(format);
	depth->set_pixelType(type);

	rs->drawFrameBuffer().push(id());
	{
		depth->begin(rs);
		if (numSamples == 1) {
			depth->wrapping().push(GL_REPEAT);
			depth->filter().push(GL_LINEAR);
			depth->compare().push(TextureCompare(GL_NONE, GL_EQUAL));
		}
		{
			depth->texImage();
		}
		depth->end(rs);
		if (isStencil) {
			set_depthStencilTexture(depth);
		} else {
			set_depthAttachment(depth);
		}
	}
	rs->drawFrameBuffer().pop();
}

void FBO::createDepthTexture(GLenum target, GLenum format, GLenum type, uint32_t numSamples) {
	createDepthTexture(target, format, type, false, numSamples);
}

void FBO::createDepthStencilTexture(GLenum target, GLenum format, GLenum type) {
	createDepthTexture(target, format, type, true);
}

void FBO::set_stencilTexture(const ref_ptr<Texture> &tex) {
	attachTexture(tex, GL_STENCIL_ATTACHMENT);
	stencilTexture_ = tex;
}

void FBO::set_stencilTexture(const ref_ptr<RenderBuffer> &rbo) {
	attachRenderBuffer(rbo, GL_STENCIL_ATTACHMENT);
	stencilTexture_ = ref_ptr<Texture>();
}

void FBO::set_depthStencilTexture(const ref_ptr<Texture> &tex) {
	attachTexture(tex, GL_DEPTH_STENCIL_ATTACHMENT);
	depthStencilTexture_ = tex;
}

void FBO::set_depthStencilTexture(const ref_ptr<RenderBuffer> &rbo) {
	attachRenderBuffer(rbo, GL_DEPTH_STENCIL_ATTACHMENT);
	depthStencilTexture_ = ref_ptr<Texture>();
}

GLenum FBO::addTexture(const ref_ptr<Texture> &tex) {
	RenderState *rs = RenderState::get();

	GLenum attachment = GL_COLOR_ATTACHMENT0 + colorBuffers_.buffers_.size();
	colorBuffers_.buffers_.push_back(attachment);
	colorTextures_.push_back(tex);

	rs->drawFrameBuffer().push(id());
	attachTexture(tex, attachment);
	rs->drawFrameBuffer().pop();

	return attachment;
}

ref_ptr<Texture> FBO::createTexture(
		GLuint width,
		GLuint height,
		GLuint depth,
		GLuint count,
		GLenum targetType,
		GLenum format,
		GLint internalFormat,
		GLenum pixelType,
		GLuint numSamples) {
	RenderState *rs = RenderState::get();
	ref_ptr<Texture> tex;
	ref_ptr<Texture3D> tex3d;

	switch (targetType) {
		case GL_TEXTURE_RECTANGLE:
			tex = ref_ptr<TextureRectangle>::alloc(count);
			break;

		case GL_TEXTURE_2D:
			if (numSamples > 1) {
				tex = ref_ptr<Texture2DMultisample>::alloc(numSamples, count);
			} else {
				tex = ref_ptr<Texture2D>::alloc(count);
			}
			break;

		case GL_TEXTURE_2D_ARRAY:
			if (numSamples > 1) {
				tex3d = ref_ptr<Texture2DArrayMultisample>::alloc(numSamples, count);
			} else {
				tex3d = ref_ptr<Texture2DArray>::alloc(count);
			}
			tex3d->set_depth(depth);
			tex = tex3d;
			break;

		case GL_TEXTURE_CUBE_MAP:
			tex = ref_ptr<TextureCube>::alloc(count);
			if (numSamples > 1) {
				REGEN_WARN("Multisample cube textures not supported. Using normal cube texture.");
			}
			break;

		case GL_TEXTURE_3D:
			tex3d = ref_ptr<Texture3D>::alloc(count);
			if (numSamples > 1) {
				REGEN_WARN("Multisample 3D textures not supported. Using normal 3D texture.");
			}
			tex3d->set_depth(depth);
			tex = tex3d;
			break;

		default: // GL_TEXTURE_2D:
			REGEN_WARN("Unknown texture type " << targetType << ". Using 2D texture.");
			tex = ref_ptr<Texture2D>::alloc(count);
			break;

	}

	tex->set_rectangleSize(width, height);
	tex->set_format(format);
	tex->set_internalFormat(internalFormat);
	tex->set_pixelType(pixelType);
	rs->activeTexture().push(GL_TEXTURE7);
	for (GLuint j = 0; j < count; ++j) {
		rs->textures().push(7, tex->textureBind());
		if (numSamples == 1) {
			tex->wrapping().push(GL_CLAMP_TO_EDGE);
			tex->filter().push(GL_LINEAR);
		}
		tex->texImage();

		rs->textures().pop(7);
		tex->nextObject();
	}
	rs->activeTexture().pop();

	return tex;
}

ref_ptr<Texture> FBO::addTexture(
		GLuint count,
		GLenum targetType,
		GLenum format,
		GLint internalFormat,
		GLenum pixelType,
		GLuint numSamples) {
	ref_ptr<Texture> tex = createTexture(width(), height(), depth_,
										 count, targetType, format, internalFormat, pixelType, numSamples);

	for (GLuint j = 0; j < tex->numObjects(); ++j) {
		addTexture(tex);
		tex->nextObject();
	}

	return tex;
}

GLenum FBO::addRenderBuffer(const ref_ptr<RenderBuffer> &rbo) {
	RenderState *rs = RenderState::get();

	GLenum attachment = GL_COLOR_ATTACHMENT0 + colorBuffers_.buffers_.size();
	colorBuffers_.buffers_.push_back(attachment);
	renderBuffers_.push_back(rbo);

	rs->drawFrameBuffer().push(id());
	attachRenderBuffer(rbo, attachment);
	rs->drawFrameBuffer().pop();

	return attachment;
}

ref_ptr<RenderBuffer> FBO::addRenderBuffer(GLuint count) {
	RenderState *rs = RenderState::get();
	ref_ptr<RenderBuffer> rbo = ref_ptr<RenderBuffer>::alloc(count);

	rbo->set_rectangleSize(width(), height());
	for (GLuint j = 0; j < count; ++j) {
		rbo->begin(rs);
		rbo->storage();
		rbo->end(rs);
		addRenderBuffer(rbo);
		rbo->nextObject();
	}

	return rbo;
}

void FBO::blitCopy(
		FBO &dst,
		GLenum readAttachment,
		GLenum writeAttachment,
		GLbitfield mask,
		GLenum filter,
		GLboolean keepRatio) {
	RenderState *rs = RenderState::get();
	// read from this
	rs->readFrameBuffer().push(id());
	if (readAttachment != GL_DEPTH_ATTACHMENT) {
		readBuffer_.push(readAttachment);
	}
	// write to dst
	rs->drawFrameBuffer().push(dst.id());
	if (writeAttachment != GL_DEPTH_ATTACHMENT) {
		dst.drawBuffers().push(writeAttachment);
	}
	if (keepRatio) {
		uint32_t dstWidth = dst.width();
		uint32_t dstHeight = dst.width() * ((GLfloat) width() / height());
		uint32_t offsetX, offsetY;
		if (dstHeight > dst.height()) {
			dstHeight = dst.height();
			dstWidth = dst.height() * ((GLfloat) height() / width());
			offsetX = (dst.width() - dstWidth) / 2;
			offsetY = 0;
		} else {
			offsetX = 0;
			offsetY = (dst.height() - dstHeight) / 2;
		}
		glBlitFramebuffer(
				0, 0,
				static_cast<int32_t>(width()),
				static_cast<int32_t>(height()),
				static_cast<int32_t>(offsetX),
				static_cast<int32_t>(offsetY),
				static_cast<int32_t>(offsetX + dstWidth),
				static_cast<int32_t>(offsetY + dstHeight),
				mask, filter);
	} else {
		glBlitFramebuffer(
				0, 0,
				static_cast<int32_t>(width()),
				static_cast<int32_t>(height()),
				0, 0,
				static_cast<int32_t>(dst.width()),
				static_cast<int32_t>(dst.height()),
				mask, filter);
	}

	if (writeAttachment != GL_DEPTH_ATTACHMENT) {
		dst.drawBuffers().pop();
	}
	rs->drawFrameBuffer().pop();
	if (readAttachment != GL_DEPTH_ATTACHMENT) {
		readBuffer_.pop();
	}
	rs->readFrameBuffer().pop();
}

void FBO::blitCopyToScreen(
		GLuint screenWidth, GLuint screenHeight,
		GLenum readAttachment,
		GLbitfield mask,
		GLenum filter,
		GLboolean keepRatio) {
	RenderState *rs = RenderState::get();
	// read from this
	rs->readFrameBuffer().push(id());
	readBuffer_.push(readAttachment);
	// write to screen front buffer
	rs->drawFrameBuffer().push(0);
	screen().drawBuffer_.push(GL_FRONT);

	if (keepRatio) {
		uint32_t dstWidth = screenWidth;
		uint32_t dstHeight = screenWidth * ((GLfloat) width() / height());
		uint32_t offsetX, offsetY;
		if (dstHeight > screenHeight) {
			dstHeight = screenHeight;
			dstWidth = screenHeight * ((GLfloat) height() / width());
			offsetX = (screenWidth - dstWidth) / 2;
			offsetY = 0;
		} else {
			offsetX = 0;
			offsetY = (screenHeight - dstHeight) / 2;
		}
		glBlitFramebuffer(
				0, 0,
				static_cast<int32_t>(width()),
				static_cast<int32_t>(height()),
				static_cast<int32_t>(offsetX),
				static_cast<int32_t>(offsetY),
				static_cast<int32_t>(offsetX + dstWidth),
				static_cast<int32_t>(offsetY + dstHeight),
				mask, filter);
	} else {
		glBlitFramebuffer(
				0, 0,
				static_cast<int32_t>(width()),
				static_cast<int32_t>(height()),
				0, 0,
				static_cast<int32_t>(screenWidth),
				static_cast<int32_t>(screenHeight),
				mask, filter);
	}

	rs->drawFrameBuffer().pop();
	readBuffer_.pop();
	rs->readFrameBuffer().pop();
	screen().drawBuffer_.pop();
}

void FBO::resize(GLuint w, GLuint h, GLuint depth) {
	RenderState *rs = RenderState::get();
	set_rectangleSize(w, h);
	depth_ = depth;

	viewport_->setUniformData(
			Vec2f((GLfloat) w, (GLfloat) h));
	inverseViewport_->setUniformData(
			Vec2f(1.0f / (GLfloat) w, 1.0f / (GLfloat) h));
	glViewport_ = Vec4ui(0, 0, w, h);
	rs->drawFrameBuffer().push(id());
	rs->activeTexture().push(GL_TEXTURE7);

	// resize depth attachment
	if (depthTexture_.get() != nullptr) {
		depthTexture_->set_rectangleSize(w, h);
		auto *tex3D = dynamic_cast<Texture3D *>(depthTexture_.get());
		if (tex3D != nullptr) { tex3D->set_depth(depth); }
		rs->textures().push(7, depthTexture_->textureBind());
		depthTexture_->texImage();
		rs->textures().pop(7);
	}

	// resize stencil attachment
	if (stencilTexture_.get() != nullptr) {
		stencilTexture_->set_rectangleSize(w, h);
		auto *tex3D = dynamic_cast<Texture3D *>(stencilTexture_.get());
		if (tex3D != nullptr) { tex3D->set_depth(depth); }
		rs->textures().push(7, stencilTexture_->textureBind());
		stencilTexture_->texImage();
		rs->textures().pop(7);
	}

	// resize depth stencil attachment
	if (depthStencilTexture_.get() != nullptr) {
		depthStencilTexture_->set_rectangleSize(w, h);
		auto *tex3D = dynamic_cast<Texture3D *>(depthStencilTexture_.get());
		if (tex3D != nullptr) { tex3D->set_depth(depth); }
		rs->textures().push(7, depthStencilTexture_->textureBind());
		depthStencilTexture_->texImage();
		rs->textures().pop(7);
	}

	// resize color attachments
	for (auto & tex : colorTextures_) {
		tex->set_rectangleSize(w, h);
		auto *tex3D = dynamic_cast<Texture3D *>(tex.get());
		if (tex3D != nullptr) { tex3D->set_depth(depth); }
		for (GLuint i = 0; i < tex->numObjects(); ++i) {
			rs->textures().push(7, tex->textureBind());
			tex->texImage();
			rs->textures().pop(7);
			tex->nextObject();
		}
	}

	// resize rbo attachments
	for (auto & rbo : renderBuffers_) {
		rbo->set_rectangleSize(w, h);
		for (GLuint i = 0; i < rbo->numObjects(); ++i) {
			rbo->begin(rs);
			rbo->storage();
			rbo->end(rs);
			rbo->nextObject();
		}
	}

	rs->activeTexture().pop();
	rs->drawFrameBuffer().pop();
}

void FBO::checkStatus() const {
	RenderState::get()->drawFrameBuffer().push(id());
	auto status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		REGEN_WARN("Framebuffer not complete: 0x" << std::hex << status << std::dec);
	}
	RenderState::get()->drawFrameBuffer().pop();
}

const ref_ptr<Texture> &FBO::firstColorTexture() const { return colorTextures_.front(); }

namespace regen {
	class FBOResizer : public EventHandler {
	public:
		FBOResizer(const ref_ptr<FBO> &fbo,
				   const ref_ptr<ShaderInput2i> &windowViewport,
				   GLfloat wScale, GLfloat hScale)
				: EventHandler(),
				  fbo_(fbo),
				  windowViewport_(windowViewport),
				  wScale_(wScale), hScale_(hScale) {}

		void call(EventObject *, EventData *) {
			auto winSize = windowViewport_->getVertex(0);
			Vec2i fboSize(winSize.r.x * wScale_, winSize.r.y * hScale_);
			if (fboSize.x % 2 != 0) fboSize.x += 1;
			if (fboSize.y % 2 != 0) fboSize.y += 1;
			winSize.unmap();
			fbo_->resize(fboSize.x, fboSize.y, 1);
		}

	protected:
		ref_ptr<FBO> fbo_;
		ref_ptr<ShaderInput2i> windowViewport_;
		GLfloat wScale_, hScale_;
	};
}

ref_ptr<FBO> FBO::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto sizeMode = input.getValue<std::string>("size-mode", "abs");
	auto relSize = input.getValue<Vec3f>("size", Vec3f(256.0, 256.0, 1.0));
	auto absSize = Texture::getSize(ctx.scene()->getViewport(), sizeMode, relSize);

	ref_ptr<FBO> fbo = ref_ptr<FBO>::alloc(absSize.x, absSize.y, absSize.z);
	if (sizeMode == "rel") {
		ref_ptr<FBOResizer> resizer = ref_ptr<FBOResizer>::alloc(
				fbo,
				ctx.scene()->getViewport(),
				relSize.x,
				relSize.y);
		ctx.scene()->addEventHandler(Scene::RESIZE_EVENT, resizer);
	}

	for (auto &n: input.getChildren()) {
		if (n->getCategory() == "texture") {
			LoadingContext texCfg(ctx.scene(), ctx.parent());
			ref_ptr<Texture> tex = Texture::load(texCfg, *n.get());
			for (GLuint j = 0; j < tex->numObjects(); ++j) {
				fbo->addTexture(tex);
				tex->nextObject();
			}
		} else if (n->getCategory() == "depth") {
			auto depthSize = n->getValue<GLint>("pixel-size", 16);
			GLenum depthType = glenum::pixelType(
					n->getValue<std::string>("pixel-type", "UNSIGNED_BYTE"));
			GLenum textureTarget = glenum::textureTarget(
					n->getValue<std::string>("target", "TEXTURE_2D"));
			auto numSamples = n->getValue<GLuint>("num-samples", 1);

			GLenum depthFormat;
			if (depthSize <= 16) depthFormat = GL_DEPTH_COMPONENT16;
			else if (depthSize <= 24) depthFormat = GL_DEPTH_COMPONENT24;
			else depthFormat = GL_DEPTH_COMPONENT32;

			fbo->createDepthTexture(textureTarget, depthFormat, depthType, numSamples);

			ref_ptr<Texture> tex = fbo->depthTexture();
			Texture::configure(tex, *n.get());
		} else if (n->getCategory() == "depth-stencil") {
			GLenum textureTarget = glenum::textureTarget(
					n->getValue<std::string>("target", "TEXTURE_2D"));
			GLenum depthFormat = GL_DEPTH24_STENCIL8;
			GLenum depthType = GL_UNSIGNED_INT_24_8;

   			auto tex = fbo->createTexture(
   				fbo->width(), fbo->height(), fbo->depth(),
   				1, textureTarget,
   				GL_DEPTH_STENCIL, depthFormat, depthType);
			fbo->set_depthStencilTexture(tex);
		} else if (n->getCategory() == "stencil") {
			GLenum pixelType = glenum::pixelType(
					n->getValue<std::string>("pixel-type", "UNSIGNED_BYTE"));
			GLenum textureTarget = glenum::textureTarget(
					n->getValue<std::string>("target", "TEXTURE_2D"));

			GLenum stencilFormat;
			auto stencilSize = n->getValue<GLint>("pixel-size", 8);
			if (stencilSize < 8) stencilFormat = GL_STENCIL_INDEX1;
			else if (stencilSize < 16) stencilFormat = GL_STENCIL_INDEX4;
			else stencilFormat = GL_STENCIL_INDEX8;

			auto stencilTexture = FBO::createTexture(
					fbo->width(), fbo->height(), fbo->depth(),
					1, textureTarget,
					GL_STENCIL_INDEX,
					stencilFormat,
					pixelType);
			fbo->set_stencilTexture(stencilTexture);
		} else {
			REGEN_WARN("No processor registered for '" << n->getDescription() << "'.");
		}
	}

	if (input.hasAttribute("clear-color")) {
		auto c = input.getValue<Vec4f>("clear-color", Vec4f(0.0f));

		fbo->drawBuffers().push(fbo->colorBuffers());
		RenderState::get()->clearColor().push(c);
		glClear(GL_COLOR_BUFFER_BIT);
		RenderState::get()->clearColor().pop();
		fbo->drawBuffers().pop();
	}
	GL_ERROR_LOG();
	fbo->checkStatus();

	return fbo;
}
