/*
 * light.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "light.h"
#include "regen/scene/states/input.h"
#include "regen/animations/boids.h"

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

	// process light node children
	for (auto &child : input.getChildren()) {
		if (child->getCategory() == "set") {
			// set a given light input. The input key is given by the "target" attribute.
			auto targetName = child->getValue("target");
			// find the shader input in the light state
			auto target_opt = light->findShaderInput(targetName);
			if (!target_opt) {
				REGEN_WARN("Cannot find light input for set in node " << child->getDescription());
				continue;
			}
			auto setTarget = target_opt.value().in;
			auto numInstances = std::max(
				child->getValue<GLuint>("num-instances", 1u),
				setTarget->numInstances());
			// allocate memory for the shader input
			setTarget->setInstanceData(numInstances, 1, nullptr);
			InputStateProvider::setInput(*child.get(), setTarget.get(), numInstances);
		}
		if (child->getCategory() == "animation") {
			auto animationType = child->getValue("type");
			if (animationType == "boids") {
				// let a boid simulation change the light positions
				auto boidsAnimation = ref_ptr<BoidsSimulation_CPU>::alloc(light->position());
				boidsAnimation->loadSettings(parser, child);
				light->attach(boidsAnimation);
				boidsAnimation->startAnimation();
			} else {
				REGEN_WARN("Unknown animation type '" << animationType << "' in node " << child->getDescription());
			}
		}
	}

	return light;
}


