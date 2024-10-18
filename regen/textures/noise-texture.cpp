/*
 * noise-texture.cpp
 *
 *  Created on: 29.12.2012
 *      Author: daniel
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <regen/external/libnoise/src/noise/noise.h>
#include <regen/math/vector.h>
#include <regen/gl-types/render-state.h>

#include "noise-texture.h"

using namespace regen;
using namespace noise;
using namespace regen::textures;

static GLfloat sampleNoise(
		module::Module &noiseGen,
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

static ref_ptr<Texture2D> noise2D(
		module::Module &noiseGen,
		GLuint width,
		GLuint height,
		GLboolean isSeamless) {
	auto *data = new GLubyte[width * height];
	GLubyte *dataPtr = data;
	for (GLuint x = 0u; x < width; ++x) {
		for (GLuint y = 0u; y < height; ++y) {
			GLfloat val = sampleNoise(noiseGen, x, y, 0.0, isSeamless, GL_FALSE);
			*dataPtr = (GLubyte) (val * 255.0f);
			++dataPtr;
		}
	}

	ref_ptr<Texture2D> tex = ref_ptr<Texture2D>::alloc();
	tex->begin(RenderState::get());
	tex->set_rectangleSize(width, height);
	tex->set_pixelType(GL_UNSIGNED_BYTE);
	tex->set_format(GL_RED);
	tex->set_internalFormat(GL_RED);
	tex->set_data(data);
	tex->texImage();
	tex->filter().push(GL_LINEAR);
	tex->wrapping().push(GL_REPEAT);
	tex->set_data(nullptr);
	delete[]data;
	tex->end(RenderState::get());

	return tex;
}

static ref_ptr<Texture3D> noise3D(
		module::Module &noiseGen,
		GLuint width,
		GLuint height,
		GLuint depth,
		GLboolean isSeamless) {
	auto *data = new GLubyte[width * height * depth];
	GLubyte *dataPtr = data;
	for (GLuint x = 0u; x < width; ++x) {
		for (GLuint y = 0u; y < height; ++y) {
			for (GLuint z = 0u; z < depth; ++z) {
				GLfloat val = sampleNoise(noiseGen, x, y, z, GL_FALSE, isSeamless);
				*dataPtr = (GLubyte) (val * 255.0f);
				++dataPtr;
			}
		}
	}

	ref_ptr<Texture3D> tex = ref_ptr<Texture3D>::alloc();
	tex->begin(RenderState::get());
	tex->set_rectangleSize(width, height);
	tex->set_depth(depth);
	tex->set_pixelType(GL_UNSIGNED_BYTE);
	tex->set_format(GL_RED);
	tex->set_internalFormat(GL_RED);
	tex->set_data(data);
	tex->texImage();
	tex->filter().push(GL_LINEAR);
	tex->wrapping().push(GL_REPEAT);
	tex->set_data(nullptr);
	delete[]data;
	tex->end(RenderState::get());

	return tex;
}

/////////
/////////

PerlinNoiseConfig::PerlinNoiseConfig()
		: baseFrequency(4.0),
		  persistence(0.5f),
		  lacunarity(2.5),
		  octaveCount(4) {}

ref_ptr<Texture2D> textures::perlin2D(
		GLuint width, GLuint height,
		const PerlinNoiseConfig &cfg,
		GLint randomSeed, GLboolean isSeamless) {
	module::Perlin perlin;
	if (randomSeed != 0) perlin.SetSeed(randomSeed);
	perlin.SetFrequency(cfg.baseFrequency);
	perlin.SetPersistence(cfg.persistence);
	perlin.SetLacunarity(cfg.lacunarity);
	perlin.SetOctaveCount(cfg.octaveCount);
	return noise2D(perlin, width, height, isSeamless);
}

ref_ptr<Texture3D> textures::perlin3D(
		GLuint width, GLuint height, GLuint depth,
		const PerlinNoiseConfig &cfg,
		GLint randomSeed, GLboolean isSeamless) {
	module::Perlin perlin;
	if (randomSeed != 0) perlin.SetSeed(randomSeed);
	perlin.SetFrequency(cfg.baseFrequency);
	perlin.SetPersistence(cfg.persistence);
	perlin.SetLacunarity(cfg.lacunarity);
	perlin.SetOctaveCount(cfg.octaveCount);
	return noise3D(perlin, width, height, depth, isSeamless);
}

///////////
///////////

ref_ptr<Texture2D> textures::clouds2D(
		GLuint width, GLuint height,
		GLint randomSeed, GLboolean isSeamless) {
	// Base of the cloud texture.
	// The billowy noise produces the basic shape of soft, fluffy clouds.
	module::Billow cloudBase;
	cloudBase.SetSeed(randomSeed);
	cloudBase.SetFrequency(2.0);
	cloudBase.SetPersistence(0.375);
	cloudBase.SetLacunarity(2.12109375);
	cloudBase.SetOctaveCount(4);
	cloudBase.SetNoiseQuality(QUALITY_BEST);

	// Perturb the cloud texture for more realism.
	module::Turbulence finalClouds;
	finalClouds.SetSourceModule(0, cloudBase);
	finalClouds.SetSeed(randomSeed + 1);
	finalClouds.SetFrequency(16.0);
	finalClouds.SetPower(1.0 / 64.0);
	finalClouds.SetRoughness(2);

	return noise2D(finalClouds, width, height, isSeamless);
}

ref_ptr<Texture2D> textures::wood(
		GLuint width, GLuint height,
		GLint randomSeed, GLboolean isSeamless) {
	// Base wood texture.  The base texture uses concentric cylinders aligned
	// on the z axis, like a log.
	module::Cylinders baseWood;
	baseWood.SetFrequency(16.0);

	// Perlin noise to use for the wood grain.
	module::Perlin woodGrainNoise;
	woodGrainNoise.SetSeed(randomSeed);
	woodGrainNoise.SetFrequency(48.0);
	woodGrainNoise.SetPersistence(0.5);
	woodGrainNoise.SetLacunarity(2.20703125);
	woodGrainNoise.SetOctaveCount(3);
	woodGrainNoise.SetNoiseQuality(QUALITY_STD);

	// Stretch the Perlin noise in the same direction as the center of the
	// log.  This produces a nice wood-grain texture.
	module::ScalePoint scaledBaseWoodGrain;
	scaledBaseWoodGrain.SetSourceModule(0, woodGrainNoise);
	scaledBaseWoodGrain.SetYScale(0.25);

	// Scale the wood-grain values so that they may be added to the base wood
	// texture.
	module::ScaleBias woodGrain;
	woodGrain.SetSourceModule(0, scaledBaseWoodGrain);
	woodGrain.SetScale(0.25);
	woodGrain.SetBias(0.125);

	// Add the wood grain texture to the base wood texture.
	module::Add combinedWood;
	combinedWood.SetSourceModule(0, baseWood);
	combinedWood.SetSourceModule(1, woodGrain);

	// Slightly perturb the wood texture for more realism.
	module::Turbulence perturbedWood;
	perturbedWood.SetSourceModule(0, combinedWood);
	perturbedWood.SetSeed(randomSeed + 1);
	perturbedWood.SetFrequency(4.0);
	perturbedWood.SetPower(1.0 / 256.0);
	perturbedWood.SetRoughness(4);

	// Cut the wood texture a small distance from the center of the "log".
	module::TranslatePoint translatedWood;
	translatedWood.SetSourceModule(0, perturbedWood);
	translatedWood.SetZTranslation(1.48);

	// Cut the wood texture on an angle to produce a more interesting wood
	// texture.
	module::RotatePoint rotatedWood;
	rotatedWood.SetSourceModule(0, translatedWood);
	rotatedWood.SetAngles(84.0, 0.0, 0.0);

	// Finally, perturb the wood texture to produce the final texture.
	module::Turbulence finalWood;
	finalWood.SetSourceModule(0, rotatedWood);
	finalWood.SetSeed(randomSeed + 2);
	finalWood.SetFrequency(2.0);
	finalWood.SetPower(1.0 / 64.0);
	finalWood.SetRoughness(4);

	return noise2D(finalWood, width, height, isSeamless);
}

ref_ptr<Texture2D> textures::granite(
		GLuint width, GLuint height,
		GLint randomSeed, GLboolean isSeamless) {
	// Primary granite texture.  This generates the "roughness" of the texture
	// when lit by a light source.
	module::Billow primaryGranite;
	primaryGranite.SetSeed(randomSeed);
	primaryGranite.SetFrequency(8.0);
	primaryGranite.SetPersistence(0.625);
	primaryGranite.SetLacunarity(2.18359375);
	primaryGranite.SetOctaveCount(6);
	primaryGranite.SetNoiseQuality(QUALITY_STD);

	// Use Voronoi polygons to produce the small grains for the granite texture.
	module::Voronoi baseGrains;
	baseGrains.SetSeed(randomSeed + 1);
	baseGrains.SetFrequency(16.0);
	baseGrains.EnableDistance(true);

	// Scale the small grain values so that they may be added to the base
	// granite texture.  Voronoi polygons normally generate pits, so apply a
	// negative scaling factor to produce bumps instead.
	module::ScaleBias scaledGrains;
	scaledGrains.SetSourceModule(0, baseGrains);
	scaledGrains.SetScale(-0.5);
	scaledGrains.SetBias(0.0);

	// Combine the primary granite texture with the small grain texture.
	module::Add combinedGranite;
	combinedGranite.SetSourceModule(0, primaryGranite);
	combinedGranite.SetSourceModule(1, scaledGrains);

	// Finally, perturb the granite texture to add realism.
	module::Turbulence finalGranite;
	finalGranite.SetSourceModule(0, combinedGranite);
	finalGranite.SetSeed(randomSeed + 2);
	finalGranite.SetFrequency(4.0);
	finalGranite.SetPower(1.0 / 8.0);
	finalGranite.SetRoughness(6);

	return noise2D(finalGranite, width, height, isSeamless);
}
