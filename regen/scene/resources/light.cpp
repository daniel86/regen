/*
 * light.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "light.h"

using namespace regen::scene;
using namespace regen;
using namespace std;

#define REGEN_LIGHT_CATEGORY "light"

LightResource::LightResource()
		: ResourceProvider(REGEN_LIGHT_CATEGORY) {}

ref_ptr<Light> LightResource::createResource(
		SceneParser *parser, SceneInputNode &input) {
	auto lightType = input.getValue<Light::Type>("type", Light::SPOT);
	ref_ptr<Light> light = ref_ptr<Light>::alloc(lightType);
	light->set_isAttenuated(
			input.getValue<bool>("is-attenuated", lightType != Light::DIRECTIONAL));

	auto dir = input.getValue<Vec3f>("direction", Vec3f(0.0f, 0.0f, 1.0f));
	dir.normalize();
	light->direction()->setVertex(0, dir);
	light->position()->setVertex(0,
								 input.getValue<Vec3f>("position", Vec3f(0.0f)));
	light->direction()->setVertex(0,
								  input.getValue<Vec3f>("direction", Vec3f(0.0f, 0.0f, 1.0f)));
	light->diffuse()->setVertex(0,
								input.getValue<Vec3f>("diffuse", Vec3f(1.0f)));
	light->specular()->setVertex(0,
								 input.getValue<Vec3f>("specular", Vec3f(1.0f)));
	light->radius()->setVertex(0,
							   input.getValue<Vec2f>("radius", Vec2f(50.0f)));

	auto angles = input.getValue<Vec2f>("cone-angles", Vec2f(50.0f, 55.0f));
	light->set_innerConeAngle(angles.x);
	light->set_outerConeAngle(angles.y);
	parser->putState(input.getName(), light);

	if (lightType == Light::SPOT) {
		light->startAnimation();
	}

	return light;
}


