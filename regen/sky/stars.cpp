/*
 * stars.cpp
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#include "stars.h"

#include <regen/textures/texture-loader.h>
#include <regen/external/osghimmel/brightstars.h>
#include <regen/external/osghimmel/randommapgenerator.h>
#include <regen/external/osghimmel/coords.h>

using namespace regen;

Stars::Stars(const ref_ptr<Sky> &sky)
		: SkyLayer(sky) {
	state()->joinStates(ref_ptr<BlendState>::alloc(GL_SRC_ALPHA, GL_ONE));

	auto starsUniforms = ref_ptr<UniformBlock>::alloc("Stars");
	state()->joinShaderInput(starsUniforms);

	color_ = ref_ptr<ShaderInput3f>::alloc("starColor");
	color_->setUniformData(defaultColor());
	starsUniforms->addUniform(color_);

	apparentMagnitude_ = ref_ptr<ShaderInput1f>::alloc("apparentMagnitude");
	apparentMagnitude_->setUniformData(defaultApparentMagnitude());
	starsUniforms->addUniform(apparentMagnitude_);

	colorRatio_ = ref_ptr<ShaderInput1f>::alloc("colorRatio");
	colorRatio_->setUniformData(defaultColorRatio());
	starsUniforms->addUniform(colorRatio_);

	glareIntensity_ = ref_ptr<ShaderInput1f>::alloc("glareIntensity");
	glareIntensity_->setUniformData(0.1);
	starsUniforms->addUniform(glareIntensity_);

	glareScale_ = ref_ptr<ShaderInput1f>::alloc("glareScale");
	glareScale_->setUniformData(defaultGlareScale());
	starsUniforms->addUniform(glareScale_);

	scintillation_ = ref_ptr<ShaderInput1f>::alloc("scintillation");
	scintillation_->setUniformData(defaultScintillation());
	starsUniforms->addUniform(scintillation_);

	scattering_ = ref_ptr<ShaderInput1f>::alloc("scattering");
	scattering_->setUniformData(defaultScattering());
	starsUniforms->addUniform(scattering_);

	scale_ = ref_ptr<ShaderInput1f>::alloc("scale");
	scale_->setUniformData(2.0f);
	starsUniforms->addUniform(scale_);

	noiseTexState_ = ref_ptr<TextureState>::alloc();
	updateNoiseTexture();
	state()->joinStates(noiseTexState_);

	shaderState_ = ref_ptr<HasShader>::alloc("regen.weather.bright-stars");
	meshState_ = ref_ptr<Mesh>::alloc(GL_POINTS, VBO::USAGE_STATIC);
	pos_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_POS);
	col_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_COL0);
}


#define _rightascd(deg, min, sec) \
    (_decimal(deg, min, sec) * 15.0L)

#define _rightasc(deg, min, sec) \
    (_rad(_rightascd(deg, min, sec)))

void Stars::set_brightStarsFile(const std::string &brightStars) {
	osgHimmel::BrightStars bs(brightStars.c_str());
	const osgHimmel::BrightStars::s_BrightStar *stars = bs.stars();
	if (bs.numStars() == 0) {
		REGEN_WARN("Unable to load bright stars catalog at " << brightStars << ".");
		return;
	}

	pos_->setVertexData(bs.numStars());
	col_->setVertexData(bs.numStars());

	for (unsigned int i = 0; i < bs.numStars(); ++i) {
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

	meshState_->begin(ShaderInputContainer::INTERLEAVED);
	meshState_->setInput(pos_);
	meshState_->setInput(col_);
	meshState_->end();
}

void Stars::updateNoiseTexture() {
	const int noiseN = 256;

	byte *noiseMap = new byte[noiseN];
	osgHimmel::RandomMapGenerator::generate1(noiseN, 1, noiseMap);

	noiseTex_ = ref_ptr<Texture1D>::alloc();
	noiseTex_->set_rectangleSize(noiseN, 1);
	noiseTex_->set_format(GL_RED);
	noiseTex_->set_internalFormat(GL_R8);
	noiseTex_->set_pixelType(GL_UNSIGNED_BYTE);
	noiseTex_->set_textureData(noiseMap);
	noiseTex_->begin(RenderState::get());
	noiseTex_->texImage();
	noiseTex_->filter().push(GL_LINEAR);
	noiseTex_->wrapping().push(GL_REPEAT);
	noiseTex_->end(RenderState::get());
	noiseTex_->set_textureData(nullptr);

	delete[]noiseMap;

	noiseTexState_->set_texture(noiseTex_);
	noiseTexState_->set_name("noiseTexture");
}

float Stars::defaultApparentMagnitude() { return 7.0f; }

Vec3f Stars::defaultColor() { return {0.66, 0.78, 1.0}; }

float Stars::defaultColorRatio() { return 0.66f; }

float Stars::defaultGlareScale() { return 1.2f; }

float Stars::defaultScintillation() { return 0.2f; }

float Stars::defaultScattering() { return 2.0f; }
