#include <sstream>

#include <regen/utility/string-util.h>
#include <regen/utility/filesystem.h>
#include <regen/gl/gl-util.h>
#include <regen/gl/gl-enum.h>
#include <regen/gl/render-state.h>

#include "height-map.h"

using namespace regen;

#include "texture.h"
#include "texture-loader.h"
#include "regen/av/video-texture.h"
#include "noise-texture.h"
#include "ramp-texture.h"
#include "regen/passes/bloom-texture.h"
#include "regen/scene/scene.h"
#include "regen/scene/loading-context.h"
#include "fbo.h"
#include "texture-binder.h"

Texture::Texture(GLenum textureTarget, uint32_t numTextures)
		: GLRectangle(
				glCreateTextures,
				glDeleteTextures,
				textureTarget,
				numTextures),
		  ShaderInput1i(REGEN_STRING("textureChannel" << id())),
		  dim_(2),
		  format_(GL_RGBA),
		  internalFormat_(GL_RGBA8),
		  pixelType_(GL_BYTE),
		  texBind_(textureTarget, id()),
		  allocTexture_(&Texture::allocTexture_noop),
		  updateImage_(&Texture::updateImage_noop),
		  updateSubImage_(&Texture::updateSubImage_noop),
		  texCopyCounter_(ref_ptr<std::atomic<uint32_t>>::alloc(1)) {
	set_rectangleSize(2, 2);
	samplerType_ = "sampler2D";
	setUniformData(-1);
	set_active(false);
}

Texture::Texture(const Texture &other)
		: GLRectangle(other),
		  ShaderInput1i(other.name_),
		  dim_(other.dim_),
		  format_(other.format_),
		  internalFormat_(other.internalFormat_),
		  numComponents_(other.numComponents_),
		  imageDepth_(other.imageDepth_),
		  numMips_(other.numMips_),
		  pixelType_(other.pixelType_),
		  border_(other.border_),
		  texBind_(other.texBind_),
		  numSamples_(other.numSamples_),
		  fixedSampleLocations_(other.fixedSampleLocations_),
		  textureChannel_(other.textureChannel_),
		  samplerType_(other.samplerType_),
		  textureFile_(other.textureFile_),
		  texFilter_(other.texFilter_),
		  wrappingMode_(other.wrappingMode_),
		  texLoD_(other.texLoD_),
		  texSwizzle_(other.texSwizzle_),
		  texCompare_(other.texCompare_),
		  texMaxLevel_(other.texMaxLevel_),
		  texAniso_(other.texAniso_),
		  textureData_(other.textureData_),
		  numTexel_(other.numTexel_),
		  allocTexture_(other.allocTexture_),
		  updateImage_(other.updateImage_),
		  updateSubImage_(other.updateSubImage_),
		  allocatedSize_(other.allocatedSize_),
		  texCopyCounter_(other.texCopyCounter_) {
	auto &counter = *(other.texCopyCounter_.get());
	counter.fetch_add(1);
	setUniformData(-1);
	setTextureChannel(textureChannel_);
}

Texture::~Texture() {
	auto &counter = *(texCopyCounter_.get());
	auto oldValue = counter.fetch_sub(1);
	if (oldValue == 1) {
		// we are the last copy, make sure the texture is not bound anymore
		if (textureChannel_ != -1) {
			TextureBinder::release(this);
		}
	}
}

void Texture::set_filter(const TextureFilter &v) {
	glTextureParameteri(id(), GL_TEXTURE_MIN_FILTER, v.x);
	glTextureParameteri(id(), GL_TEXTURE_MAG_FILTER, v.y);
	texFilter_ = v;
}

void Texture::set_lod(const TextureLoD &v) {
	glTextureParameterf(id(), GL_TEXTURE_MIN_LOD, v.x);
	glTextureParameterf(id(), GL_TEXTURE_MAX_LOD, v.y);
	texLoD_ = v;
}

void Texture::set_swizzle(const TextureSwizzle &v) {
	glTextureParameteriv(id(), GL_TEXTURE_SWIZZLE_RGBA, &v.x);
	texSwizzle_ = v;
}

