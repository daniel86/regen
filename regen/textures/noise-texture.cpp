#include <regen/external/libnoise/src/noise/noise.h>
#include <regen/compute/vector.h>
#include <regen/gl/render-state.h>

#include "noise-texture.h"

using namespace regen;
using namespace noise;

static float sampleNoise(
		const NoiseGenerator &noiseGen,
		double x, double y, double z,
		bool isSeamless2D,
		bool isSeamless3D) {
	float val;
	if (isSeamless2D) {
		double a = noiseGen.GetValue(x, y, z);
		double b = noiseGen.GetValue(x + 1.0, y, z);
		double c = noiseGen.GetValue(x, y + 1.0, z);
		double d = noiseGen.GetValue(x + 1.0, y + 1.0, z);
		val = static_cast<float>(math::mix(
				math::mix(a, b, 1.0 - x),
				math::mix(c, d, 1.0 - x), 1.0 - y));
	} else if (isSeamless3D) {
		double a0 = noiseGen.GetValue(x, y, z);
		double b0 = noiseGen.GetValue(x + 1.0, y, z);
		double c0 = noiseGen.GetValue(x, y + 1.0, z);
		double d0 = noiseGen.GetValue(x + 1.0, y + 1.0, z);
		double val0 = math::mix(
				math::mix(a0, b0, 1.0 - x),
				math::mix(c0, d0, 1.0 - x), 1.0 - y);

		double a1 = noiseGen.GetValue(x, y, z + 1.0);
		double b1 = noiseGen.GetValue(x + 1.0, y, z + 1.0);
		double c1 = noiseGen.GetValue(x, y + 1.0, z + 1.0);
		double d1 = noiseGen.GetValue(x + 1.0, y + 1.0, z + 1.0);
		double val1 = math::mix(
				math::mix(a1, b1, 1.0 - x),
				math::mix(c1, d1, 1.0 - x), 1.0 - y);

		val = static_cast<float>(math::mix(val0, val1, 1.0 - z));
	} else {
		val = static_cast<float>(noiseGen.GetValue(x, y, z));
	}
	// map roughly to [0,1] and clamp to that range
	return math::clamp((val + 1.0f) * 0.5f, 0.0f, 1.0f);
}

void NoiseTexture::setNoiseGenerator(const ref_ptr<NoiseGenerator> &generator) {
	generator_ = generator;
	updateNoise();
}

NoiseTexture2D::NoiseTexture2D(uint32_t width, uint32_t height, bool isSeamless)
		: Texture2D(), NoiseTexture(isSeamless) {
	set_rectangleSize(width, height);
	set_pixelType(GL_UNSIGNED_BYTE);
	set_format(GL_RED);
	set_internalFormat(GL_R8);
}

void NoiseTexture2D::updateNoise() {
	if (generator_.get() == nullptr) return;
	const auto &gen = *generator_.get();

	auto *data = new GLubyte[width() * height()];
	GLubyte *dataPtr = data;
	for (uint32_t x = 0u; x < width(); ++x) {
		for (uint32_t y = 0u; y < height(); ++y) {
			float fx = noiseScale_ * float(x) / float(width());
			float fy = noiseScale_ * float(y) / float(height());
			float val = sampleNoise(gen, fx, fy, 0.0, isSeamless_, false);
			*dataPtr = static_cast<GLubyte>(val * 255.0f);
			++dataPtr;
		}
	}

	allocTexture();
	updateImage(data);
	delete[]data;

	set_swizzle(Vec4i(GL_RED, GL_RED, GL_RED, GL_ONE));
	set_filter(TextureFilter::create(GL_LINEAR));
	set_wrapping(TextureWrapping::create(GL_MIRRORED_REPEAT));
}

NoiseTexture3D::NoiseTexture3D(uint32_t width, uint32_t height, uint32_t depth, bool isSeamless)
		: Texture3D(), NoiseTexture(isSeamless) {
	set_rectangleSize(width, height);
	set_depth(depth);
	set_rectangleSize(width, height);
	set_pixelType(GL_UNSIGNED_BYTE);
	set_format(GL_RED);
	set_internalFormat(GL_R8);
}

