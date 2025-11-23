#include "state-node-comparator.h"

using namespace regen;

NodeEyeDepthComparator::NodeEyeDepthComparator(
		const ref_ptr<Camera> &cam, bool frontToBack)
		: cam_(cam),
		  mode_(((int) frontToBack) * 2 - 1) {
}

float NodeEyeDepthComparator::getEyeDepth(const Vec3f &p) const {
	auto &mat = cam_->view()[0];
	return mat.x[2] * p.x + mat.x[6] * p.y + mat.x[10] * p.z + mat.x[14];
}

ModelTransformation *NodeEyeDepthComparator::findModelTransformation(StateNode *n) const {
	State *nodeState = n->state().get();
	auto *ret = dynamic_cast<ModelTransformation *>(nodeState);
	if (ret != nullptr) { return ret; }

	for (const auto &it: *nodeState->joined().get()) {
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
	auto *tf0 = getModelTransformation(n0.get());
	auto *tf1 = getModelTransformation(n1.get());
	if (tf0 != nullptr && tf1 != nullptr) {
		auto diff = mode_ * (
				getEyeDepth(tf0->modelMat()->getVertex(0).r.position()) -
				getEyeDepth(tf1->modelMat()->getVertex(0).r.position()));
		return diff < 0;
	} else if (tf0 != nullptr) {
		return true;
	} else if (tf1 != nullptr) {
		return false;
	} else {
		return n0 < n1;
	}
}
