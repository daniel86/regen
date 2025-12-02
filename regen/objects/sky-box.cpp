/*
 * sky-box.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include <climits>

#include <regen/objects/primitives/rectangle.h>
#include "sky-box.h"
#include "regen/gl/states/depth-state.h"

using namespace regen;

static Box::Config cubeCfg(uint32_t levelOfDetail) {
	Box::Config cfg;
	cfg.isNormalRequired = false;
	cfg.isTangentRequired = false;
	cfg.texcoMode = Box::TEXCO_MODE_CUBE_MAP;
	cfg.updateHint.frequency = BUFFER_UPDATE_NEVER;
	cfg.updateHint.scope = BUFFER_UPDATE_FULLY;
	cfg.mapMode = BUFFER_MAP_DISABLED;
	cfg.accessMode = BUFFER_CPU_WRITE;
	cfg.levelOfDetails = {levelOfDetail};
	return cfg;
}

SkyBox::SkyBox(uint32_t levelOfDetail, const std::string &shaderKey)
		: Box(cubeCfg(levelOfDetail)), HasShader(shaderKey) {
	joinStates(ref_ptr<CullFaceState>::alloc(GL_FRONT));

	ref_ptr<DepthState> depth = ref_ptr<DepthState>::alloc();
	depth->set_depthFunc(GL_LEQUAL);
	joinStates(depth);

	joinStates(shaderState());

	shaderDefine("IGNORE_VIEW_TRANSLATION", "TRUE");
}

void SkyBox::setCubeMap(const ref_ptr<TextureCube> &cubeMap) {
	cubeMap_ = cubeMap;
	if (texState_.get()) {
		disjoinStates(texState_);
	}
	texState_ = ref_ptr<TextureState>::alloc(cubeMap_);
	texState_->set_mapTo(TextureState::MAP_TO_COLOR);
	joinStatesFront(texState_);
}

const ref_ptr<TextureCube> &SkyBox::cubeMap() const {
	return cubeMap_;
}