void Texture::set_wrapping(const TextureWrapping &v) {
	glTextureParameteri(id(), GL_TEXTURE_WRAP_S, v.x);
	glTextureParameteri(id(), GL_TEXTURE_WRAP_T, v.y);
	glTextureParameteri(id(), GL_TEXTURE_WRAP_R, v.z);
	wrappingMode_ = v;
}

void Texture::set_compare(const TextureCompare &v) {
	glTextureParameteri(id(), GL_TEXTURE_COMPARE_MODE, v.x);
	glTextureParameteri(id(), GL_TEXTURE_COMPARE_FUNC, v.y);
	texCompare_ = v;
}

void Texture::set_maxLevel(const TextureMaxLevel &v) {
	glTextureParameteri(id(), GL_TEXTURE_MAX_LEVEL, v);
	texMaxLevel_ = v;
}

void Texture::set_aniso(const TextureAniso &v) {
	glTextureParameterf(id(), GL_TEXTURE_MAX_ANISOTROPY_EXT, v);
	texAniso_ = v;
}

void Texture::allocTexture1D() {
	glTextureStorage1D(id(),
					   getNumMipmaps(),
					   internalFormat_,
					   static_cast<int32_t>(width()));
}

void Texture::allocTexture2D() {
	glTextureStorage2D(id(),
					   getNumMipmaps(),
					   internalFormat_,
					   static_cast<int32_t>(width()),
					   static_cast<int32_t>(height()));
}

void Texture::allocTexture2D_Multisample() {
	glTextureStorage2DMultisample(id(),
								  numSamples_,
								  internalFormat_,
								  static_cast<int32_t>(width()),
								  static_cast<int32_t>(height()),
								  fixedSampleLocations_);
}

void Texture::allocTexture3D() {
	glTextureStorage3D(id(),
					   getNumMipmaps(),
					   internalFormat_,
					   static_cast<int32_t>(width()),
					   static_cast<int32_t>(height()),
					   static_cast<int32_t>(depth()));
}

void Texture::updateImage1D(GLubyte *subData) {
	glTextureSubImage1D(id(),
						0, // mipmap level
						0, // x offset
						static_cast<int32_t>(width()),
						format_,
						pixelType_,
						subData);
}

void Texture::updateImage2D(GLubyte *subData) {
	glTextureSubImage2D(id(),
						0, // mipmap level
						0, // x offset
						0, // y offset
						static_cast<int32_t>(width()),
						static_cast<int32_t>(height()),
						format_,
						pixelType_,
						subData);
}

void Texture::updateImage3D(GLubyte *subData) {
	glTextureSubImage3D(id(),
						0, // mipmap level
						0, // x offset
						0, // y offset
						0, // z offset
						static_cast<int32_t>(width()),
						static_cast<int32_t>(height()),
						static_cast<int32_t>(depth()),
						format_,
						pixelType_,
						subData);
}

void Texture::updateSubImage1D(int layer, GLubyte *subData) {
	glTextureSubImage1D(id(),
						0, // mipmap level
						0, // x offset
						static_cast<int32_t>(width()),
						format_,
						pixelType_,
						subData);
}

void Texture::updateSubImage2D(int layer, GLubyte *subData) {
	glTextureSubImage2D(id(),
						0, // mipmap level
						0, // x offset
						0, // y offset
						static_cast<int32_t>(width()),
						static_cast<int32_t>(height()),
						format_,
						pixelType_,
						subData);
}

void Texture::updateSubImage3D(int layer, GLubyte *subData) {
	glTextureSubImage3D(id(),
						0, // mipmap level
						0, // x offset
						0, // y offset
						layer, // z offset
						static_cast<int32_t>(width()),
						static_cast<int32_t>(height()),
						1, // depth of the sub-image
						format_,
						pixelType_,
						subData);
}

GLenum Texture::targetType() const {
	return texBind_.target_;
}

void Texture::set_targetType(GLenum targetType) {
	texBind_.target_ = targetType;
}

const TextureBind &Texture::textureBind() {
	texBind_.id_ = id();
	return texBind_;
}

