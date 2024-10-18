/*
 * sky.cpp
 *
 *  Created on: Jan 5, 2014
 *      Author: daniel
 */

#include "sky.h"

using namespace regen::scene;
using namespace regen;
using namespace std;

#include <regen/scene/resource-manager.h>
#include <regen/scene/input-processors.h>

#include <regen/sky/sky.h>
#include <regen/sky/atmosphere.h>
#include <regen/sky/cloud-layer.h>

#define REGEN_SKY_CATEGORY "sky"

SkyResource::SkyResource()
		: ResourceProvider(REGEN_SKY_CATEGORY) {}

ref_ptr<Sky> SkyResource::createResource(
		SceneParser *parser, SceneInputNode &input) {
	if (!input.hasAttribute("camera")) {
		REGEN_WARN("No user-camera attribute specified for '" << input.getDescription() << "'.");
		return ref_ptr<Sky>();
	}
	ref_ptr<Camera> cam = parser->getResources()->getCamera(parser, input.getValue("camera"));
	if (cam.get() == NULL) {
		REGEN_WARN("No Camera can be found for '" << input.getDescription() << "'.");
		return ref_ptr<Sky>();
	}

	ref_ptr<Sky> sky = ref_ptr<Sky>::alloc(cam, parser->getViewport());

	if (input.hasAttribute("noon-color"))
		sky->set_noonColor(input.getValue<Vec3f>("noon-color", Vec3f(0.5)));
	if (input.hasAttribute("dawn-color"))
		sky->set_dawnColor(input.getValue<Vec3f>("dawn-color", Vec3f(0.2, 0.15, 0.15)));

	if (input.hasAttribute("moon-reflectance"))
		sky->set_moonSunLightReflectance(input.getValue<GLfloat>("moon-reflectance", 0.4));

	sky->set_altitude(input.getValue<GLfloat>("altitude", 0.043));
	sky->set_longitude(input.getValue<GLfloat>("longitude", 13.3611));
	sky->set_latitude(input.getValue<GLfloat>("latitude", 52.5491));

	if (input.hasAttribute("date"))
		sky->set_date(input.getValue("date"));
	if (input.hasAttribute("timestamp"))
		sky->set_timestamp(input.getValue<double>("timestamp", 0.0));
	sky->set_secondsPerCycle(
			input.getValue<double>("seconds-per-cycle", 3600.0));

	const list<ref_ptr<SceneInputNode> > &childs = input.getChildren();
	for (list<ref_ptr<SceneInputNode> >::const_iterator
				 it = childs.begin(); it != childs.end(); ++it) {
		ref_ptr<SceneInputNode> n = *it;

		if (n->getCategory() == "atmosphere") {
			createAtmosphereLayer(sky, parser, *n.get(), input.getName());
		} else if (n->getCategory() == "cloud-layer") {
			createCloudLayer(sky, parser, *n.get());
		} else if (n->getCategory() == "moon") {
			createMoonLayer(sky, parser, *n.get());
		} else if (n->getCategory() == "star-map") {
			createStarMapLayer(sky, parser, *n.get());
		} else if (n->getCategory() == "stars") {
			createStarsLayer(sky, parser, *n.get());
		}
	}

	// The Sky also exposes a Light (the sun) and a Texture (the cube map)
	parser->getResources()->putLight(input.getName() + "-sun", sky->sun());
	parser->getResources()->putLight(input.getName() + "-moon", sky->moon());
	parser->getResources()->putSky(input.getName(), sky);
	parser->putState(input.getName() + "-sun", sky->sun());
	parser->putState(input.getName() + "-moon", sky->moon());

	return sky;
}

ref_ptr<StarMap> SkyResource::createStarMapLayer(const ref_ptr<Sky> &sky,
												 SceneParser *parser, SceneInputNode &input) {
	ref_ptr<StarMap> starMap = ref_ptr<StarMap>::alloc(sky, input.getValue<GLuint>("lod", 0));

	if (input.hasAttribute("texture"))
		starMap->set_texture(input.getValue("texture"));

	if (input.hasAttribute("scattering"))
		starMap->set_scattering(input.getValue<GLdouble>("scattering", starMap->defaultScattering()));

	if (input.hasAttribute("apparent-magnitude"))
		starMap->set_apparentMagnitude(input.getValue<GLdouble>("apparent-magnitude", 6.0));

	starMap->set_updateInterval(
			input.getValue<GLdouble>("update-interval", 4000.0));
	sky->addLayer(starMap);

	return starMap;
}

