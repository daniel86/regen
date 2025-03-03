/*
 * texture.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "texture.h"
#include "regen/effects/bloom-texture.h"
#include "regen/textures/ramp-texture.h"
#include <regen/application.h>

using namespace regen::scene;
using namespace regen;

#include <regen/gl-types/gl-enum.h>
#include <regen/av/video-texture.h>
#include <regen/textures/texture-loader.h>
#include <regen/textures/noise-texture.h>

#define REGEN_TEXTURE_CATEGORY "texture"

class TextureResizer : public EventHandler {
public:
	TextureResizer(Application *app, const ref_ptr<Texture> &tex,
			const ref_ptr<ShaderInput2i> &windowViewport, GLfloat wScale, GLfloat hScale)
			: EventHandler(),
			  app_(app),
			  tex_(tex),
			  windowViewport_(windowViewport),
			  wScale_(wScale), hScale_(hScale) {}

	void call(EventObject *, EventData *) override {
		auto winSize = windowViewport_->getVertex(0).r;
		winSize.x = (winSize.x*wScale_);
		winSize.y = (winSize.y*hScale_);
		// FIXME: I think we should enforce GL thread here! But initially the resize needs to be done
		//        right away as withGLContext causes some fbo errors. possible fix: check if
		//        we have a GL context, and only use withGLContext if not. Could also do this in withGLContext.
		//app_->withGLContext([&]() {
			tex_->resize(winSize.x, winSize.y);
		//});
	}

protected:
	Application *app_;
	ref_ptr<Texture> tex_;
	ref_ptr<ShaderInput2i> windowViewport_;
	GLfloat wScale_, hScale_;
};


Vec3i TextureResource::getSize(
		const ref_ptr<ShaderInput2i> &viewport,
		const std::string &sizeMode,
		const Vec3f &size) {
	if (sizeMode == "abs") {
		return Vec3i(size.x, size.y, size.z);
	} else if (sizeMode == "rel") {
		auto v = viewport->getVertex(0);
		return Vec3i(
				(GLint) (size.x * v.r.x),
				(GLint) (size.y * v.r.y), 1);
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
					input.getValue<std::string>("wrapping", "CLAMP_TO_EDGE")));
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
			auto swizzleR = static_cast<int>(glenum::textureSwizzle(
					input.getValue<std::string>("swizzle-r", "RED")));
			auto swizzleG = static_cast<int>(glenum::textureSwizzle(
					input.getValue<std::string>("swizzle-g", "GREEN")));
			auto swizzleB = static_cast<int>(glenum::textureSwizzle(
					input.getValue<std::string>("swizzle-b", "BLUE")));
			auto swizzleA = static_cast<int>(glenum::textureSwizzle(
					input.getValue<std::string>("swizzle-a", "ALPHA")));
			tex->swizzle().push(Vec4i(swizzleR, swizzleG, swizzleB, swizzleA));
		}
		if (!input.getValue("compare-mode").empty()) {
			auto function = static_cast<int>(glenum::compareFunction(
					input.getValue<std::string>("compare-function", "LEQUAL")));
			auto mode = static_cast<int>(glenum::compareMode(
					input.getValue<std::string>("compare-mode", "NONE")));
			tex->compare().push(TextureCompare(mode, function));
		}
		if (!input.getValue("max-level").empty()) {
			tex->maxLevel().push(input.getValue<GLint>("max-level", 1000));
		}

		if (!input.getValue("min-filter").empty() &&
			!input.getValue("mag-filter").empty()) {
			auto min = static_cast<int>(glenum::filterMode(input.getValue("min-filter")));
			auto mag = static_cast<int>(glenum::filterMode(input.getValue("mag-filter")));
			tex->filter().push(TextureFilter(min, mag));
		} else if (!input.getValue("min-filter").empty() ||
				   !input.getValue("mag-filter").empty()) {
			REGEN_WARN("Minification and magnification filters must be specified both." <<
																						 " One missing for '"
																						 << input.getDescription()
																						 << "'.");
		}
	}
	tex->end(RenderState::get(), 0);
	GL_ERROR_LOG();
}

static std::vector<GLubyte> readTextureData(SceneInputNode &input, GLenum format) {
	std::vector<GLubyte> data;
	auto numPixelComponents = glenum::pixelComponents(format);
	// iterate over all "texel" children
	for (auto &child: input.getChildren("texel")) {
		if (!child->hasAttribute("v")) {
			REGEN_WARN("No 'v' attribute found for texel child '" << child->getName() << "'.");
			continue;
		}
		int texelWidth = child->getValue<int>("width", 1);
		for (int i = 0; i < texelWidth; ++i) {
			if (numPixelComponents == 1) {
				data.push_back(child->getValue<GLuint>("v", 0u));
			} else if (numPixelComponents == 2) {
				auto v = child->getValue<Vec2ui>("v", Vec2ui(0u));
				data.push_back(v.x);
				data.push_back(v.y);
			} else if (numPixelComponents == 3) {
				auto v = child->getValue<Vec3ui>("v", Vec3ui(0u));
				data.push_back(v.x);
				data.push_back(v.y);
				data.push_back(v.z);
			} else if (numPixelComponents == 4) {
				auto v = child->getValue<Vec4ui>("v", Vec4ui(0u));
				data.push_back(v.x);
				data.push_back(v.y);
				data.push_back(v.z);
				data.push_back(v.w);
			}
		}
	}
	return data;
}

ref_ptr<Texture> TextureResource::createResource(
		SceneParser *parser, SceneInputNode &input) {
	ref_ptr<Texture> tex;

	if (input.hasAttribute("file")) {
		auto mipmapFlag = GL_DONT_CARE;
		auto forcedInternalFormat = glenum::textureInternalFormat(
				input.getValue<std::string>("forced-internal-format", "NONE"));
		auto forcedFormat = glenum::textureFormat(
				input.getValue<std::string>("forced-format", "NONE"));
		auto forcedSize =
				input.getValue<Vec3ui>("forced-size", Vec3ui(0u));
		auto keepData = input.getValue<bool>("keep-data", false);
		const std::string filePath =
				resourcePath(input.getValue("file"));

		try {
			if (input.getValue<bool>("is-cube", false)) {
				tex = textures::loadCube(
						filePath,
						input.getValue<bool>("cube-flip-back", false),
						mipmapFlag,
						forcedInternalFormat,
						forcedFormat,
						forcedSize);
			} else if (input.getValue<bool>("is-array", false)) {
				tex = textures::loadArray(
						filePath,
						input.getValue<std::string>("name-pattern", ".*"),
						mipmapFlag,
						forcedInternalFormat,
						forcedFormat,
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
						forcedSize,
						keepData);
			}
		}
		catch (textures::Error &ie) {
			REGEN_ERROR("Failed to load Texture at " << filePath << ".");
		}
	} else if (input.hasAttribute("video")) {
		const std::string filePath = resourcePath(input.getValue("video"));
		ref_ptr<VideoTexture> video = ref_ptr<VideoTexture>::alloc();
		try {
			video->set_file(filePath);
			auto filename = filePath.substr(filePath.find_last_of('/') + 1);
			video->setAnimationName(REGEN_STRING("Video" << filename));
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
		const std::string noiseMode = input.getValue("noise");

		auto sizeMode = input.getValue<std::string>("size-mode", "abs");
		auto sizeRel = input.getValue<Vec3f>("size", Vec3f(256.0, 256.0, 1.0));
		auto sizeAbs = getSize(parser->getViewport(), sizeMode, sizeRel);

		auto randomSeed = input.getValue<GLint>("random-seed", rand());
		auto isSeamless = input.getValue<bool>("is-seamless", false);

		// TODO: allow configuration of noise parameters
		if (noiseMode == "cloud") {
			auto noise = ref_ptr<NoiseTexture2D>::alloc(sizeAbs.x, sizeAbs.y, isSeamless);
			noise->setNoiseGenerator(NoiseGenerator::preset_clouds(randomSeed));
			tex = noise;
		} else if (noiseMode == "wood") {
			auto noise = ref_ptr<NoiseTexture2D>::alloc(sizeAbs.x, sizeAbs.y, isSeamless);
			noise->setNoiseGenerator(NoiseGenerator::preset_wood(randomSeed));
			tex = noise;
		} else if (noiseMode == "granite") {
			auto noise = ref_ptr<NoiseTexture2D>::alloc(sizeAbs.x, sizeAbs.y, isSeamless);
			noise->setNoiseGenerator(NoiseGenerator::preset_granite(randomSeed));
			tex = noise;
		} else if (noiseMode == "perlin-2d" || noiseMode == "perlin") {
			auto noise = ref_ptr<NoiseTexture2D>::alloc(sizeAbs.x, sizeAbs.y, isSeamless);
			noise->setNoiseGenerator(NoiseGenerator::preset_perlin(randomSeed));
			tex = noise;
		} else if (noiseMode == "perlin-3d") {
			auto noise = ref_ptr<NoiseTexture3D>::alloc(sizeAbs.x, sizeAbs.y, sizeAbs.z, isSeamless);
			noise->setNoiseGenerator(NoiseGenerator::preset_perlin(randomSeed));
			tex = noise;
		}
	} else if (input.hasAttribute("ramp")) {
		auto ramp = input.getValue("ramp");
		if (ramp == "dark-white") {
			tex = RampTexture::darkWhite();
		} else if (ramp == "dark-white-skewed") {
			tex = RampTexture::darkWhiteSkewed();
		} else if (ramp == "normal") {
			tex = RampTexture::normal();
		} else if (ramp == "three-step") {
			tex = RampTexture::threeStep();
		} else if (ramp == "four-step") {
			tex = RampTexture::fourStep();
		} else if (ramp == "four-step-skewed") {
			tex = RampTexture::fourStepSkewed();
		} else if (ramp == "black-white-black") {
			tex = RampTexture::blackWhiteBlack();
		} else if (ramp == "stripes") {
			tex = RampTexture::stripes();
		} else if (ramp == "stripe") {
			tex = RampTexture::stripe();
		} else if (ramp == "rgb") {
			tex = RampTexture::rgb();
		} else if (ramp == "inline") {
			auto format = glenum::textureFormat(
					input.getValue<std::string>("format", "LUMINANCE"));
			auto internalFormat = format;
			if (input.hasAttribute("internal-format")) {
				internalFormat = glenum::textureInternalFormat(
						input.getValue<std::string>("internal-format", "LUMINANCE"));
			}
			auto data = readTextureData(input, format);
			tex = ref_ptr<RampTexture>::alloc(format, internalFormat, data);
		} else {
			REGEN_WARN("Unknown ramp type '" << ramp << "'.");
		}
	} else if (input.hasAttribute("spectrum")) {
		auto spectrum = input.getValue<Vec2d>("spectrum", Vec2d(0.0, 1.0));
		auto numTexels = input.getValue<GLint>("num-texels", 256u);
		tex = regen::textures::loadSpectrum(spectrum.x, spectrum.y, numTexels);
	} else if (input.hasAttribute("type")) {
		const std::string typeName = input.getValue("type");
		if (typeName == "bloom") {
			auto numMips = input.getValue<GLuint>("num-mips", 5u);
			auto bloomTexture = ref_ptr<BloomTexture>::alloc(numMips);

			auto inputFBO = parser->getResources()->getFBO(parser, input.getValue("input-fbo"));
			if (inputFBO.get() == nullptr) {
				REGEN_WARN("Unable to find FBO for '" << input.getDescription() << "'.");
			} else {
				auto resizer = ref_ptr<TextureResizer>::alloc(parser->application(),
						bloomTexture, parser->getViewport(), 1.0, 1.0);
				parser->addEventHandler(Application::RESIZE_EVENT, resizer);
				tex = bloomTexture;
				bloomTexture->resize(inputFBO->width(), inputFBO->height());
			}
		} else {
			REGEN_WARN("Unknown texture type '" << typeName << "'.");
		}
	} else {
		auto sizeMode = input.getValue<std::string>("size-mode", "abs");
		auto sizeRel = input.getValue<Vec3f>("size", Vec3f(256.0, 256.0, 1.0));
		Vec3i sizeAbs = getSize(parser->getViewport(), sizeMode, sizeRel);

		auto texCount = input.getValue<GLuint>("count", 1);
		auto pixelSize = input.getValue<GLuint>("pixel-size", 16);
		auto pixelComponents = input.getValue<GLuint>("pixel-components", 4);
		auto pixelType = glenum::pixelType(
				input.getValue<std::string>("pixel-type", "UNSIGNED_BYTE"));
		auto textureTarget = glenum::textureTarget(
				input.getValue<std::string>("target", sizeAbs.z > 1 ? "TEXTURE_3D" : "TEXTURE_2D"));


		tex = FBO::createTexture(
				sizeAbs.x, sizeAbs.y, sizeAbs.z,
				texCount,
				textureTarget,
				glenum::textureFormat(pixelComponents),
				glenum::textureInternalFormat(pixelType, pixelComponents, pixelSize),
				pixelType);

		if (input.hasAttribute("size-mode") && sizeMode == "rel") {
			auto resizer = ref_ptr<TextureResizer>::alloc(parser->application(),
					tex, parser->getViewport(), sizeRel.x, sizeRel.y);
			parser->addEventHandler(Application::RESIZE_EVENT, resizer);
		}
	}

	if (tex.get() == nullptr) {
		REGEN_WARN("Failed to create Texture for '" << input.getDescription() << ".");
		return tex;
	}
	tex->set_name(input.getName());
	configureTexture(tex, input);

	return tex;
}