void Texture::setTextureData(const ref_ptr<ImageData> &textureData) {
	textureData_ = textureData;
	if (!textureData_->floatPixels) {
		if (textureData_->pixelType == GL_FLOAT) {
			// already float data
			textureData_->floatPixels = reinterpret_cast<float *>(textureData_->pixels);
		} else if (textureData_->pixelType == GL_UNSIGNED_BYTE) {
			// convert to float data
			const uint32_t numPixels = static_cast<uint32_t>(textureData_->width) *
									   static_cast<uint32_t>(textureData_->height) *
									   static_cast<uint32_t>(textureData_->depth);
			const uint32_t numComponents = static_cast<uint32_t>(textureData_->bpp);
			auto *floatPixels = new float[numPixels * numComponents];
			for (uint32_t i = 0; i < numPixels * numComponents; ++i) {
				floatPixels[i] = static_cast<float>(textureData_->pixels[i]) / 255.0f;
			}
			textureData_->floatPixels = floatPixels;
		}
	}
}

void Texture::unsetTextureData() {
	textureData_ = {};
}

void Texture::updateTextureData() {
	const uint32_t pixelTypeSize = (pixelType_ == GL_FLOAT) ? sizeof(float) : sizeof(byte);
	auto bufSize = static_cast<int32_t>(numTexel() * numComponents_ * pixelTypeSize);
	auto *pixels = new byte[bufSize];
	glGetTextureImage(id(), 0,
					  format(), pixelType_,
					  bufSize, pixels);
	auto imgData = ref_ptr<ImageData>::alloc();
	imgData->width = width();
	imgData->height = height();
	imgData->depth = depth();
	imgData->bpp = numComponents_;
	imgData->format = format();
	imgData->pixelType = pixelType_;
	imgData->pixels = pixels;
	setTextureData(imgData);
}

void Texture::ensureTextureData() {
	if (!textureData_) {
		updateTextureData();
	}
}

void Texture::allocTexture() {
	Vec3ui size(width(), height(), depth());
	if (size == allocatedSize_) {
		// already allocated
		return;
	}
	bool isReAlloc = (
		allocatedSize_.x > 0 &&
		allocatedSize_.y > 0 &&
		allocatedSize_.z > 0);
	allocatedSize_ = size;
	numTexel_ = size.x * size.y * size.z;
	if (isReAlloc) {
		// NOTE: texture objects must be destroyed and re-created
		glDeleteTextures(numObjects_, ids_);
		glCreateTextures(texBind_.target_, numObjects_, ids_);
	}
	texBind_.id_ = ids_[0];
	objectIndex_ = 0;
	for (uint32_t j = 0; j < numObjects_; ++j) {
		(this->*(this->allocTexture_))();
		if (isReAlloc) {
			set_wrapping(wrappingMode_);
			set_filter(texFilter_);
			if (texLoD_) { set_lod(*texLoD_); }
			if (texSwizzle_) { set_swizzle(*texSwizzle_); }
			if (texCompare_) { set_compare(*texCompare_); }
			if (texMaxLevel_) { set_maxLevel(*texMaxLevel_); }
			if (texAniso_) { set_aniso(*texAniso_); }
		}
		nextObject();
	}
	if (isReAlloc) {
		TextureBinder::rebind(this);
	}
}

void Texture::updateImage(GLubyte *data) {
	(this->*(this->updateImage_))(data);
}

void Texture::updateSubImage(int layer, GLubyte *subData) {
	(this->*(this->updateSubImage_))(layer, subData);
}

void Texture::setNumMipmaps(int32_t numMips) {
	numMips_ = numMips;
}

int32_t Texture::getNumMipmaps() {
	int32_t maxNumLevels = 1 + (int)floor(log2(std::max({width(), height()})));
	if (numMips_ >= 0 && numMips_ < maxNumLevels) {
		return numMips_;
	} else {
		return maxNumLevels;
	}
}

void Texture::updateMipmaps() {
	set_filter(TextureFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR));
	glGenerateTextureMipmap(id());
}

void Texture::setTextureChannel(int32_t channel) {
	textureChannel_ = channel;
	setVertex(0, channel);
	set_active(channel >= 0);
}

void Texture::bind() {
	TextureBinder::bind(this);
}

