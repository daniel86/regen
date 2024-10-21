/*
 * texture.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "texture.h"

using namespace regen::scene;
using namespace regen;
using namespace std;

#include <regen/gl-types/gl-enum.h>
#include <regen/av/video-texture.h>
#include <regen/textures/texture-loader.h>
#include <regen/textures/noise-texture.h>

#define REGEN_TEXTURE_CATEGORY "texture"

/**
 * Resizes Texture texture when the window size changed
 */
class TextureResizer : public EventHandler {
public:
	/**
	 * Default constructor.
	 * @param tex Texture reference.
	 * @param windowViewport The window dimensions.
	 * @param wScale The width scale.
	 * @param hScale The height scale.
	 */
	TextureResizer(const ref_ptr<Texture> &tex,
				   const ref_ptr<ShaderInput2i> &windowViewport,
				   GLfloat wScale, GLfloat hScale)
			: EventHandler(),
			  tex_(tex),
			  windowViewport_(windowViewport),
			  wScale_(wScale), hScale_(hScale) {}

	void call(EventObject *, EventData *) {
		const Vec2i &winSize = windowViewport_->getVertex(0);

		tex_->set_rectangleSize(winSize.x * wScale_, winSize.y * hScale_);
		RenderState::get()->textures().push(7, tex_->textureBind());
		tex_->texImage();
		RenderState::get()->textures().pop(7);
	}

protected:
	ref_ptr<Texture> tex_;
	ref_ptr<ShaderInput2i> windowViewport_;
	GLfloat wScale_, hScale_;
};

Vec3i TextureResource::getSize(
		const ref_ptr<ShaderInput2i> &viewport,
		const string &sizeMode,
		const Vec3f &size) {
	if (sizeMode == "abs") {
		return Vec3i(size.x, size.y, size.z);
	} else if (sizeMode == "rel") {
		Vec2i v = viewport->getVertex(0);
		return Vec3i(
				(GLint) (size.x * v.x),
				(GLint) (size.y * v.y), 1);
	} else {
		REGEN_WARN("Unknown size mode '" << sizeMode << "'.");
		return Vec3i(size.x, size.y, size.z);
	}
}

TextureResource::TextureResource()
		: ResourceProvider(REGEN_TEXTURE_CATEGORY) {}

void TextureResource::configureTexture(
		ref_ptr<Texture> &tex, SceneInputNode &input) {
	if (!input.getValue("sampler-type").empty()) {
		tex->set_samplerType(input.getValue("sampler-type"));
	}
	tex->begin(RenderState::get(), 0);
	{
		if (!input.getValue("wrapping").empty()) {
			tex->wrapping().push(glenum::wrappingMode(
					input.getValue<string>("wrapping", "CLAMP_TO_EDGE")));
		}
		if (!input.getValue("aniso").empty()) {
			tex->aniso().push(input.getValue<GLfloat>("aniso", 2.0f));
		}
		if (!input.getValue("lod").empty()) {
			tex->lod().push(input.getValue<Vec2f>("lod", Vec2f(1.0f)));
		}
		if (!input.getValue("swizzle-r").empty() ||
			!input.getValue("swizzle-g").empty() ||
			!input.getValue("swizzle-b").empty() ||
			!input.getValue("swizzle-a").empty()) {
			GLenum swizzleR = glenum::textureSwizzle(
					input.getValue<string>("swizzle-r", "RED"));
			GLenum swizzleG = glenum::textureSwizzle(
					input.getValue<string>("swizzle-g", "GREEN"));
			GLenum swizzleB = glenum::textureSwizzle(
					input.getValue<string>("swizzle-b", "BLUE"));
			GLenum swizzleA = glenum::textureSwizzle(
					input.getValue<string>("swizzle-a", "ALPHA"));
			tex->swizzle().push(Vec4i(swizzleR, swizzleG, swizzleB, swizzleA));
		}
		if (!input.getValue("compare-mode").empty()) {
			GLenum function = glenum::compareFunction(
					input.getValue<string>("compare-function", "LEQUAL"));
			GLenum mode = glenum::compareMode(
					input.getValue<string>("compare-mode", "NONE"));
			tex->compare().push(TextureCompare(mode, function));
		}
		if (!input.getValue("max-level").empty()) {
			tex->maxLevel().push(input.getValue<GLint>("max-level", 1000));
		}

		if (!input.getValue("min-filter").empty() &&
			!input.getValue("mag-filter").empty()) {
			GLenum min = glenum::filterMode(input.getValue("min-filter"));
			GLenum mag = glenum::filterMode(input.getValue("mag-filter"));
			tex->filter().push(TextureFilter(min, mag));
		} else if (!input.getValue("min-filter").empty() ||
				   !input.getValue("mag-filter").empty()) {
			REGEN_WARN("Minifiacation and magnification filters must be specified both." <<
																						 " One missing for '"
																						 << input.getDescription()
																						 << "'.");
		}
	}
	tex->end(RenderState::get(), 0);
	GL_ERROR_LOG();
}