ref_ptr<Stars> SkyResource::createStarsLayer(
		const ref_ptr<Sky> &sky,
		SceneParser *parser, SceneInputNode &input) {
	ref_ptr<Stars> stars = ref_ptr<Stars>::alloc(sky);

	if (input.hasAttribute("catalog"))
		stars->set_brightStarsFile(input.getValue("catalog"));

	if (input.hasAttribute("scattering"))
		stars->set_scattering(input.getValue<GLfloat>("scattering", stars->defaultScattering()));

	if (input.hasAttribute("apparent-magnitude"))
		stars->set_apparentMagnitude(input.getValue<GLfloat>("apparent-magnitude", stars->defaultApparentMagnitude()));

	if (input.hasAttribute("color"))
		stars->set_color(input.getValue<Vec3f>("color", stars->defaultColor()));

	if (input.hasAttribute("color-ratio"))
		stars->set_colorRatio(input.getValue<GLfloat>("color-ratio", stars->defaultColorRatio()));

	if (input.hasAttribute("glare-intensity"))
		stars->set_glareIntensity(input.getValue<GLfloat>("glare-intensity", 1.0f));

	if (input.hasAttribute("glare-scale"))
		stars->set_glareScale(input.getValue<GLfloat>("glare-scale", stars->defaultGlareScale()));

	if (input.hasAttribute("scintillation"))
		stars->set_scintillation(input.getValue<GLfloat>("scintillation", stars->defaultScintillation()));

	if (input.hasAttribute("scale"))
		stars->set_scale(input.getValue<GLfloat>("scale", 1.0f));

	stars->set_updateInterval(
			input.getValue<GLdouble>("update-interval", 4000.0));
	sky->addLayer(stars);

	return stars;
}

ref_ptr<MoonLayer> SkyResource::createMoonLayer(const ref_ptr<Sky> &sky,
												SceneParser *parser, SceneInputNode &input) {
	const string textureFile = getResourcePath(input.getValue("texture"));
	ref_ptr<MoonLayer> moon = ref_ptr<MoonLayer>::alloc(sky, textureFile);

	if (input.hasAttribute("scale"))
		moon->set_scale(input.getValue<GLdouble>("scale", moon->defaultScale()));

	if (input.hasAttribute("scattering"))
		moon->set_scattering(input.getValue<GLdouble>("scattering", moon->defaultScattering()));

	if (input.hasAttribute("sun-shine-color"))
		moon->set_sunShineColor(input.getValue<Vec3f>("sun-shine-color", moon->defaultSunShineColor()));

	if (input.hasAttribute("earth-shine-color"))
		moon->set_earthShineColor(input.getValue<Vec3f>("earth-shine-color", moon->defaultEarthShineColor()));

	if (input.hasAttribute("sun-shine-intensity"))
		moon->set_sunShineIntensity(input.getValue<GLfloat>("sun-shine-intensity", moon->defaultSunShineIntensity()));

	if (input.hasAttribute("earth-shine-intensity"))
		moon->set_earthShineIntensity(
				input.getValue<GLfloat>("earth-shine-intensity", moon->defaultEarthShineIntensity()));

	moon->set_updateInterval(
			input.getValue<GLdouble>("update-interval", 4000.0));
	sky->addLayer(moon);

	return moon;
}