Bounds<Vec2ui> Texture::getRegion(const Vec2f &texco, const Vec2f &regionTS) const {
	auto w = static_cast<float>(width());
	auto h = static_cast<float>(height());
	auto startX = static_cast<unsigned int>(std::floor(texco.x * w));
	auto startY = static_cast<unsigned int>(std::floor(texco.y * h));
	auto endX = static_cast<unsigned int>(std::ceil((texco.x + regionTS.x) * w));
	auto endY = static_cast<unsigned int>(std::ceil((texco.y + regionTS.y) * h));
	if (endX >= width()) endX = width() - 1;
	if (endY >= height()) endY = height() - 1;
	return Bounds<Vec2ui>::create(
			Vec2ui(startX, startY),
			Vec2ui(endX, endY));
}

unsigned int Texture::texelIndex(const Vec2f &texco) const {
	auto w = width();
	auto h = height();
	//auto x = static_cast<unsigned int>(std::round(texco.x * static_cast<float>(w)));
	//auto y = static_cast<unsigned int>(std::round(texco.y * static_cast<float>(h)));
	auto x = static_cast<unsigned int>(texco.x * static_cast<float>(w));
	auto y = static_cast<unsigned int>(texco.y * static_cast<float>(h));
	// clamp to texture size
	switch (wrappingMode_.x) {
		case GL_REPEAT:
			x = x % w;
			y = y % h;
			break;
		case GL_MIRRORED_REPEAT:
			x = x % (2 * w);
			y = y % (2 * h);
			if (x >= w) x = 2 * w - x - 1;
			if (y >= h) y = 2 * h - y - 1;
			break;
		default: // GL_CLAMP_TO_EDGE:
			if (x >= w) x = w - 1;
			if (y >= h) y = h - 1;
			break;
	}
	return (y * w + x);
}

void Texture::set_textureFile(std::string_view fileName) {
	if (fileName.empty()) {
		textureFile_.reset();
	} else {
		textureFile_ = TextureFile(fileName);
	}
}

void Texture::set_textureFile(std::string_view directory, std::string_view namePattern) {
	if (directory.empty() || namePattern.empty()) {
		textureFile_.reset();
	} else {
		textureFile_ = TextureFile(directory, namePattern);
	}
}

static std::vector<GLubyte> readTextureData_cfg(LoadingContext&, scene::SceneInputNode &input, GLenum format) {
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
				data.push_back(child->getValue<uint32_t>("v", 0u));
			} else if (numPixelComponents == 2) {
				auto v = child->getValue<Vec2ui>("v", Vec2ui::zero());
				data.push_back(v.x);
				data.push_back(v.y);
			} else if (numPixelComponents == 3) {
				auto v = child->getValue<Vec3ui>("v", Vec3ui::zero());
				data.push_back(v.x);
				data.push_back(v.y);
				data.push_back(v.z);
			} else {
				auto v = child->getValue<Vec4ui>("v", Vec4ui::zero());
				data.push_back(v.x);
				data.push_back(v.y);
				data.push_back(v.z);
				data.push_back(v.w);
			}
		}
	}
	return data;
}

namespace regen {
	class TextureResizer : public EventHandler {
	public:
		TextureResizer(const ref_ptr<Texture> &tex,
					   const ref_ptr<Screen> &screen,
					   float wScale, float hScale)
				: EventHandler(),
				  tex_(tex),
				  screen_(screen),
				  wScale_(wScale), hScale_(hScale) {}

		~TextureResizer() override = default;

		void call(EventObject *, EventData *) override {
			Vec2i winSize = screen_->viewport().r;
			winSize.x = static_cast<int32_t>(static_cast<float>(winSize.x) * wScale_);
			winSize.y = static_cast<int32_t>(static_cast<float>(winSize.y) * hScale_);
			tex_->set_rectangleSize(winSize.x, winSize.y);
			tex_->allocTexture();
		}

	protected:
		ref_ptr<Texture> tex_;
		ref_ptr<Screen> screen_;
		float wScale_, hScale_;
	};
}

Vec3i Texture::getSize(
		const Vec2i &viewport,
		const std::string &sizeMode,
		const Vec3f &size) {
	if (sizeMode == "abs") {
		return size.asVec3i();
	} else if (sizeMode == "rel") {
		return {
			static_cast<int>(size.x * static_cast<float>(viewport.x)),
			static_cast<int>(size.y * static_cast<float>(viewport.y)),
			1 };
	} else {
		REGEN_WARN("Unknown size mode '" << sizeMode << "'.");
		return size.asVec3i();
	}
}