void NoiseTexture3D::updateNoise() {
	if (generator_.get() == nullptr) return;
	const auto &gen = *generator_.get();

	auto *data = new GLubyte[width() * height() * depth()];
	GLubyte *dataPtr = data;
	for (uint32_t x = 0u; x < width(); ++x) {
		for (uint32_t y = 0u; y < height(); ++y) {
			for (uint32_t z = 0u; z < depth(); ++z) {
				float fx = noiseScale_ * float(x) / float(width());
				float fy = noiseScale_ * float(y) / float(height());
				float fz = noiseScale_ * float(z) / float(depth());
				float val = sampleNoise(gen, fx, fy, fz, false, isSeamless_);
				*dataPtr = static_cast<GLubyte>(val * 255.0f);
				++dataPtr;
			}
		}
	}

	allocTexture();
	updateImage(data);
	delete[]data;
	set_swizzle(Vec4i(GL_RED, GL_RED, GL_RED, GL_ONE));
	set_filter(TextureFilter::create(GL_LINEAR));
	set_wrapping(TextureWrapping::create(GL_MIRRORED_REPEAT));
}

/////////
/////////

NoiseGenerator::NoiseGenerator(std::string_view name,
		const ref_ptr<noise::module::Module> &handle)
		: name_(name), handle_(handle)
{}

