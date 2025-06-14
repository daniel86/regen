#include "state-node-comparator.h"

using namespace regen;

NodeEyeDepthComparator::NodeEyeDepthComparator(
		const ref_ptr<Camera> &cam, GLboolean frontToBack)
		: cam_(cam),
		  mode_(((GLint) frontToBack) * 2 - 1) {
}

GLfloat NodeEyeDepthComparator::getEyeDepth(const Vec3f &p) const {
	auto mat = cam_->view()->getVertex(0);
	return mat.r.x[2] * p.x + mat.r.x[6] * p.y + mat.r.x[10] * p.z + mat.r.x[14];
}

ModelTransformation *NodeEyeDepthComparator::findModelTransformation(StateNode *n) const {
	State *nodeState = n->state().get();
	auto *ret = dynamic_cast<ModelTransformation *>(nodeState);
	if (ret != nullptr) { return ret; }

	for (const auto &it: nodeState->joined()) {
		ret = dynamic_cast<ModelTransformation *>(it.get());
		if (ret != nullptr) { return ret; }
	}

	for (auto &it: n->childs()) {
		ret = findModelTransformation(it.get());
		if (ret != nullptr) { return ret; }
	}

	return nullptr;
}

ModelTransformation *NodeEyeDepthComparator::getModelTransformation(StateNode *n) const {
	auto it = modelTransformations_.find(n);
	if (it != modelTransformations_.end()) {
		return it->second;
	} else {
		ModelTransformation *modelMat = findModelTransformation(n);
		modelTransformations_[n] = modelMat;
		return modelMat;
	}
}

bool NodeEyeDepthComparator::operator()(ref_ptr<StateNode> &n0, ref_ptr<StateNode> &n1) const {
	auto *modelMat0 = getModelTransformation(n0.get());
	auto *modelMat1 = getModelTransformation(n1.get());
	if (modelMat0 != nullptr && modelMat1 != nullptr) {
		auto diff = mode_ * (
				getEyeDepth(modelMat0->modelMat()->getVertex(0).r.position()) -
				getEyeDepth(modelMat1->modelMat()->getVertex(0).r.position()));
		return diff < 0;
	} else if (modelMat0 != nullptr) {
		return true;
	} else if (modelMat1 != nullptr) {
		return false;
	} else {
		return n0 < n1;
	}
}