ref_ptr<Texture> Texture::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	ref_ptr<Texture> tex;
	auto &screen = ctx.scene()->screen();
	const std::string typeName = input.getValue("type");

	if (input.hasAttribute("file")) {
		TextureConfig texConfig;
		texConfig.forcedInternalFormat = glenum::textureInternalFormat(
				input.getValue<std::string>("internal-format", "NONE"));
		texConfig.forcedFormat = glenum::textureFormat(
				input.getValue<std::string>("forced-format", "NONE"));
		texConfig.forcedSize =
				input.getValue<Vec3ui>("forced-size", Vec3ui::zero());
		texConfig.keepData = input.getValue<bool>("keep-data", false);
		texConfig.useMipmaps = input.getValue<bool>("mipmap", false);
		const std::string filePath =
				resourcePath(input.getValue("file"));

		REGEN_DEBUG("Loading texture from file " << filePath << " with config: "
					<< " internal-format=0x" << std::hex << texConfig.forcedInternalFormat << std::dec
					<< " format=0x" << std::hex << texConfig.forcedFormat << std::dec
					<< " size=" << texConfig.forcedSize
					<< " keep-data=" << texConfig.keepData
					<< " use-mipmaps=" << texConfig.useMipmaps);

		try {
			if (input.getValue<bool>("is-cube", false)) {
				tex = textures::loadCube(
						filePath,
						input.getValue<bool>("cube-flip-back", false),
						texConfig);
			} else if (input.getValue<bool>("is-array", false)) {
				tex = textures::loadArray(
						filePath,
						input.getValue<std::string>("name-pattern", ".*"),
						texConfig);
			} else if (input.getValue<bool>("is-raw", false)) {
				tex = textures::loadRAW(
						filePath,
						input.getValue<Vec3ui>("raw-size", Vec3ui::create(256u)),
						input.getValue<uint32_t>("raw-components", 3u),
						input.getValue<uint32_t>("raw-bytes", 4u));
			} else {
				tex = textures::load(filePath, texConfig);
			}
		}
		catch (textures::Error &ie) {
			REGEN_ERROR("Failed to load Texture at " << filePath << ". " << ie.what());
		}
	} else if (input.hasAttribute("video")) {
		const std::string filePath = resourcePath(input.getValue("video"));
		ref_ptr<VideoTexture> video = ref_ptr<VideoTexture>::alloc();
		try {
			video->set_file(filePath);
			auto filename = filePath.substr(filePath.find_last_of('/') + 1);
			video->setAnimationName(REGEN_STRING("Video" << filename));
			video->demuxer()->set_repeat(
					input.getValue<bool>("repeat", true));
			tex = video;
			video->play();
			video->startAnimation();
		}
		catch (VideoTexture::Error &ve) {
			REGEN_ERROR("Failed to load Video at " << filePath << ". " << ve.what());
		}
	} else if (typeName == "noise") {
		auto sizeMode = input.getValue<std::string>("size-mode", "abs");
		auto sizeRel = input.getValue<Vec3f>("size", Vec3f(256.0, 256.0, 1.0));
		auto sizeAbs = getSize(screen->viewport().r, sizeMode, sizeRel);
		auto isSeamless = input.getValue<bool>("is-seamless", false);
		auto generator = NoiseGenerator::load(ctx, input);
		if (generator.get()) {
			auto noise = ref_ptr<NoiseTexture2D>::alloc(sizeAbs.x, sizeAbs.y, isSeamless);
			noise->setNoiseScale(input.getValue<float>("noise-scale", 1.0f));
			noise->setNoiseGenerator(generator);
			tex = noise;
		} else {
			REGEN_WARN("No noise generator found for " << input.getDescription() << ".");
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
			auto internalFormat = glenum::textureInternalFormat(format);
			if (input.hasAttribute("internal-format")) {
				internalFormat = glenum::textureInternalFormat(
						input.getValue<std::string>("internal-format", "LUMINANCE"));
			}
			auto data = readTextureData_cfg(ctx, input, format);
			tex = ref_ptr<RampTexture>::alloc(format, internalFormat, data);
		} else {
			REGEN_WARN("Unknown ramp type '" << ramp << "'.");
		}
	} else if (input.hasAttribute("spectrum")) {
		auto spectrum = input.getValue<Vec2d>("spectrum", Vec2d(0.0, 1.0));
		auto numTexels = input.getValue<int>("num-texels", 256u);
		tex = regen::textures::loadSpectrum(spectrum.x, spectrum.y, numTexels);
	} else if (typeName == "bloom") {
		auto numMips = input.getValue<uint32_t>("num-mips", 5u);
		auto bloomTexture = ref_ptr<BloomTexture>::alloc(numMips);
		auto inputFBO = ctx.scene()->getResource<FBO>(input.getValue("input-fbo"));
		if (inputFBO.get() == nullptr) {
			REGEN_WARN("Unable to find FBO for '" << input.getDescription() << "'.");
		} else {
			auto resizer = ref_ptr<TextureResizer>::alloc(
					bloomTexture, screen, 1.0, 1.0);
			ctx.scene()->addEventHandler(Scene::RESIZE_EVENT, resizer);
			tex = bloomTexture;
			bloomTexture->resize(inputFBO->width(), inputFBO->height());
		}
	} else {
		auto sizeMode = input.getValue<std::string>("size-mode", "abs");
		auto sizeRel = input.getValue<Vec3f>("size", Vec3f(256.0, 256.0, 1.0));
		Vec3i sizeAbs = getSize(screen->viewport().r, sizeMode, sizeRel);

		auto texCount = input.getValue<uint32_t>("count", 1);
		auto pixelComponents = input.getValue<uint32_t>("pixel-components", 4);
		auto pixelType = glenum::pixelType(
				input.getValue<std::string>("pixel-type", "UNSIGNED_BYTE"));
		auto textureTarget = glenum::textureTarget(
				input.getValue<std::string>("target", sizeAbs.z > 1 ? "TEXTURE_3D" : "TEXTURE_2D"));
		auto numSamples = input.getValue<uint32_t>("num-samples", 1);

		GLenum internalFormat;
		if (input.hasAttribute("internal-format")) {
			internalFormat = glenum::textureInternalFormat(
					input.getValue<std::string>("internal-format", "RGBA8"));
		} else {
			auto pixelSize = input.getValue<uint32_t>("pixel-size", 16);
			internalFormat = glenum::textureInternalFormat(pixelType,
														   pixelComponents, pixelSize);
		}

		auto textureFormat = glenum::textureFormat(pixelComponents);
		REGEN_DEBUG("Creating texture with "
					<< sizeAbs.x << "x" << sizeAbs.y << "x" << sizeAbs.z
					<< ", " << texCount << " textures, "
					<< "target: 0x" << std::hex << textureTarget << ", "
					<< "format: 0x" << std::hex << textureFormat << ", "
					<< "internal-format: 0x" << std::hex << internalFormat << ", "
					<< "pixel-type: 0x" << std::hex << pixelType << std::dec << ", "
					<< numSamples << " samples.");
		tex = FBO::createTexture(
				sizeAbs.x, sizeAbs.y, sizeAbs.z,
				texCount,
				textureTarget,
				textureFormat,
				internalFormat,
				pixelType,
				numSamples);

		if (input.hasAttribute("size-mode") && sizeMode == "rel") {
			auto resizer = ref_ptr<TextureResizer>::alloc(
					tex, screen,
					sizeRel.x, sizeRel.y);
			ctx.scene()->addEventHandler(Scene::RESIZE_EVENT, resizer);
		}
	}

	if (tex.get() == nullptr) {
		REGEN_WARN("Failed to create Texture for '" << input.getDescription() << ".");
		return tex;
	}
	tex->set_name(input.getName());
	configure(tex, input);

	if (typeName == "height") {
		auto tex2d = dynamic_cast<Texture2D *>(tex.get());
		if (tex2d) {
			auto heightMap = ref_ptr<HeightMap>::alloc(*tex2d);
			heightMap->setMapCenter(input.getValue<Vec2f>(
				"map-center", Vec2f::zero()));
			heightMap->setMapFactor(input.getValue<float>(
				"map-factor", 8.0f));
			heightMap->setMapSize(input.getValue<Vec2f>(
				"map-size", Vec2f::create(10.0)));
			tex = heightMap;
		}
	}

	return tex;
}

