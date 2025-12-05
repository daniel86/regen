#include "sky.h"
#include "cloud-layer.h"
#include "atmosphere.h"
#include "moon.h"
#include "earth.h"
#include "bright-stars.h"
#include "star-map.h"
#include "regen/utility/filesystem.h"
#include "darkness.h"
#include "regen/scene/state-configurer.h"

using namespace regen;

Sky::Sky(const ref_ptr<Camera> &cam, const ref_ptr<Screen> &screen)
		: StateNode(),
		  Animation(true, true),
		  cam_(cam),
		  screen_(screen) {
	depth_ = ref_ptr<DepthState>::alloc();
	depth_->set_depthFunc(GL_LEQUAL);
	depth_->set_depthRange(1.0, 1.0);
	depth_->set_useDepthWrite(false);
	depth_->set_useDepthTest(true);

	noonColor_ = Vec3f(0.5, 0.5, 0.5);
	dawnColor_ = Vec3f(0.2, 0.15, 0.15);
	moonSunLightReflectance_ = 0.4;

	astro_ = ref_ptr<Astronomy>::alloc();
	astro_->setLatitude(52.5491);
	astro_->setLongitude(13.3611);

	auto uniformBlock = ref_ptr<UBO>::alloc("Sky", BufferUpdateFlags::FULL_PER_FRAME);

	// 0: altitude in km
	// 1: apparent angular radius (not diameter!)
	// 2: radius up to "end of atm"
	// 3: seed (for randomness of stuff)
	cmnUniform_ = ref_ptr<ShaderInput4f>::alloc("cmn");
	cmnUniform_->setUniformData(Vec4f(
			0.043,
			Earth::meanRadius(),
			Earth::meanRadius() + Earth::atmosphereThicknessNonUniform(),
			math::random<float>()));
	uniformBlock->addStagedInput(cmnUniform_);

	R_ = ref_ptr<ShaderInputMat4>::alloc("equToHorMatrix");
	R_->setUniformData(Mat4f::identity());
	uniformBlock->addStagedInput(R_);

	q_ = ref_ptr<ShaderInput1f>::alloc("q");
	q_->setUniformData(0.0f);
	uniformBlock->addStagedInput(q_);

	sqrt_q_ = ref_ptr<ShaderInput1f>::alloc("sqrt_q");
	sqrt_q_->setUniformData(0.0f);
	uniformBlock->addStagedInput(sqrt_q_);

	// directional light that approximates the sun
	sun_ = ref_ptr<Light>::alloc(Light::DIRECTIONAL);
	sun_->set_isAttenuated(false);
	sun_->setSpecular(0, Vec3f::zero());
	sun_->setDiffuse(0, Vec3f::zero());
	sun_->setDirection(0, Vec3f::one());
	state()->setInput(sun_->lightUBO(), "SunLight", "_Sun");

	// directional light that approximates the moon
	moon_ = ref_ptr<Light>::alloc(Light::DIRECTIONAL);
	moon_->set_isAttenuated(false);
	moon_->setSpecular(0, Vec3f::zero());
	moon_->setDiffuse(0, Vec3f::zero());
	moon_->setDirection(0, Vec3f::one());
	state()->setInput(moon_->lightUBO(), "MoonLight", "_Moon");

	state()->setInput(uniformBlock);

	Rectangle::Config cfg;
	cfg.centerAtOrigin = false;
	cfg.isNormalRequired = false;
	cfg.isTangentRequired = false;
	cfg.isTexcoRequired = false;
	cfg.levelOfDetails = {4};
	cfg.posScale = Vec3f::create(2.0f);
	cfg.rotation = Vec3f(0.5 * M_PI, 0.0f, 0.0f);
	cfg.texcoScale = Vec2f::one();
	cfg.translation = Vec3f(-1.0f, -1.0f, 0.0f);
	cfg.updateHint.frequency = BUFFER_UPDATE_NEVER;
	cfg.updateHint.scope = BUFFER_UPDATE_FULLY;
	cfg.mapMode = BUFFER_MAP_DISABLED;
	cfg.accessMode = BUFFER_CPU_WRITE;
	skyQuad_ = Rectangle::create(cfg);

	// mae some parts of the sky configurable from the GUI.
	setAnimationName("sky");
	// make sure the client buffer data is initialized
	Sky::cpuUpdate(0.0f);

	joinAnimationState(state());
	GL_ERROR_LOG();
}