void NoiseGenerator::addSource(const ref_ptr<NoiseGenerator> &source) {
	handle_->SetSourceModule(static_cast<int>(sources_.size()), *source->handle_.get());
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

double NoiseGenerator::GetValue(double x, double y, double z) const {
	return handle_->GetValue(x, y, z);
}

ref_ptr<NoiseGenerator> NoiseGenerator::preset_perlin(int randomSeed) {
	auto perlin = ref_ptr<module::Perlin>::alloc();
	if (randomSeed != 0) perlin->SetSeed(randomSeed);
	perlin->SetFrequency(4.0);
	perlin->SetPersistence(0.5f);
	perlin->SetLacunarity(2.5);
	perlin->SetOctaveCount(4);
	return ref_ptr<NoiseGenerator>::alloc("perlin", perlin);
}

ref_ptr<NoiseGenerator> NoiseGenerator::preset_clouds(int randomSeed) {
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

ref_ptr<NoiseGenerator> NoiseGenerator::preset_wood(int randomSeed) {
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

ref_ptr<NoiseGenerator> NoiseGenerator::preset_granite(int randomSeed) {
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

ref_ptr<NoiseGenerator> loadGenerator(
		LoadingContext &ctx,
		scene::SceneInputNode &input,
		const std::map<std::string, ref_ptr<NoiseGenerator>> &generators,
		int randomSeed) {
	const std::string generatorType = input.getValue("type");
	ref_ptr<NoiseGenerator> generator;

	if (generatorType == "perlin") {
		auto perlin = ref_ptr<module::Perlin>::alloc();
		perlin->SetSeed(randomSeed);
		perlin->SetFrequency(input.getValue<double>("frequency", 4.0));
		perlin->SetPersistence(input.getValue<double>("persistence", 0.5));
		perlin->SetLacunarity(input.getValue<double>("lacunarity", 2.5));
		perlin->SetOctaveCount(input.getValue<int>("octaves", 4));
		generator = ref_ptr<NoiseGenerator>::alloc("perlin", perlin);
	}
	else if (generatorType == "billow") {
		auto billow = ref_ptr<module::Billow>::alloc();
		billow->SetSeed(randomSeed);
		billow->SetFrequency(input.getValue<double>("frequency", 4.0));
		billow->SetPersistence(input.getValue<double>("persistence", 0.5));
		billow->SetLacunarity(input.getValue<double>("lacunarity", 2.5));
		billow->SetOctaveCount(input.getValue<int>("octaves", 4));
		//billow->SetNoiseQuality(QUALITY_STD);
		generator = ref_ptr<NoiseGenerator>::alloc("billow", billow);
	}
	else if (generatorType == "turbulence") {
		auto turbulence = ref_ptr<module::Turbulence>::alloc();
		turbulence->SetSeed(randomSeed);
		turbulence->SetFrequency(input.getValue<double>("frequency", 4.0));
		turbulence->SetPower(input.getValue<double>("power", 1.0));
		turbulence->SetRoughness(input.getValue<int>("roughness", 2));
		generator = ref_ptr<NoiseGenerator>::alloc("turbulence", turbulence);
	}
	else if (generatorType == "voronoi") {
		auto voronoi = ref_ptr<module::Voronoi>::alloc();
		voronoi->SetSeed(randomSeed);
		voronoi->SetFrequency(input.getValue<double>("frequency", 4.0));
		voronoi->SetDisplacement(input.getValue<double>("displacement", 0.0));
		voronoi->EnableDistance(input.getValue<bool>("distance", false));
		generator = ref_ptr<NoiseGenerator>::alloc("voronoi", voronoi);
	}
	else if (generatorType == "cylinders") {
		auto cylinders = ref_ptr<module::Cylinders>::alloc();
		cylinders->SetFrequency(input.getValue<double>("frequency", 4.0));
		generator = ref_ptr<NoiseGenerator>::alloc("cylinders", cylinders);
	}
	else if (generatorType == "scale-bias") {
		auto scaleBias = ref_ptr<module::ScaleBias>::alloc();
		scaleBias->SetScale(input.getValue<double>("scale", 1.0));
		scaleBias->SetBias(input.getValue<double>("bias", 0.0));
		generator = ref_ptr<NoiseGenerator>::alloc("scale-bias", scaleBias);
	}
	else if (generatorType == "rotate-point") {
		auto rotatePoint = ref_ptr<module::RotatePoint>::alloc();
		rotatePoint->SetAngles(
				input.getValue<double>("angle-x", 0.0),
				input.getValue<double>("angle-y", 0.0),
				input.getValue<double>("angle-z", 0.0));
		generator = ref_ptr<NoiseGenerator>::alloc("rotate-point", rotatePoint);
	}
	else if (generatorType == "translate-point") {
		auto translatePoint = ref_ptr<module::TranslatePoint>::alloc();
		translatePoint->SetXTranslation(input.getValue<double>("x-translation", 0.0));
		translatePoint->SetYTranslation(input.getValue<double>("y-translation", 0.0));
		translatePoint->SetZTranslation(input.getValue<double>("z-translation", 0.0));
		generator = ref_ptr<NoiseGenerator>::alloc("translate-point", translatePoint);
	}
	else if (generatorType == "add") {
		generator = ref_ptr<NoiseGenerator>::alloc("add", ref_ptr<module::Add>::alloc());
	}
	else if (generatorType == "multiply") {
		generator = ref_ptr<NoiseGenerator>::alloc("multiply", ref_ptr<module::Multiply>::alloc());
	}
	else if (generatorType == "max") {
		generator = ref_ptr<NoiseGenerator>::alloc("max", ref_ptr<module::Max>::alloc());
	}
	else if (generatorType == "min") {
		generator = ref_ptr<NoiseGenerator>::alloc("min", ref_ptr<module::Min>::alloc());
	}
	else if (generatorType == "select") {
		auto select = ref_ptr<module::Select>::alloc();
		select->SetEdgeFalloff(input.getValue<double>("edge-falloff", 0.0));
		select->SetBounds(
				input.getValue<double>("lower-bound", -0.25),
				input.getValue<double>("upper-bound", 0.25));
		generator = ref_ptr<NoiseGenerator>::alloc("select", select);
	}
	else {
		REGEN_WARN("Unknown noise generator type '" << generatorType << "'.");
		return {};
	}
	for (auto &n: input.getChildren()) {
		if (n->getCategory() != "source") {
			REGEN_WARN("Unknown noise generator type '" << n->getCategory() <<
					"' in node '" << n->getDescription() << "'.");
			continue;
		}
		auto sourceId = n->getName();
		auto needle = generators.find(sourceId);
		if (needle == generators.end()) {
			REGEN_WARN("Unable to find noise generator '" << sourceId <<
					"' in node '" << n->getDescription() << "'.");
			continue;
		}
		generator->addSource(needle->second);
	}
	return generator;
}

ref_ptr<NoiseGenerator> NoiseGenerator::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto randomSeed = input.getValue<int>("random-seed", math::randomInt());

	if (input.hasAttribute("preset")) {
		auto preset = input.getValue("preset");
		if (preset == "perlin") {
			return preset_perlin(randomSeed);
		} else if (preset == "wood") {
			return preset_wood(randomSeed);
		} else if (preset == "granite") {
			return preset_granite(randomSeed);
		} else if (preset == "cloud" || preset == "clouds") {
			return preset_clouds(randomSeed);
		} else {
			REGEN_WARN("Unknown noise generator preset '" << preset << "'.");
			return {};
		}
	}
	auto outputGenerator = input.getValue("output-generator");
	if (outputGenerator.empty()) {
		REGEN_WARN("Missing 'output-generator' attribute for noise generator '" << input.getDescription() << "'.");
		return {};
	}

	std::map<std::string, ref_ptr<NoiseGenerator>> generators;
	for (auto &n: input.getChildren()) {
		if (n->getCategory() == "generator") {
			auto noiseGen = loadGenerator(ctx, *n.get(), generators, randomSeed);
			if (noiseGen.get() != nullptr) {
				generators[n->getName()] = noiseGen;
			}
		} else {
			REGEN_WARN("Unknown noise generator type '" << n->getCategory() << "'.");
		}
	}
	if (generators.empty()) {
		REGEN_WARN("No noise generators found for '" << input.getDescription() << "'.");
		return {};
	}

	auto needle = generators.find(outputGenerator);
	if (needle == generators.end()) {
		REGEN_WARN("Unable to find noise generator '" << outputGenerator << "'.");
		needle = generators.begin();
	}
	return needle->second;
}