void Texture::configure(ref_ptr<Texture> &tex, scene::SceneInputNode &input) {
	if (!input.getValue("sampler-type").empty()) {
		tex->set_samplerType(input.getValue("sampler-type"));
	}
	if (tex->numSamples() > 1) {
		// NOTE: glTexParameter* not allowed for multi-sampled textures
		return;
	}
	if (!input.getValue("wrapping").empty()) {
		tex->set_wrapping(TextureWrapping::create(glenum::wrappingMode(
				input.getValue<std::string>("wrapping", "CLAMP_TO_EDGE"))));
	}
	if (!input.getValue("aniso").empty()) {
		tex->set_aniso(input.getValue<float>("aniso", 2.0f));
	}
	if (!input.getValue("lod").empty()) {
		tex->set_lod(input.getValue<Vec2f>("lod", Vec2f::create(1.0f)));
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
		tex->set_swizzle(Vec4i(swizzleR, swizzleG, swizzleB, swizzleA));
	}
	if (!input.getValue("compare-mode").empty()) {
		auto function = static_cast<int>(glenum::compareFunction(
				input.getValue<std::string>("compare-function", "LEQUAL")));
		auto mode = static_cast<int>(glenum::compareMode(
				input.getValue<std::string>("compare-mode", "NONE")));
		tex->set_compare(TextureCompare(mode, function));
	}
	if (!input.getValue("max-level").empty()) {
		tex->set_maxLevel(input.getValue<int>("max-level", 1000));
	}

	if (!input.getValue("min-filter").empty() &&
		!input.getValue("mag-filter").empty()) {
		auto min = static_cast<int>(glenum::filterMode(input.getValue("min-filter")));
		auto mag = static_cast<int>(glenum::filterMode(input.getValue("mag-filter")));
		tex->set_filter(TextureFilter(min, mag));
	} else if (!input.getValue("min-filter").empty() ||
			   !input.getValue("mag-filter").empty()) {
		REGEN_WARN("Minification and magnification filters must be specified both." <<
																					" One missing for '"
																					<< input.getDescription()
																					<< "'.");
	}
	GL_ERROR_LOG();
}