Astronomy &Sky::astro() { return *astro_.get(); }

void Sky::set_moonSunLightReflectance(
		float moonSunLightReflectance) { moonSunLightReflectance_ = moonSunLightReflectance; }

double Sky::altitude() const { return cmnUniform_->getVertex(0).r.x; }

double Sky::longitude() const { return astro_->getLongitude(); }

double Sky::latitude() const { return astro_->getLatitude(); }

void Sky::set_altitude(const float altitude) {
	auto v_cmnUniform = cmnUniform_->mapClientVertex<Vec4f>(BUFFER_GPU_READ | BUFFER_GPU_WRITE, 0);
	v_cmnUniform.w = Vec4f(
			math::clamp(altitude, 0.001f, Earth::atmosphereThicknessNonUniform()),
			v_cmnUniform.r.y,
			v_cmnUniform.r.z,
			v_cmnUniform.r.w);
}

void Sky::set_longitude(const float longitude) {
	astro_->setLongitude(longitude);
}

void Sky::set_latitude(const float latitude) {
	astro_->setLatitude(latitude);
}

void Sky::updateSeed() {
	auto v_cmnUniform = cmnUniform_->mapClientVertex<Vec4f>(BUFFER_GPU_READ | BUFFER_GPU_WRITE, 0);
	v_cmnUniform.w = Vec4f(
			v_cmnUniform.r.x,
			v_cmnUniform.r.y,
			v_cmnUniform.r.z,
			math::random<float>());
}

void Sky::addLayer(const ref_ptr<SkyLayer> &layer) {
	//addChild(layer);
	this->layer_.push_back(layer);
}

void Sky::createShader() {
	for (auto &layer: layer_) {
		layer->createUpdateShader();
	}
}

void Sky::startAnimation() {
	if (isRunning()) return;
	Animation::startAnimation();
}

void Sky::stopAnimation() {
	if (!isRunning()) return;
	Animation::stopAnimation();
}

float Sky::computeHorizonExtinction(const Vec3f &position, const Vec3f &dir, float planetRadius) {
	float u = dir.dot(-position);
	if (u < 0.0) {
		return 1.0;
	}
	Vec3f near = position + dir * u;
	if (near.length() < planetRadius) {
		return 0.0;
	} else {
		near.normalize();
		Vec3f v2 = near * planetRadius - position;
		v2.normalize();
		float diff = acos(v2.dot(dir));
		return math::smoothstep(0.0f, 1.0f, powf(diff * 2.0f, 3.0f));
	}
}

float Sky::computeEyeExtinction(const Vec3f &eyeDir, float planetRadius) {
	const Vec3f eyePosition(0.0, planetRadius, 0.0);
	return computeHorizonExtinction(eyePosition, eyeDir, planetRadius);
}

static Vec3f computeColor(const Vec3f &color, float ext) {
	if (ext >= 0.0) {
		return color;
	} else {
		return color * (-ext * ext + 1.0f);
	}
}

void Sky::cpuUpdate(double dt) {
	if (worldTime_) {
		time_osg_.sett(boost::posix_time::to_time_t(worldTime_->p_time));
		time_osg_.setUtcOffset(14400); // UTC+4, Berlin time
		astro_->update(osgHimmel::t_aTime::fromTimeF(time_osg_));
	}

	const float planetRadius = Earth::meanRadius();
	// Compute sun/moon directions
	Vec3f moon = astro_->getMoonPosition(false);
	Vec3f sun = astro_->getSunPosition(false);
	sun_->setDirection(0, sun);
	moon_->setDirection(0, moon);
	// Compute sun/moon diffuse color
	float sunExt = computeEyeExtinction(sun, planetRadius);
	Vec3f sunColor = math::mix(dawnColor_, noonColor_, abs(sunExt));
	sun_->setDiffuse(0, computeColor(
			sunColor,
			sunExt));
	moon_->setDiffuse(0, computeColor(
			sunColor * moonSunLightReflectance_,
			computeEyeExtinction(moon, planetRadius)));
	R_->setVertex(0, astro().getEquToHorTransform());

	if (camStamp_ != cam_->stamp() || viewportStamp_ != screen_->stampOfWriteData()) {
		const float fovHalf = camera()->projParams()[0].fov * 0.5f * DEGREE_TO_RAD;
		const float height = static_cast<float>(screen_->viewport().r.y);
		const float q = 2.8284271247461903f // = sqrt(2.0f) * 2.0f
						* tan(fovHalf) / height; // q is the distance from the camera to the sky quad
		q_->setVertex(0, q);
		sqrt_q_->setVertex(0, sqrt(q));
		camStamp_ = cam_->stamp();
		viewportStamp_ = screen_->stampOfWriteData();
	}

	// Update random number in cmn uniform
	updateSeed();
}

