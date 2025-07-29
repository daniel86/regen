#include "cube-camera.h"

using namespace regen;

CubeCamera::CubeCamera(int hiddenFacesMask)
		: Camera(6),
		  hiddenFacesMask_(hiddenFacesMask) {
	shaderDefine("RENDER_TARGET", "CUBE");
	// for now always use sphere for intersection tests, else a test is done
	// for each face, which might be slower in most cases (at least with layered rendering).
	isOmni_ = true;
	//isOmni_ = (hiddenFacesMask_ == 0);

	// Set matrix array size
	// NOTE: the matrices are indexed by layer, so even if some faces are hidden,
	// the matrices are still allocated for all faces. Could change the shaders to
	// compute index based on layer and face visibility, instead of using gl_Layer directly.
	view_.resize(numLayer_);
	viewInv_.resize(numLayer_);
	viewProj_.resize(numLayer_);
	viewProjInv_.resize(numLayer_);

	sh_view_->set_numArrayElements(numLayer_);
	sh_viewInv_->set_numArrayElements(numLayer_);
	sh_viewProj_->set_numArrayElements(numLayer_);
	sh_viewProjInv_->set_numArrayElements(numLayer_);

	// Allocate matrices
	sh_view_->setUniformUntyped();
	sh_viewInv_->setUniformUntyped();
	sh_viewProj_->setUniformUntyped();
	sh_viewProjInv_->setUniformUntyped();

	// Initialize directions
	direction_.resize(numLayer_);
	sh_direction_->set_numArrayElements(numLayer_);
	sh_direction_->setUniformUntyped();
	for (auto i = 0; i < 6; ++i) {
		// Set the cube face directions
		direction_[i] = Vec4f(Mat4f::cubeDirections()[i], 0.0f);
		sh_direction_->setVertex(i, direction_[i]);
		if (!isCubeFaceVisible(i)) {
			shaderDefine(REGEN_STRING("SKIP_LAYER" << i), "1");
		}
	}

	// TODO: make this configurable
	setPerspective(1.0f, 90.0f, 0.1f, 100.0f);
}

bool CubeCamera::isCubeFaceVisible(int face) const {
	switch (face) {
		case 0: return !(hiddenFacesMask_ & POS_X);
		case 1: return !(hiddenFacesMask_ & NEG_X);
		case 2: return !(hiddenFacesMask_ & POS_Y);
		case 3: return !(hiddenFacesMask_ & NEG_Y);
		case 4: return !(hiddenFacesMask_ & POS_Z);
		case 5: return !(hiddenFacesMask_ & NEG_Z);
		default: return false;
	}
}

bool CubeCamera::updateView() {
	auto posStamp = positionStamp();
	if (posStamp == posStamp_) { return false; }
	posStamp_ = posStamp;

	const Vec3f *dir = Mat4f::cubeDirections();
	const Vec3f *up = Mat4f::cubeUpVectors();
	for (int i = 0; i < 6; ++i) {
		if (isCubeFaceVisible(i)) {
			setView(i, Mat4f::lookAtMatrix(position(0), dir[i], up[i]));
			setViewInverse(i, view(i).lookAtInverse());
		}
	}
	return true;
}

void CubeCamera::updateViewProjection1() {
	for (int i = 0; i < 6; ++i) {
		if (isCubeFaceVisible(i)) {
			updateViewProjection(0, i);
		}
	}
	updateFrustumBuffer();
}

namespace regen {
	std::ostream &operator<<(std::ostream &out, const CubeCamera::Face &face) {
		switch (face) {
			case CubeCamera::POS_X:
				return out << "pos_x";
			case CubeCamera::NEG_X:
				return out << "neg_x";
			case CubeCamera::POS_Y:
				return out << "pos_y";
			case CubeCamera::NEG_Y:
				return out << "neg_y";
			case CubeCamera::POS_Z:
				return out << "pos_z";
			case CubeCamera::NEG_Z:
				return out << "neg_z";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, CubeCamera::Face &face) {
		std::string val;
		in >> val;
		boost::to_lower(val);
		if (val == "pos_x") face = CubeCamera::POS_X;
		else if (val == "neg_x") face = CubeCamera::NEG_X;
		else if (val == "pos_y") face = CubeCamera::POS_Y;
		else if (val == "neg_y") face = CubeCamera::NEG_Y;
		else if (val == "pos_z") face = CubeCamera::POS_Z;
		else if (val == "neg_z") face = CubeCamera::NEG_Z;
		else if (val == "left") face = CubeCamera::NEG_X;
		else if (val == "right") face = CubeCamera::POS_X;
		else if (val == "top") face = CubeCamera::POS_Y;
		else if (val == "bottom") face = CubeCamera::NEG_Y;
		else if (val == "front") face = CubeCamera::POS_Z;
		else if (val == "back") face = CubeCamera::NEG_Z;
		else {
			REGEN_WARN("Unknown cube face '" << val << "'. Using default POS_X.");
			face = CubeCamera::POS_X;
		}
		return in;
	}
}