Texture1D::Texture1D(uint32_t numTextures)
		: Texture(GL_TEXTURE_1D, numTextures) {
	dim_ = 1;
	samplerType_ = "sampler1D";
	allocTexture_ = &Texture1D::allocTexture1D;
	updateImage_ = &Texture1D::updateImage1D;
	updateSubImage_ = &Texture1D::updateSubImage1D;
}

Texture2D::Texture2D(GLenum textureTarget, uint32_t numTextures)
		: Texture(textureTarget, numTextures) {
	dim_ = 2;
	samplerType_ = "sampler2D";
	allocTexture_ = &Texture2D::allocTexture2D;
	updateImage_ = &Texture2D::updateImage2D;
	updateSubImage_ = &Texture2D::updateSubImage2D;
}

TextureMips2D::TextureMips2D(uint32_t numMips)
		: Texture2D(GL_TEXTURE_2D, 1) {
	numMips_ = 1; // we do not use the built-in mipmap generation
	mipTextures_.resize(numMips);
	mipRefs_.resize(numMips-1);

	mipTextures_[0] = this;
	for (uint32_t i = 1u; i < numMips; ++i) {
		mipRefs_[i-1] = ref_ptr<Texture2D>::alloc();
		mipTextures_[i] = mipRefs_[i-1].get();
	}
}

TextureRectangle::TextureRectangle(uint32_t numTextures)
		: Texture2D(GL_TEXTURE_RECTANGLE, numTextures) {
	samplerType_ = "sampler2DRect";
}

Texture2DDepth::Texture2DDepth(GLenum textureTarget, uint32_t numTextures)
		: Texture2D(textureTarget, numTextures) {
	format_ = GL_DEPTH_COMPONENT;
	internalFormat_ = GL_DEPTH_COMPONENT24;
	pixelType_ = GL_UNSIGNED_BYTE;
}