void Sky::gpuUpdate(RenderState *rs, double dt) {
	bool needsUpdate = false;
	for (auto &layer: layer_) {
		if (layer->advanceTime(dt)) {
			needsUpdate = true;
			break;
		}
	}
	if (!needsUpdate) {
		return;
	}
	//state_->enable(rs);
	depth_->enable(rs);
	for (auto &layer: layer_) {
		layer->updateSky(rs, dt);
	}
	depth_->disable(rs);
	//state_->disable(rs);
}

SkyView::SkyView(const ref_ptr<Sky> &sky)
		: StateNode(),
		  sky_(sky) {
	for (auto &layer: sky->layer_) {
		addLayer(layer);
	}
}

void SkyView::addLayer(const ref_ptr<SkyLayer> &layer) {
	ref_ptr<SkyLayerView> view = ref_ptr<SkyLayerView>::alloc(sky_, layer);
	addChild(view);
	this->layer_.push_back(view);
}

void SkyView::traverse(RenderState *rs) {
	if (!isHidden_ && !state_->isHidden() && !layer_.empty()) {
		sky_->state()->enable(rs);
		sky_->depth_->enable(rs);
		state()->enable(rs);

		// render first layer without blending
		auto &firstLayer = layer_.front();
		firstLayer->traverse(rs);

		if (layer_.size()>1) {
			rs->toggles().push(RenderState::BLEND, true);
			for (size_t i = 1; i < layer_.size(); ++i) {
				auto &layer = layer_[i];
				if (layer->isHidden()) {
					continue;
				}
				layer->traverse(rs);
			}
			rs->toggles().pop(RenderState::BLEND);
		}

		state()->disable(rs);
		sky_->depth_->disable(rs);
		sky_->state()->disable(rs);
	}
}

void SkyView::createShader(RenderState *, const StateConfig &stateCfg) {
	for (auto &layer: layer_) {
		StateConfigurer cfg(stateCfg);
		cfg.addNode(layer.get());

		auto hasShader = layer->getShaderState();
		auto mesh = layer->getMeshState();

		// replicate each mesh LOD numLayer times for indirect multi-layer rendering
		const uint32_t numRenderLayer = cfg.cfg().numRenderLayer();
		if (numRenderLayer > 1) {
			REGEN_INFO("Using indirect multi-layer sky-pass with " << numRenderLayer << " layers.");
			mesh->createIndirectDrawBuffer(numRenderLayer);
		}

		hasShader->createShader(cfg.cfg());
		mesh->updateVAO(cfg.cfg(), hasShader->shaderState()->shader());
	}
}

ref_ptr<SkyView> SkyView::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	ref_ptr<Sky> skyNode = ctx.scene()->getResource<Sky>(input.getName());
	if (skyNode.get() == nullptr) {
		REGEN_WARN("Unable to load sky for '" << input.getDescription() << "'.");
		return {};
	}
	ref_ptr<SkyView> view = ref_ptr<SkyView>::alloc(skyNode);

	ctx.parent()->addChild(view);
	StateConfigurer stateConfigurer;
	stateConfigurer.addNode(skyNode.get());
	stateConfigurer.addNode(view.get());
	view->createShader(RenderState::get(), stateConfigurer.cfg());

	return view;
}

