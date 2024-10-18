/*
 * asset.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "asset.h"

using namespace regen::scene;
using namespace regen;

#define REGEN_ASSET_CATEGORY "asset"

AssetResource::AssetResource()
		: ResourceProvider(REGEN_ASSET_CATEGORY) {}

ref_ptr<AssetImporter> AssetResource::createResource(
		SceneParser *parser,
		SceneInputNode &input) {
	if (!input.hasAttribute("file")) {
		REGEN_WARN("Ignoring Asset '" << input.getDescription() << "' without file.");
		return {};
	}
	const std::string assetPath = getResourcePath(input.getValue("file"));
	const std::string texturePath = getResourcePath(input.getValue("texture-path"));
	auto assimpFlags = input.getValue<GLint>("import-flags", -1);

	AssimpAnimationConfig animConfig;
	animConfig.numInstances =
			input.getValue<GLuint>("animation-instances", 1u);
	animConfig.useAnimation = (animConfig.numInstances > 0) &&
							  input.getValue<bool>("use-animation", true);
	animConfig.forceStates =
			input.getValue<bool>("animation-force-states", true);
	animConfig.ticksPerSecond =
			input.getValue<GLfloat>("animation-tps", 20.0);
	animConfig.postState = input.getValue<NodeAnimation::Behavior>(
			"animation-post-state",
			NodeAnimation::BEHAVIOR_LINEAR);
	animConfig.preState = input.getValue<NodeAnimation::Behavior>(
			"animation-pre-state",
			NodeAnimation::BEHAVIOR_LINEAR);

	try {
		return ref_ptr<AssetImporter>::alloc(
				assetPath, texturePath, animConfig, assimpFlags);
	}
	catch (AssetImporter::Error &e) {
		REGEN_WARN("Unable to open Asset file: " << e.what() << ".");
		return {};
	}
}