Texture2DMultisample::Texture2DMultisample(
		GLsizei numSamples,
		uint32_t numTextures,
		bool fixedSampleLocations)
		: Texture2D(GL_TEXTURE_2D_MULTISAMPLE, numTextures) {
	fixedSampleLocations_ = fixedSampleLocations;
	samplerType_ = "sampler2DMS";
	set_numSamples(numSamples);
	allocTexture_ = &Texture2DMultisample::allocTexture2D_Multisample;
	// NOTE: no data can be uploaded from CPU to a multisample texture
	updateImage_ = &Texture2DMultisample::updateImage_noop;
	updateSubImage_ = &Texture2DMultisample::updateSubImage_noop;
}

Texture2DMultisampleDepth::Texture2DMultisampleDepth(
		GLsizei numSamples,
		bool fixedSampleLocations)
		: Texture2DDepth(GL_TEXTURE_2D_MULTISAMPLE, 1) {
	internalFormat_ = GL_DEPTH_COMPONENT24;
	fixedSampleLocations_ = fixedSampleLocations;
	set_numSamples(numSamples);
	allocTexture_ = &Texture2DMultisampleDepth::allocTexture2D_Multisample;
	// NOTE: no data can be uploaded from CPU to a multisample texture
	updateImage_ = &Texture2DMultisampleDepth::updateImage_noop;
	updateSubImage_ = &Texture2DMultisampleDepth::updateSubImage_noop;
}

Texture3D::Texture3D(GLenum textureTarget, uint32_t numTextures)
		: Texture(textureTarget, numTextures) {
	dim_ = 3;
	samplerType_ = "sampler3D";
	allocTexture_ = &Texture3D::allocTexture3D;
	updateImage_ = &Texture3D::updateImage3D;
	updateSubImage_ = &Texture3D::updateSubImage3D;
}

void Texture3D::set_depth(uint32_t numTextures) {
	imageDepth_ = numTextures;
}

Texture3DDepth::Texture3DDepth(uint32_t numTextures)
		: Texture3D(GL_TEXTURE_3D, numTextures) {
	format_ = GL_DEPTH_COMPONENT;
	internalFormat_ = GL_DEPTH_COMPONENT24;
}

Texture2DArray::Texture2DArray(GLenum textureTarget, uint32_t numTextures)
		: Texture3D(textureTarget, numTextures) {
	samplerType_ = "sampler2DArray";
}

Texture2DArrayDepth::Texture2DArrayDepth(uint32_t numTextures)
		: Texture2DArray(GL_TEXTURE_2D_ARRAY, numTextures) {
	format_ = GL_DEPTH_COMPONENT;
	internalFormat_ = GL_DEPTH_COMPONENT24;
	pixelType_ = GL_UNSIGNED_BYTE;
}

Texture2DArrayMultisample::Texture2DArrayMultisample(
		GLsizei numSamples,
		uint32_t numTextures,
		bool fixedSampleLocations)
		: Texture2DArray(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, numTextures) {
	samplerType_ = "sampler2DMSArray";
	set_numSamples(numSamples);
	fixedSampleLocations_ = fixedSampleLocations;
}

Texture2DArrayMultisampleDepth::Texture2DArrayMultisampleDepth(
		GLsizei numSamples,
		uint32_t numTextures,
		bool fixedSampleLocations)
		: Texture2DArray(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, numTextures) {
	samplerType_ = "sampler2DMSArray";
	set_numSamples(numSamples);
	fixedSampleLocations_ = fixedSampleLocations;
}

TextureCube::TextureCube(uint32_t numTextures)
		: Texture2D(GL_TEXTURE_CUBE_MAP, numTextures) {
	samplerType_ = "samplerCube";
	dim_ = 3;
	imageDepth_ = 6; // 6 faces for cube map
	updateImage_ = &TextureCube::updateImage3D;
	updateSubImage_ = &TextureCube::updateSubImage3D;
}

TextureCubeDepth::TextureCubeDepth(uint32_t numTextures)
		: TextureCube(numTextures) {
	format_ = GL_DEPTH_COMPONENT;
	internalFormat_ = GL_DEPTH_COMPONENT24;
	pixelType_ = GL_UNSIGNED_BYTE;
}