static ref_ptr<Darkness> createDarknessLayer(const ref_ptr<Sky> &sky,
											 scene::SceneInputNode &input) {
	auto darkness = ref_ptr<Darkness>::alloc(sky, input.getValue<uint32_t>("lod", 0));

	darkness->set_updateInterval(
			input.getValue<double>("update-interval", 4000.0));
	sky->addLayer(darkness);

	return darkness;
}

static ref_ptr<StarMap> createStarMapLayer(const ref_ptr<Sky> &sky,
										   scene::SceneInputNode &input) {
	ref_ptr<StarMap> starMap = ref_ptr<StarMap>::alloc(sky, input.getValue<uint32_t>("lod", 0));

	if (input.hasAttribute("texture"))
		starMap->set_texture(input.getValue("texture"));

	if (input.hasAttribute("scattering"))
		starMap->set_scattering(input.getValue<float>("scattering", StarMap::defaultScattering()));

	if (input.hasAttribute("apparent-magnitude"))
		starMap->set_apparentMagnitude(input.getValue<float>("apparent-magnitude", 6.0));

	if (input.hasAttribute("delta-magnitude"))
		starMap->set_deltaMagnitude(input.getValue<float>("delta-magnitude", 0.5));

	starMap->set_updateInterval(
			input.getValue<double>("update-interval", 4000.0));
	sky->addLayer(starMap);

	return starMap;
}

static ref_ptr<BrightStars> createStarsLayer(
		const ref_ptr<Sky> &sky,
		scene::SceneInputNode &input) {
	ref_ptr<BrightStars> stars = ref_ptr<BrightStars>::alloc(sky);

	if (input.hasAttribute("catalog"))
		stars->set_brightStarsFile(input.getValue("catalog"));

	if (input.hasAttribute("scattering"))
		stars->set_scattering(input.getValue<float>("scattering", BrightStars::defaultScattering()));

	if (input.hasAttribute("apparent-magnitude"))
		stars->set_apparentMagnitude(
				input.getValue<float>("apparent-magnitude", BrightStars::defaultApparentMagnitude()));

	if (input.hasAttribute("color"))
		stars->set_color(input.getValue<Vec3f>("color", BrightStars::defaultColor()));

	if (input.hasAttribute("color-ratio"))
		stars->set_colorRatio(input.getValue<float>("color-ratio", BrightStars::defaultColorRatio()));

	if (input.hasAttribute("glare-intensity"))
		stars->set_glareIntensity(input.getValue<float>("glare-intensity", 1.0f));

	if (input.hasAttribute("glare-scale"))
		stars->set_glareScale(input.getValue<float>("glare-scale", BrightStars::defaultGlareScale()));

	if (input.hasAttribute("scintillation"))
		stars->set_scintillation(input.getValue<float>("scintillation", BrightStars::defaultScintillation()));

	if (input.hasAttribute("scale"))
		stars->set_scale(input.getValue<float>("scale", 1.0f));

	stars->set_updateInterval(
			input.getValue<double>("update-interval", 4000.0));
	sky->addLayer(stars);

	return stars;
}

static ref_ptr<Moon> createMoonLayer(const ref_ptr<Sky> &sky,
									 scene::SceneInputNode &input) {
	const std::string textureFile = resourcePath(input.getValue("texture"));
	ref_ptr<Moon> moon = ref_ptr<Moon>::alloc(sky, textureFile);

	if (input.hasAttribute("scale"))
		moon->set_scale(input.getValue<float>("scale", Moon::defaultScale()));

	if (input.hasAttribute("scattering"))
		moon->set_scattering(input.getValue<float>("scattering", Moon::defaultScattering()));

	if (input.hasAttribute("sun-shine-color"))
		moon->set_sunShineColor(input.getValue<Vec3f>("sun-shine-color", Moon::defaultSunShineColor()));

	if (input.hasAttribute("earth-shine-color"))
		moon->set_earthShineColor(input.getValue<Vec3f>("earth-shine-color", Moon::defaultEarthShineColor()));

	if (input.hasAttribute("sun-shine-intensity"))
		moon->set_sunShineIntensity(
				input.getValue<float>("sun-shine-intensity", Moon::defaultSunShineIntensity()));

	if (input.hasAttribute("earth-shine-intensity"))
		moon->set_earthShineIntensity(
				input.getValue<float>("earth-shine-intensity", Moon::defaultEarthShineIntensity()));

	moon->set_updateInterval(
			input.getValue<double>("update-interval", 4000.0));
	sky->addLayer(moon);

	return moon;
}

