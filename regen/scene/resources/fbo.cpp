/*
 * fbo.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "fbo.h"
#include "texture.h"
#include <regen/application.h>

using namespace regen::scene;
using namespace regen;
using namespace std;

#define REGEN_FBO_CATEGORY "fbo"

/**
 * Resizes Framebuffer texture when the window size changed.
 */
class FBOResizer : public EventHandler {
public:
	/**
	 * Default constructor.
	 * @param fbo FBO reference.
	 * @param windowViewport The window dimensions.
	 * @param wScale The width scale.
	 * @param hScale The height scale.
	 */
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

FBOResource::FBOResource()
		: ResourceProvider(REGEN_FBO_CATEGORY) {}

ref_ptr<FBO> FBOResource::createResource(
		SceneParser *parser, SceneInputNode &input) {
	auto sizeMode = input.getValue<string>("size-mode", "abs");
	auto relSize = input.getValue<Vec3f>("size", Vec3f(256.0, 256.0, 1.0));
	auto absSize = TextureResource::getSize(
			parser->getViewport(), sizeMode, relSize);

	ref_ptr<FBO> fbo = ref_ptr<FBO>::alloc(absSize.x, absSize.y, absSize.z);
	if (sizeMode == "rel") {
		ref_ptr<FBOResizer> resizer = ref_ptr<FBOResizer>::alloc(
				fbo,
				parser->getViewport(),
				relSize.x,
				relSize.y);
		parser->addEventHandler(Application::RESIZE_EVENT, resizer);
	}

	const list<ref_ptr<SceneInputNode> > &childs = input.getChildren();
	for (auto it = childs.begin(); it != childs.end(); ++it) {
		const ref_ptr<SceneInputNode> &n = *it;

		if (n->getCategory() == "texture") {
			ref_ptr<Texture> tex = TextureResource().createResource(parser, *n.get());
			for (GLuint j = 0; j < tex->numObjects(); ++j) {
				fbo->addTexture(tex);
				tex->nextObject();
			}
		} else if (n->getCategory() == "depth") {
			auto depthSize = n->getValue<GLint>("pixel-size", 16);
			GLenum depthType = glenum::pixelType(
					n->getValue<string>("pixel-type", "UNSIGNED_BYTE"));
			GLenum textureTarget = glenum::textureTarget(
					n->getValue<string>("target", "TEXTURE_2D"));

			GLenum depthFormat;
			if (depthSize <= 16) depthFormat = GL_DEPTH_COMPONENT16;
			else if (depthSize <= 24) depthFormat = GL_DEPTH_COMPONENT24;
			else depthFormat = GL_DEPTH_COMPONENT32;

			fbo->createDepthTexture(textureTarget, depthFormat, depthType);

			ref_ptr<Texture> tex = fbo->depthTexture();
			TextureResource::configureTexture(tex, *n.get());
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

	return fbo;
}
