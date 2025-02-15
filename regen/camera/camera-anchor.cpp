#include "camera-anchor.h"

using namespace regen;

TransformCameraAnchor::TransformCameraAnchor(const ref_ptr<ModelTransformation> &transform)
		: transform_(transform), mode_(LOOK_AT_FRONT) {
}

Vec3f TransformCameraAnchor::position() {
    auto modelMatrix = transform_->get()->getVertex(0);
    return modelMatrix.r.position() + (modelMatrix.r ^ Vec4f(offset_,0.0)).xyz_() / modelMatrix.r.scaling();
}

Vec3f TransformCameraAnchor::direction() {
    auto modelMatrix = transform_->get()->getVertex(0);
    auto diff = (modelMatrix.r.position() - position());
    diff.normalize();
    return diff;
}