static ref_ptr<Atmosphere> createAtmosphereLayer(const ref_ptr<Sky> &sky,
												 scene::SceneLoader *parser, scene::SceneInputNode &input,
												 const std::string &skyName) {
	ref_ptr<Atmosphere> atmosphere = ref_ptr<Atmosphere>::alloc(sky,
																input.getValue<uint32_t>("size", 512),
																input.getValue<uint32_t>("use-float", false),
																input.getValue<uint32_t>("lod", 0));

	const auto preset = input.getValue<std::string>("preset", "earth");
	if (preset == "earth") atmosphere->setEarth();
	else if (preset == "mars") atmosphere->setMars();
	else if (preset == "venus") atmosphere->setVenus();
	else if (preset == "uranus") atmosphere->setUranus();
	else if (preset == "alien") atmosphere->setAlien();
	else if (preset == "custom") {
		auto absorption = input.getValue<Vec3f>("absorbtion", Vec3f(
				0.18867780436772762,
				0.4978442963618773,
				0.6616065586417131));
		auto rayleigh = input.getValue<Vec3f>("rayleigh", Vec3f(19.0, 359.0, 81.0));
		auto mie = input.getValue<Vec4f>("mie", Vec4f(44.0, 308.0, 39.0, 74.0));
		auto spot = input.getValue<float>("spot", 373.0);
		auto strength = input.getValue<float>("strength", 54.0);

		atmosphere->setRayleighBrightness(rayleigh.x);
		atmosphere->setRayleighStrength(rayleigh.y);
		atmosphere->setRayleighCollect(rayleigh.z);
		atmosphere->setMieBrightness(mie.x);
		atmosphere->setMieStrength(mie.y);
		atmosphere->setMieCollect(mie.z);
		atmosphere->setMieDistribution(mie.w);
		atmosphere->setSpotBrightness(spot);
		atmosphere->setScatterStrength(strength);
		atmosphere->setAbsorption(absorption);
	} else
		REGEN_WARN("Ignoring unknown sky preset '" << preset <<
												   "' for node " << input.getDescription() << ".");

	atmosphere->set_updateInterval(
			input.getValue<double>("update-interval", 4000.0));

	parser->putResource<Texture>(skyName, atmosphere->cubeMap());

	sky->addLayer(atmosphere);

	return atmosphere;
}

static ref_ptr<CloudLayer>
createCloudLayer(const ref_ptr<Sky> &sky, scene::SceneLoader *parser, scene::SceneInputNode &input) {
	ref_ptr<CloudLayer> cloudLayer = ref_ptr<CloudLayer>::alloc(sky,
																input.getValue<uint32_t>("texture-size", 2048));

	if (input.hasAttribute("use-scatter") && std::string("TRUE") == (input.getValue("use-scatter")))
		cloudLayer->state()->shaderDefine("USE_SCATTER", "TRUE");

	if (input.hasAttribute("altitude"))
		cloudLayer->set_altitude(input.getValue<float>("altitude", CloudLayer::defaultAltitudeHigh()));
	if (input.hasAttribute("sharpness"))
		cloudLayer->set_sharpness(input.getValue<float>("sharpness", 0.5f));
	if (input.hasAttribute("coverage"))
		cloudLayer->set_coverage(input.getValue<float>("coverage", 0.2f));
	if (input.hasAttribute("change"))
		cloudLayer->set_change(input.getValue<float>("change", CloudLayer::defaultChangeHigh()));
	if (input.hasAttribute("scale"))
		cloudLayer->set_scale(input.getValue<Vec2f>("scale", CloudLayer::defaultScaleHigh()));
	if (input.hasAttribute("wind"))
		cloudLayer->set_wind(input.getValue<Vec2f>("wind", Vec2f(0.f, 0.f)));
	if (input.hasAttribute("color"))
		cloudLayer->set_color(input.getValue<Vec3f>("color", Vec3f(1.f, 1.f, 1.f)));
	if (input.hasAttribute("top-color"))
		cloudLayer->set_topColor(input.getValue<Vec3f>("top-color", Vec3f(1.f, 1.f, 1.f)));
	if (input.hasAttribute("bottom-color"))
		cloudLayer->set_bottomColor(input.getValue<Vec3f>("bottom-color", Vec3f(1.f, 1.f, 1.f)));
	if (input.hasAttribute("offset"))
		cloudLayer->set_offset(input.getValue<float>("offset", -0.5f));
	if (input.hasAttribute("thickness"))
		cloudLayer->set_thickness(input.getValue<float>("thickness", 3.0f));

	cloudLayer->set_updateInterval(
			input.getValue<double>("update-interval", 4000.0));
	sky->addLayer(cloudLayer);

	parser->putResource<FBO>(input.getName(), cloudLayer->cloudTextureFBO());
	parser->putResource<Texture>(input.getName(), cloudLayer->cloudTexture());

	return cloudLayer;
}

