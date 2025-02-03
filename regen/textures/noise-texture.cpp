/*
 * noise-texture.cpp
 *
 *  Created on: 29.12.2012
 *      Author: daniel
 */

#include <regen/external/libnoise/src/noise/noise.h>
#include <regen/math/vector.h>
#include <regen/gl-types/render-state.h>

#include "noise-texture.h"

using namespace regen;
using namespace noise;

static GLfloat sampleNoise(
		const NoiseGenerator &noiseGen,
		GLdouble x, GLdouble y, GLdouble z,
		GLboolean isSeamless2D,
		GLboolean isSeamless3D) {
	GLfloat val;
	if (isSeamless2D) {
		GLdouble a = noiseGen.GetValue(x, y, z);
		GLdouble b = noiseGen.GetValue(x + 1.0, y, z);
		GLdouble c = noiseGen.GetValue(x, y + 1.0, z);
		GLdouble d = noiseGen.GetValue(x + 1.0, y + 1.0, z);
		val = math::mix(
				math::mix(a, b, 1.0 - x),
				math::mix(c, d, 1.0 - x), 1.0 - y);
	} else if (isSeamless3D) {
		GLdouble a0 = noiseGen.GetValue(x, y, z);
		GLdouble b0 = noiseGen.GetValue(x + 1.0, y, z);
		GLdouble c0 = noiseGen.GetValue(x, y + 1.0, z);
		GLdouble d0 = noiseGen.GetValue(x + 1.0, y + 1.0, z);
		GLdouble val0 = math::mix(
				math::mix(a0, b0, 1.0 - x),
				math::mix(c0, d0, 1.0 - x), 1.0 - y);

		GLdouble a1 = noiseGen.GetValue(x, y, z + 1.0);
		GLdouble b1 = noiseGen.GetValue(x + 1.0, y, z + 1.0);
		GLdouble c1 = noiseGen.GetValue(x, y + 1.0, z + 1.0);
		GLdouble d1 = noiseGen.GetValue(x + 1.0, y + 1.0, z + 1.0);
		GLdouble val1 = math::mix(
				math::mix(a1, b1, 1.0 - x),
				math::mix(c1, d1, 1.0 - x), 1.0 - y);

		val = (GLfloat) math::mix(val0, val1, 1.0 - z);
	} else {
		val = (GLfloat) noiseGen.GetValue(x, y, z);
	}
	// map roughly to [0,1] and clamp to that range
	return math::clamp((val + 1.0f) * 0.5f, 0.0f, 1.0f);
}

void NoiseTexture::setNoiseGenerator(const ref_ptr<NoiseGenerator> &generator) {
	generator_ = generator;
	updateNoise();
}

NoiseTexture2D::NoiseTexture2D(GLuint width, GLuint height, GLboolean isSeamless)
		: Texture2D(), NoiseTexture(isSeamless) {
	begin(RenderState::get());
	set_rectangleSize(width, height);
	set_rectangleSize(width, height);
	set_pixelType(GL_UNSIGNED_BYTE);
	set_format(GL_RED);
	set_internalFormat(GL_RED);
	swizzle().push(Vec4i(GL_RED, GL_RED, GL_RED, GL_ONE));
	filter().push(GL_LINEAR);
	wrapping().push(GL_REPEAT);
	end(RenderState::get());
}

void NoiseTexture2D::updateNoise() {
	if (generator_.get() == nullptr) return;
	const auto &gen = *generator_.get();

	auto *data = new GLubyte[width() * height()];
	GLubyte *dataPtr = data;
	for (GLuint x = 0u; x < width(); ++x) {
		for (GLuint y = 0u; y < height(); ++y) {
			GLfloat val = sampleNoise(gen, x, y, 0.0, isSeamless_, GL_FALSE);
			*dataPtr = (GLubyte) (val * 255.0f);
			++dataPtr;
		}
	}

	begin(RenderState::get());
	set_textureData(data);
	texImage();
	set_textureData(nullptr);
	delete[]data;
	end(RenderState::get());
}

NoiseTexture3D::NoiseTexture3D(GLuint width, GLuint height, GLuint depth, GLboolean isSeamless)
		: Texture3D(), NoiseTexture(isSeamless) {
	begin(RenderState::get());
	set_rectangleSize(width, height);
	set_depth(depth);
	set_rectangleSize(width, height);
	set_pixelType(GL_UNSIGNED_BYTE);
	set_format(GL_RED);
	set_internalFormat(GL_RED);
	swizzle().push(Vec4i(GL_RED, GL_RED, GL_RED, GL_ONE));
	filter().push(GL_LINEAR);
	wrapping().push(GL_REPEAT);
	end(RenderState::get());
}

