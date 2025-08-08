#include "bright-stars.h"

#include <regen/textures/texture-loader.h>
#include <regen/external/osghimmel/randommapgenerator.h>
#include <regen/external/osghimmel/coords.h>

using namespace regen;

BrightStars::BrightStars(const ref_ptr<Sky> &sky)
		: SkyLayer(sky) {
	state()->joinStates(ref_ptr<BlendFuncState>::alloc(
			GL_SRC_ALPHA, GL_ONE,
			GL_SRC_ALPHA, GL_ONE));

	color_ = ref_ptr<ShaderInput3f>::alloc("starColor");
	color_->setUniformData(defaultColor());
	color_->setSchema(InputSchema::color());
	state()->setInput(color_);

	apparentMagnitude_ = ref_ptr<ShaderInput1f>::alloc("apparentMagnitude");
	apparentMagnitude_->setUniformData(defaultApparentMagnitude());
	state()->setInput(apparentMagnitude_);

	colorRatio_ = ref_ptr<ShaderInput1f>::alloc("colorRatio");
	colorRatio_->setUniformData(defaultColorRatio());
	state()->setInput(colorRatio_);

	glareIntensity_ = ref_ptr<ShaderInput1f>::alloc("glareIntensity");
	glareIntensity_->setUniformData(0.1);
	state()->setInput(glareIntensity_);

	glareScale_ = ref_ptr<ShaderInput1f>::alloc("glareScale");
	glareScale_->setUniformData(defaultGlareScale());
	state()->setInput(glareScale_);

	scintillation_ = ref_ptr<ShaderInput1f>::alloc("scintillation");
	scintillation_->setUniformData(defaultScintillation());
	state()->setInput(scintillation_);

	scattering_ = ref_ptr<ShaderInput1f>::alloc("scattering");
	scattering_->setUniformData(defaultScattering());
	state()->setInput(scattering_);

	scale_ = ref_ptr<ShaderInput1f>::alloc("scale");
	scale_->setUniformData(2.0f);
	state()->setInput(scale_);

	noiseTexState_ = ref_ptr<TextureState>::alloc();
	updateNoiseTexture();
	state()->joinStates(noiseTexState_);

	shaderState_ = ref_ptr<HasShader>::alloc("regen.weather.bright-stars");
	meshState_ = ref_ptr<Mesh>::alloc(GL_POINTS, BufferUpdateFlags::NEVER);
	pos_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_POS);
	col_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_COL0);
}


#define _rightascd(deg, min, sec) \
    (_decimal(deg, min, sec) * 15.0L)

#define _rightasc(deg, min, sec) \
    (_rad(_rightascd(deg, min, sec)))

// Star model is based on the extended bright star catalogue.
struct s_BrightStar {
	float Vmag;   // visual magnitude (mag)
	float RA;     // right ascension (decimal hours)
	float DE;     // declination (decimal degrees)
	float pmRA;   // proper annual motion in right ascension (decimal hours)
	float pmDE;   // proper annual motion in declination (decimal degrees)
	float sRGB_R; // approximated color, red value   ]0;1[
	float sRGB_G; // approximated color, green value ]0;1[
	float sRGB_B; // approximated color, blue value  ]0;1[
};

static std::vector<s_BrightStar> loadStarsData(const char *fileName) {
	// Retrieve file size.
	FILE *f;
#ifdef __GNUC__
	f = std::fopen(fileName, "r");
#else // __GNUC__
	fopen_s(&f, fileName, "r");
#endif // __GNUC__
	if (!f) { return {}; }

	std::fseek(f, 0, SEEK_END);
	const auto fileSize = std::ftell(f);
	std::fclose(f);

	auto numStars = fileSize / sizeof(s_BrightStar);

	std::vector<s_BrightStar> stars(numStars);
	std::ifstream in_stream(fileName, std::ios::binary);
	in_stream.read(reinterpret_cast<char *>(stars.data()), fileSize);
	return stars;
}

void BrightStars::set_brightStarsFile(const std::string &brightStars) {
	auto starsData = loadStarsData(brightStars.c_str());
	numStars_ = static_cast<unsigned int>(starsData.size());
	if (numStars_ == 0) {
		REGEN_WARN("Unable to load bright stars catalog at " << brightStars << ".");
		return;
	}
	REGEN_INFO("Loaded " << numStars_ << " bright stars from " << brightStars << ".");

	pos_->setVertexData(numStars_);
	col_->setVertexData(numStars_);

	auto stars = starsData.data();
	for (unsigned int i = 0; i < numStars_; ++i) {
		osgHimmel::t_equf equ;
		equ.right_ascension = _rightascd(stars[i].RA, 0, 0);
		equ.declination = stars[i].DE;

		pos_->setVertex(i, Vec4f(equ.toEuclidean(), static_cast<float>(i)));
		col_->setVertex(i, Vec4f(
				stars[i].sRGB_R,
				stars[i].sRGB_G,
				stars[i].sRGB_B,
				stars[i].Vmag + 0.4f // the 0.4 accounts for magnitude decrease due to the earth's atmosphere
		));
	}

	meshState_->begin(Mesh::INTERLEAVED);
	meshState_->setInput(pos_);
	meshState_->setInput(col_);
	meshState_->end();
}

void BrightStars::updateNoiseTexture() {
	const int noiseN = 256;

	byte *noiseMap = new byte[noiseN];
	osgHimmel::RandomMapGenerator::generate1(noiseN, 1, noiseMap);

	noiseTex_ = ref_ptr<Texture1D>::alloc();
	noiseTex_->set_rectangleSize(noiseN, 1);
	noiseTex_->set_format(GL_RED);
	noiseTex_->set_internalFormat(GL_R8);
	noiseTex_->set_pixelType(GL_UNSIGNED_BYTE);
	noiseTex_->allocTexture();
	noiseTex_->set_filter(GL_LINEAR);
	noiseTex_->set_wrapping(GL_REPEAT);
	noiseTex_->updateImage((GLubyte *) noiseMap);
	delete[]noiseMap;

	noiseTexState_->set_texture(noiseTex_);
	noiseTexState_->set_name("noiseTexture");
}

float BrightStars::defaultApparentMagnitude() { return 7.0f; }

Vec3f BrightStars::defaultColor() { return {0.66, 0.78, 1.0}; }

float BrightStars::defaultColorRatio() { return 0.66f; }

float BrightStars::defaultGlareScale() { return 1.2f; }

float BrightStars::defaultScintillation() { return 0.2f; }

float BrightStars::defaultScattering() { return 2.0f; }