ref_ptr<Sky> Sky::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto scene = ctx.scene();

	if (!input.hasAttribute("camera")) {
		REGEN_WARN("No user-camera attribute specified for '" << input.getDescription() << "'.");
		return {};
	}
	ref_ptr<Camera> cam = scene->getResource<Camera>(input.getValue("camera"));
	if (cam.get() == nullptr) {
		REGEN_WARN("No Camera can be found for '" << input.getDescription() << "'.");
		return {};
	}

	ref_ptr<Sky> sky = ref_ptr<Sky>::alloc(cam, scene->screen());

	sky->setWorldTime(&scene->application()->worldTime());

	if (input.hasAttribute("noon-color"))
		sky->set_noonColor(input.getValue<Vec3f>("noon-color", Vec3f::create(0.5)));
	if (input.hasAttribute("dawn-color"))
		sky->set_dawnColor(input.getValue<Vec3f>("dawn-color", Vec3f(0.2, 0.15, 0.15)));

	if (input.hasAttribute("moon-reflectance"))
		sky->set_moonSunLightReflectance(input.getValue<float>("moon-reflectance", 0.4));

	sky->set_altitude(input.getValue<float>("altitude", 0.043));
	sky->set_longitude(input.getValue<float>("longitude", 13.3611));
	sky->set_latitude(input.getValue<float>("latitude", 52.5491));

	for (auto &n: input.getChildren()) {
		ref_ptr<SkyLayer> layer;
		if (n->getCategory() == "atmosphere") {
			layer = createAtmosphereLayer(sky, scene, *n.get(), input.getName());
		} else if (n->getCategory() == "cloud-layer") {
			layer = createCloudLayer(sky, scene, *n.get());
		} else if (n->getCategory() == "moon") {
			layer = createMoonLayer(sky, *n.get());
		} else if (n->getCategory() == "star-map") {
			layer = createStarMapLayer(sky, *n.get());
		} else if (n->getCategory() == "stars") {
			layer = createStarsLayer(sky, *n.get());
		} else if (n->getCategory() == "darkness") {
			layer = createDarknessLayer(sky, *n.get());
		}
		if (!layer.get()) {
			REGEN_WARN("No layer created for '" << n->getDescription() << "'.");
			continue;
		}
		for (auto &layerChild: n->getChildren()) {
			auto processor = scene->getStateProcessor(layerChild->getCategory());
			if (processor.get() == nullptr) {
				REGEN_WARN("No processor registered for '" << layerChild->getDescription() << "'.");
				continue;
			}
			processor->processInput(scene, *layerChild.get(), sky, layer->updateState());
		}
	}
	sky->createShader();

	// The Sky also exposes a Light (the sun) and a Texture (the cube map)
	scene->putResource<Light>(input.getName() + "-sun", sky->sun());
	scene->putResource<Light>(input.getName() + "-moon", sky->moon());
	scene->putState(input.getName() + "-sun", sky->sun());
	scene->putState(input.getName() + "-moon", sky->moon());

	sky->startAnimation();

	return sky;
}