void NoiseTexture3D::updateNoise() {
	if (generator_.get() == nullptr) return;
	const auto &gen = *generator_.get();

	auto *data = new GLubyte[width() * height() * depth()];
	GLubyte *dataPtr = data;
	for (GLuint x = 0u; x < width(); ++x) {
		for (GLuint y = 0u; y < height(); ++y) {
			for (GLuint z = 0u; z < depth(); ++z) {
				GLfloat val = sampleNoise(gen, x, y, z, GL_FALSE, isSeamless_);
				*dataPtr = (GLubyte) (val * 255.0f);
				++dataPtr;
			}
		}
	}

	begin(RenderState::get());
	set_textureData(data);
	texImage();
	set_textureData(nullptr);
	delete[]data;
	end(RenderState::get());
}

/////////
/////////

NoiseGenerator::NoiseGenerator(std::string_view name,
		const ref_ptr<noise::module::Module> &handle)
		: name_(name), handle_(handle)
{}

void NoiseGenerator::addSource(const ref_ptr<NoiseGenerator> &source) {
	handle_->SetSourceModule(sources_.size(), *source->handle_.get());
	sources_.push_back(source);
}

void NoiseGenerator::removeSource(const ref_ptr<NoiseGenerator> &source) {
	for (auto it = sources_.begin(); it != sources_.end(); ++it) {
		if (*it == source) {
			sources_.erase(it);
			break;
		}
	}
}

GLdouble NoiseGenerator::GetValue(GLdouble x, GLdouble y, GLdouble z) const {
	return handle_->GetValue(x, y, z);
}

ref_ptr<NoiseGenerator> NoiseGenerator::preset_perlin(GLint randomSeed) {
	auto perlin = ref_ptr<module::Perlin>::alloc();
	if (randomSeed != 0) perlin->SetSeed(randomSeed);
	perlin->SetFrequency(4.0);
	perlin->SetPersistence(0.5f);
	perlin->SetLacunarity(2.5);
	perlin->SetOctaveCount(4);
	return ref_ptr<NoiseGenerator>::alloc("perlin", perlin);
}

ref_ptr<NoiseGenerator> NoiseGenerator::preset_clouds(GLint randomSeed) {
	// Base of the cloud texture.
	// The billowy noise produces the basic shape of soft, fluffy clouds.
	auto cloudBase = ref_ptr<module::Billow>::alloc();
	cloudBase->SetSeed(randomSeed);
	cloudBase->SetFrequency(2.0);
	cloudBase->SetPersistence(0.375);
	cloudBase->SetLacunarity(2.12109375);
	cloudBase->SetOctaveCount(4);
	cloudBase->SetNoiseQuality(QUALITY_BEST);
	auto baseGen = ref_ptr<NoiseGenerator>::alloc("base", cloudBase);

	// Perturb the cloud texture for more realism.
	auto finalClouds = ref_ptr<module::Turbulence>::alloc();
	finalClouds->SetSeed(randomSeed + 1);
	finalClouds->SetFrequency(16.0);
	finalClouds->SetPower(1.0 / 64.0);
	finalClouds->SetRoughness(2);
	auto finalGen = ref_ptr<NoiseGenerator>::alloc("perturbed", finalClouds);
	finalGen->addSource(baseGen);
	return finalGen;
}

