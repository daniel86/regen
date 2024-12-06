
#include <regen/states/atomic-states.h>

#include "mesh-lod.h"

namespace regen {
	MeshLOD::MeshLOD(const ref_ptr<Mesh> &mesh, const ref_ptr<ModelTransformation> &meshTransform)
			: State(),
			  currentLODLevel_(LOD_HIGH),
			  mesh_(mesh),
			  meshTransform_(meshTransform) {
	}

	void MeshLOD::enable(RenderState *rs) {
		State::enable(rs);
		currentLODLevel_ = lodLevel();
		mesh_->activateLOD(currentLODLevel_);
	}

	void MeshLOD::disable(regen::RenderState *rs) {
		mesh_->activateLOD(LOD_HIGH);
	}

	CameraDistanceLOD::CameraDistanceLOD(
			const ref_ptr<Mesh> &mesh,
			const ref_ptr<ModelTransformation> &meshTransform,
			const ref_ptr<Camera> &camera)
			: MeshLOD(mesh, meshTransform),
			  camera_(camera),
			  camStamp_(0),
			  transformStamp_(0),
			  closeDistance_(40.0f),
			  mediumDistance_(100.0f) {
	}

	LODLevel CameraDistanceLOD::lodLevel() {
		auto meshStamp = meshTransform_->get()->stamp();
		auto camStamp = camera_->stamp();
		if (camStamp == camStamp_ && meshStamp == transformStamp_) {
			return currentLODLevel_;
		}
		camStamp_ = camStamp;
		transformStamp_ = meshStamp;
		auto &camPos = camera_->position()->getVertex(0);
		auto &camDir = camera_->direction()->getVertex(0);
		auto meshPos = (meshTransform_->get()->getVertex(0) * Vec3f(0.0)).xyz_();
		auto delta = meshPos - camPos;
		auto dot = delta.dot(camDir);
		// if the mesh is behind the camera we don't need to render it in high detail
		if (dot < 0) return LOD_LOW;

		auto distance = delta.length();
		if (distance > mediumDistance_) return LOD_LOW;
		else if (distance > closeDistance_) return LOD_MEDIUM;
		return LOD_HIGH;
	}

	std::ostream &operator<<(std::ostream &out, const LODMetric &mode) {
		switch (mode) {
			case LOD_CAMERA_DISTANCE:
				return out << "camera_distance";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, LODMetric &mode) {
		std::string val;
		in >> val;
		boost::to_lower(val);
		if (val == "camera_distance") mode = LOD_CAMERA_DISTANCE;
		else {
			REGEN_WARN("Unknown LODMetric: " << val);
			mode = LOD_CAMERA_DISTANCE;
		}
		return in;
	}
}