ref_ptr<Atmosphere> SkyResource::createAtmosphereLayer(const ref_ptr<Sky> &sky,
													   SceneParser *parser, SceneInputNode &input,
													   const string &skyName) {
	ref_ptr<Atmosphere> atmosphere = ref_ptr<Atmosphere>::alloc(sky,
																input.getValue<GLuint>("size", 512),
																input.getValue<GLuint>("use-float", false),
																input.getValue<GLuint>("lod", 0));

	const string preset = input.getValue<string>("preset", "earth");
	if (preset == "earth") atmosphere->setEarth();
	else if (preset == "mars") atmosphere->setMars();
	else if (preset == "venus") atmosphere->setVenus();
	else if (preset == "uranus") atmosphere->setUranus();
	else if (preset == "alien") atmosphere->setAlien();
	else if (preset == "custom") {
		const Vec3f absorbtion =
				input.getValue<Vec3f>("absorbtion", Vec3f(
						0.18867780436772762,
						0.4978442963618773,
						0.6616065586417131));
		const Vec3f rayleigh =
				input.getValue<Vec3f>("rayleigh", Vec3f(19.0, 359.0, 81.0));
		const Vec4f mie =
				input.getValue<Vec4f>("mie", Vec4f(44.0, 308.0, 39.0, 74.0));
		const GLfloat spot =
				input.getValue<GLfloat>("spot", 373.0);
		const GLfloat strength =
				input.getValue<GLfloat>("strength", 54.0);

		atmosphere->setRayleighBrightness(rayleigh.x);
		atmosphere->setRayleighStrength(rayleigh.y);
		atmosphere->setRayleighCollect(rayleigh.z);
		atmosphere->setMieBrightness(mie.x);
		atmosphere->setMieStrength(mie.y);
		atmosphere->setMieCollect(mie.z);
		atmosphere->setMieDistribution(mie.w);
		atmosphere->setSpotBrightness(spot);
		atmosphere->setScatterStrength(strength);
		atmosphere->setAbsorbtion(absorbtion);
	} else REGEN_WARN("Ignoring unknown sky preset '" << preset <<
													  "' for node " << input.getDescription() << ".");

	atmosphere->set_updateInterval(
			input.getValue<GLdouble>("update-interval", 4000.0));

	parser->getResources()->putTexture(skyName, atmosphere->cubeMap());

	sky->addLayer(atmosphere);

	return atmosphere;
}

ref_ptr<CloudLayer> SkyResource::createCloudLayer(const ref_ptr<Sky> &sky,
												  SceneParser *parser, SceneInputNode &input) {
	ref_ptr<CloudLayer> cloudLayer = ref_ptr<CloudLayer>::alloc(sky,
																input.getValue<GLuint>("texture-size", 2048));

	if (input.hasAttribute("use-scatter") && string("TRUE") == (input.getValue("use-scatter")))
		cloudLayer->state()->shaderDefine("USE_SCATTER", "TRUE");

	if (input.hasAttribute("altitude"))
		cloudLayer->set_altitude(input.getValue<GLdouble>("altitude", cloudLayer->defaultAltitudeHigh()));
	if (input.hasAttribute("sharpness"))
		cloudLayer->set_sharpness(input.getValue<GLdouble>("sharpness", 0.5f));
	if (input.hasAttribute("coverage"))
		cloudLayer->set_coverage(input.getValue<GLdouble>("coverage", 0.2f));
	if (input.hasAttribute("change"))
		cloudLayer->set_change(input.getValue<GLdouble>("change", cloudLayer->defaultChangeHigh()));
	if (input.hasAttribute("scale"))
		cloudLayer->set_scale(input.getValue<Vec2f>("scale", cloudLayer->defaultScaleHigh()));
	if (input.hasAttribute("wind"))
		cloudLayer->set_wind(input.getValue<Vec2f>("wind", Vec2f(0.f, 0.f)));
	if (input.hasAttribute("color"))
		cloudLayer->set_color(input.getValue<Vec3f>("color", Vec3f(1.f, 1.f, 1.f)));
	if (input.hasAttribute("top-color"))
		cloudLayer->set_topColor(input.getValue<Vec3f>("top-color", Vec3f(1.f, 1.f, 1.f)));
	if (input.hasAttribute("bottom-color"))
		cloudLayer->set_bottomColor(input.getValue<Vec3f>("bottom-color", Vec3f(1.f, 1.f, 1.f)));
	if (input.hasAttribute("offset"))
		cloudLayer->set_offset(input.getValue<GLdouble>("offset", -0.5f));
	if (input.hasAttribute("thickness"))
		cloudLayer->set_thickness(input.getValue<GLdouble>("thickness", 3.0f));

	cloudLayer->set_updateInterval(
			input.getValue<GLdouble>("update-interval", 4000.0));
	sky->addLayer(cloudLayer);

	parser->getResources()->putFBO(input.getName(), cloudLayer->cloudTextureFBO());
	parser->getResources()->putTexture(input.getName(), cloudLayer->cloudTexture());

	return cloudLayer;
}