ref_ptr<NoiseGenerator> NoiseGenerator::preset_wood(GLint randomSeed) {
	// Base wood texture.  The base texture uses concentric cylinders aligned
	// on the z axis, like a log.
	auto baseWood = ref_ptr<module::Cylinders>::alloc();
	baseWood->SetFrequency(16.0);
	auto baseGen = ref_ptr<NoiseGenerator>::alloc("base", baseWood);

	// Perlin noise to use for the wood grain.
	auto woodGrainNoise = ref_ptr<module::Perlin>::alloc();
	woodGrainNoise->SetSeed(randomSeed);
	woodGrainNoise->SetFrequency(48.0);
	woodGrainNoise->SetPersistence(0.5);
	woodGrainNoise->SetLacunarity(2.20703125);
	woodGrainNoise->SetOctaveCount(3);
	woodGrainNoise->SetNoiseQuality(QUALITY_STD);
	auto grainNoiseGen = ref_ptr<NoiseGenerator>::alloc("grain-noise", woodGrainNoise);

	// Stretch the Perlin noise in the same direction as the center of the
	// log.  This produces a nice wood-grain texture.
	auto scaledBaseWoodGrain = ref_ptr<module::ScalePoint>::alloc();
	scaledBaseWoodGrain->SetYScale(0.25);
	auto scaledGen = ref_ptr<NoiseGenerator>::alloc("scaled", scaledBaseWoodGrain);
	scaledGen->addSource(grainNoiseGen);

	// Scale the wood-grain values so that they may be added to the base wood
	// texture.
	auto woodGrain = ref_ptr<module::ScaleBias>::alloc();
	woodGrain->SetScale(0.25);
	woodGrain->SetBias(0.125);
	auto grainGen = ref_ptr<NoiseGenerator>::alloc("grain", woodGrain);
	grainGen->addSource(scaledGen);

	// Add the wood grain texture to the base wood texture.
	auto combinedGen = ref_ptr<NoiseGenerator>::alloc(
			"combined", ref_ptr<module::Add>::alloc());
	combinedGen->addSource(baseGen);
	combinedGen->addSource(grainGen);

	// Slightly perturb the wood texture for more realism.
	auto perturbedWood = ref_ptr<module::Turbulence>::alloc();
	perturbedWood->SetSeed(randomSeed + 1);
	perturbedWood->SetFrequency(4.0);
	perturbedWood->SetPower(1.0 / 256.0);
	perturbedWood->SetRoughness(4);
	auto perturbedGen = ref_ptr<NoiseGenerator>::alloc("perturbed", perturbedWood);
	perturbedGen->addSource(combinedGen);

	// Cut the wood texture a small distance from the center of the "log".
	auto translatedWood = ref_ptr<module::TranslatePoint>::alloc();
	translatedWood->SetZTranslation(1.48);
	auto translatedGen = ref_ptr<NoiseGenerator>::alloc("translated", translatedWood);
	translatedGen->addSource(perturbedGen);

	// Cut the wood texture on an angle to produce a more interesting wood
	// texture.
	auto rotatedWood = ref_ptr<module::RotatePoint>::alloc();
	rotatedWood->SetAngles(84.0, 0.0, 0.0);
	auto rotatedGen = ref_ptr<NoiseGenerator>::alloc("rotated", rotatedWood);
	rotatedGen->addSource(translatedGen);

	// Finally, perturb the wood texture to produce the final texture.
	auto finalWood = ref_ptr<module::Turbulence>::alloc();
	finalWood->SetSeed(randomSeed + 2);
	finalWood->SetFrequency(2.0);
	finalWood->SetPower(1.0 / 64.0);
	finalWood->SetRoughness(4);
	auto finalGen = ref_ptr<NoiseGenerator>::alloc("final", finalWood);
	finalGen->addSource(rotatedGen);

	return finalGen;
}

ref_ptr<NoiseGenerator> NoiseGenerator::preset_granite(GLint randomSeed) {
	// Primary granite texture.  This generates the "roughness" of the texture
	// when lit by a light source.
	auto primaryGranite = ref_ptr<module::Billow>::alloc();
	primaryGranite->SetSeed(randomSeed);
	primaryGranite->SetFrequency(8.0);
	primaryGranite->SetPersistence(0.625);
	primaryGranite->SetLacunarity(2.18359375);
	primaryGranite->SetOctaveCount(6);
	primaryGranite->SetNoiseQuality(QUALITY_STD);
	auto primaryGen = ref_ptr<NoiseGenerator>::alloc("primary", primaryGranite);

	// Use Voronoi polygons to produce the small grains for the granite texture.
	auto baseGrains = ref_ptr<module::Voronoi>::alloc();
	baseGrains->SetSeed(randomSeed + 1);
	baseGrains->SetFrequency(16.0);
	baseGrains->EnableDistance(true);
	auto baseGrainsGen = ref_ptr<NoiseGenerator>::alloc("base-grains", baseGrains);

	// Scale the small grain values so that they may be added to the base
	// granite texture.  Voronoi polygons normally generate pits, so apply a
	// negative scaling factor to produce bumps instead.
	auto scaledGrains = ref_ptr<module::ScaleBias>::alloc();
	scaledGrains->SetScale(-0.5);
	scaledGrains->SetBias(0.0);
	auto grainGen = ref_ptr<NoiseGenerator>::alloc("grains", scaledGrains);
	grainGen->addSource(baseGrainsGen);

	// Combine the primary granite texture with the small grain texture.
	auto combinedGen = ref_ptr<NoiseGenerator>::alloc(
			"combined", ref_ptr<module::Add>::alloc());
	combinedGen->addSource(primaryGen);
	combinedGen->addSource(grainGen);

	// Finally, perturb the granite texture to add realism.
	auto finalGranite = ref_ptr<module::Turbulence>::alloc();
	finalGranite->SetSeed(randomSeed + 2);
	finalGranite->SetFrequency(4.0);
	finalGranite->SetPower(1.0 / 8.0);
	finalGranite->SetRoughness(6);
	auto finalGen = ref_ptr<NoiseGenerator>::alloc("final", finalGranite);
	finalGen->addSource(combinedGen);
	return finalGen;
}
