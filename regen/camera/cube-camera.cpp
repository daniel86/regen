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

	{
		viewData_.resize(numLayer_ * 2, Mat4f::identity());
		view_    = std::span<Mat4f>(viewData_).subspan(0, numLayer_);
		viewInv_ = std::span<Mat4f>(viewData_).subspan(numLayer_, numLayer_);
		sh_view_->set_numArrayElements(numLayer_);
		sh_viewInv_->set_numArrayElements(numLayer_);
		sh_view_->setUniformUntyped();
		sh_viewInv_->setUniformUntyped();
	}
	{
		viewProjData_.resize(numLayer_ * 2, Mat4f::identity());
		viewProj_    = std::span<Mat4f>(viewProjData_).subspan(0, numLayer_);
		viewProjInv_ = std::span<Mat4f>(viewProjData_).subspan(numLayer_, numLayer_);
		sh_viewProj_->set_numArrayElements(numLayer_);
		sh_viewProjInv_->set_numArrayElements(numLayer_);
		sh_viewProj_->setUniformUntyped();
		sh_viewProjInv_->setUniformUntyped();
	}

	// Initialize directions
	direction_.resize(numLayer_);
	sh_direction_->set_numArrayElements(numLayer_);
	sh_direction_->setUniformUntyped();
	for (auto i = 0; i < 6; ++i) {
		// Set the cube face directions
		direction_[i] = Vec4f::create(Mat4f::cubeDirections()[i], 0.0f);
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