ref_ptr<Texture> TextureResource::createResource(
		SceneParser *parser, SceneInputNode &input) {
	ref_ptr<Texture> tex;

	if (input.hasAttribute("file")) {
		auto mipmapFlag = GL_DONT_CARE;
		auto forcedType = glenum::pixelType(
				input.getValue<string>("forced-type", "NONE"));
		auto forcedInternalFormat = glenum::textureInternalFormat(
				input.getValue<string>("forced-internal-format", "NONE"));
		auto forcedFormat = glenum::textureFormat(
				input.getValue<string>("forced-format", "NONE"));
		auto forcedSize =
				input.getValue<Vec3ui>("forced-size", Vec3ui(0u));
		const string filePath =
				getResourcePath(input.getValue("file"));

		try {
			if (input.getValue<bool>("is-cube", false)) {
				tex = textures::loadCube(
						filePath,
						input.getValue<bool>("cube-flip-back", false),
						mipmapFlag,
						forcedInternalFormat,
						forcedFormat,
						forcedType,
						forcedSize);
			} else if (input.getValue<bool>("is-array", false)) {
				tex = textures::loadArray(
						filePath,
						input.getValue<string>("name-pattern", ".*"),
						mipmapFlag,
						forcedInternalFormat,
						forcedFormat,
						forcedType,
						forcedSize);
			} else if (input.getValue<bool>("is-raw", false)) {
				tex = textures::loadRAW(
						filePath,
						input.getValue<Vec3ui>("raw-size", Vec3ui(256u)),
						input.getValue<GLuint>("raw-components", 3u),
						input.getValue<GLuint>("raw-bytes", 4u));
			} else {
				tex = textures::load(
						filePath,
						mipmapFlag,
						forcedInternalFormat,
						forcedFormat,
						forcedType,
						forcedSize);
			}
		}
		catch (textures::Error &ie) {
			REGEN_ERROR("Failed to load Texture at " << filePath << ".");
		}
	} else if (input.hasAttribute("video")) {
		const string filePath = getResourcePath(input.getValue("video"));
		ref_ptr<VideoTexture> video = ref_ptr<VideoTexture>::alloc();
		video->stopAnimation();
		try {
			video->set_file(filePath);
			video->demuxer()->set_repeat(
					input.getValue<bool>("repeat", 1));
			tex = video;
			video->play();
			video->startAnimation();
		}
		catch (VideoTexture::Error &ve) {
			REGEN_ERROR("Failed to load Video at " << filePath << ".");
		}
	} else if (input.hasAttribute("noise")) {
		const string noiseMode = input.getValue("noise");

		auto sizeMode = input.getValue<string>("size-mode", "abs");
		auto sizeRel = input.getValue<Vec3f>("size", Vec3f(256.0, 256.0, 1.0));
		auto sizeAbs = getSize(parser->getViewport(), sizeMode, sizeRel);

		auto randomSeed = input.getValue<GLint>("random-seed", rand());
		auto isSeamless = input.getValue<bool>("is-seamless", false);

		textures::PerlinNoiseConfig perlinCfg;
		perlinCfg.baseFrequency = input.getValue<GLfloat>("base-frequency", 4.0);
		perlinCfg.persistence = input.getValue<GLfloat>("persistence", 0.5);
		perlinCfg.lacunarity = input.getValue<GLfloat>("lacunarity", 2.5);
		perlinCfg.octaveCount = input.getValue<GLuint>("octave-count", 4);

		if (noiseMode == "cloud") {
			tex = regen::textures::clouds2D(
					sizeAbs.x, sizeAbs.y, randomSeed, isSeamless);
		} else if (noiseMode == "wood") {
			tex = regen::textures::wood(
					sizeAbs.x, sizeAbs.y, randomSeed, isSeamless);
		} else if (noiseMode == "granite") {
			tex = regen::textures::granite(
					sizeAbs.x, sizeAbs.y, randomSeed, isSeamless);
		} else if (noiseMode == "perlin-2d" || noiseMode == "perlin") {
			tex = regen::textures::perlin2D(
					sizeAbs.x, sizeAbs.y,
					perlinCfg,
					randomSeed, isSeamless);
		} else if (noiseMode == "perlin-3d") {
			tex = regen::textures::perlin3D(
					sizeAbs.x, sizeAbs.y, sizeAbs.z,
					perlinCfg,
					randomSeed, isSeamless);
		}
	} else if (input.hasAttribute("spectrum")) {
		auto spectrum = input.getValue<Vec2d>("spectrum", Vec2d(0.0, 1.0));
		auto numTexels = input.getValue<GLuint>("num-texels", 256u);
		tex = regen::textures::loadSpectrum(spectrum.x, spectrum.y, numTexels);
	} else {
		auto sizeMode = input.getValue<string>("size-mode", "abs");
		auto sizeRel = input.getValue<Vec3f>("size", Vec3f(256.0, 256.0, 1.0));
		Vec3i sizeAbs = getSize(parser->getViewport(), sizeMode, sizeRel);

		auto texCount = input.getValue<GLuint>("count", 1);
		auto pixelSize = input.getValue<GLuint>("pixel-size", 16);
		auto pixelComponents = input.getValue<GLuint>("pixel-components", 4);
		auto pixelType = glenum::pixelType(
				input.getValue<string>("pixel-type", "UNSIGNED_BYTE"));
		auto textureTarget = glenum::textureTarget(
				input.getValue<string>("target", sizeAbs.z > 1 ? "TEXTURE_3D" : "TEXTURE_2D"));


		tex = FBO::createTexture(
				sizeAbs.x, sizeAbs.y, sizeAbs.z,
				texCount,
				textureTarget,
				glenum::textureFormat(pixelComponents),
				glenum::textureInternalFormat(pixelType, pixelComponents, pixelSize),
				pixelType);

		if (input.hasAttribute("size-mode") && sizeMode == "rel") {
			ref_ptr<TextureResizer> resizer = ref_ptr<TextureResizer>::alloc(
					tex, parser->getViewport(), sizeRel.x, sizeRel.y);
			parser->addEventHandler(Application::RESIZE_EVENT, resizer);
		}
	}

	if (tex.get() == nullptr) {
		REGEN_WARN("Failed to create Texture for '" << input.getDescription() << ".");
		return tex;
	}
	configureTexture(tex, input);

	return tex;
}


