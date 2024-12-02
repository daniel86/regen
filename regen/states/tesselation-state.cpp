/*
 * tesselation-state.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include <regen/gl-types/gl-util.h>
#include <regen/utility/string-util.h>
#include <regen/states/atomic-states.h>

#include "tesselation-state.h"

using namespace regen;

namespace regen {
	std::ostream &operator<<(std::ostream &out, const TesselationState::LoDMetric &mode) {
		switch (mode) {
			case TesselationState::FIXED_FUNCTION:
				return out << "FIXED_FUNCTION";
			case TesselationState::EDGE_SCREEN_DISTANCE:
				return out << "EDGE_SCREEN_DISTANCE";
			case TesselationState::EDGE_DEVICE_DISTANCE:
				return out << "EDGE_DEVICE_DISTANCE";
			case TesselationState::CAMERA_DISTANCE:
				return out << "CAMERA_DISTANCE";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, TesselationState::LoDMetric &mode) {
		std::string val;
		in >> val;
		boost::to_upper(val);
		if (val == "FIXED_FUNCTION") mode = TesselationState::FIXED_FUNCTION;
		else if (val == "EDGE_SCREEN_DISTANCE") mode = TesselationState::EDGE_SCREEN_DISTANCE;
		else if (val == "EDGE_DEVICE_DISTANCE") mode = TesselationState::EDGE_DEVICE_DISTANCE;
		else if (val == "CAMERA_DISTANCE") mode = TesselationState::CAMERA_DISTANCE;
		else {
			REGEN_WARN("Unknown tesselation metric '" << val << "'. Using default CAMERA_DISTANCE blending.");
			mode = TesselationState::CAMERA_DISTANCE;
		}
		return in;
	}
}

TesselationState::TesselationState(GLuint numPatchVertices)
		: State(),
		  lodMetric_(CAMERA_DISTANCE),
		  numPatchVertices_(numPatchVertices) {
#ifdef GL_VERSION_4_0
	shaderDefine("TESS_NUM_VERTICES", REGEN_STRING(numPatchVertices));
	shaderDefine("HAS_TESSELATION", "TRUE");
	setShaderVersion(400);
#else
	REGEN_WARN("GL_ARB_tessellation_shader not supported.");
#endif

	innerLevel_ = ref_ptr<ShaderInput4f>::alloc("tessInnerLevel");
	innerLevel_->setUniformData(Vec4f(8.0f));

	outerLevel_ = ref_ptr<ShaderInput4f>::alloc("tessOuterLevel");
	outerLevel_->setUniformData(Vec4f(8.0f));

	lodFactor_ = ref_ptr<ShaderInput1f>::alloc("lodFactor");
	lodFactor_->setUniformData(4.0f);
	joinShaderInput(lodFactor_);

	joinStates(ref_ptr<PatchVerticesState>::alloc(numPatchVertices_));
}

void TesselationState::set_lodMetric(LoDMetric metric) {
	lodMetric_ = metric;

	if (tessLevelSetter_.get()) {
		disjoinStates(tessLevelSetter_);
		tessLevelSetter_ = ref_ptr<State>();
	}

	shaderDefine("TESS_IS_ADAPTIVE",
				 lodMetric_ == FIXED_FUNCTION ? "FALSE" : "TRUE");
	switch (lodMetric_) {
		case FIXED_FUNCTION:
			shaderDefine("TESS_LOD", "FIXED_FUNCTION");
			tessLevelSetter_ = ref_ptr<PatchLevelState>::alloc(innerLevel_, outerLevel_);
			joinStates(tessLevelSetter_);
			break;
		case EDGE_SCREEN_DISTANCE:
			shaderDefine("TESS_LOD", "EDGE_SCREEN_DISTANCE");
			break;
		case EDGE_DEVICE_DISTANCE:
			shaderDefine("TESS_LOD", "EDGE_DEVICE_DISTANCE");
			break;
		case CAMERA_DISTANCE:
			shaderDefine("TESS_LOD", "CAMERA_DISTANCE");
			break;
	}
}

TesselationState::LoDMetric TesselationState::lodMetric() const { return lodMetric_; }

const ref_ptr<ShaderInput4f> &TesselationState::outerLevel() const { return outerLevel_; }

const ref_ptr<ShaderInput4f> &TesselationState::innerLevel() const { return innerLevel_; }

const ref_ptr<ShaderInput1f> &TesselationState::lodFactor() const { return lodFactor_; }
