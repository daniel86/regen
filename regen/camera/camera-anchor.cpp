#include "camera-anchor.h"

using namespace regen;

TransformCameraAnchor::TransformCameraAnchor(const ref_ptr<ModelTransformation> &transform)
		: transform_(transform), mode_(LOOK_AT_FRONT) {
}

Vec3f TransformCameraAnchor::position() {
    const auto &modelMatrix = transform_->get()->getVertex(0);
    return modelMatrix.position() + offset_;
}

Vec3f TransformCameraAnchor::direction() {
    const auto &modelMatrix = transform_->get()->getVertex(0);
    auto diff = (modelMatrix.position() - position());
    diff.normalize();
    return diff;
}
