#include "indexed-shape.h"

using namespace regen;

IndexedShape::IndexedShape(
		const ref_ptr <Camera> &camera,
		const ref_ptr <Camera> &sortCamera,
		const Vec4i &lodShift,
		const ref_ptr <BoundingShape> &shape) :
		camera_(camera), sortCamera_(sortCamera), shape_(shape) {
	numLODs_ = 1;
	if (shape->mesh().get()) {
		numLODs_ = std::max(numLODs_, shape->mesh()->numLODs());
	}
	for (const auto &part : shape->parts()) {
		numLODs_ = std::max(numLODs_, part->numLODs());
	}
	const uint32_t L = camera_->numLayer();
	tmp_layerVisibility_.resize(L, false);
	visible_.resize(L, true);
	// remember LOD thresholds
	const ref_ptr<Mesh> &mesh = shape->baseMesh();
	const Vec3f &lodThresholds = mesh->lodThresholds();
	lodThresholds_ = lodThresholds;
	// move the thresholds around based on LOD shift
	for (int32_t i = 0; i < 3; ++i) {
		int shift = lodShift[i];
		if (shift > 0) {
			for (int32_t j = 0; j < shift && (i+j) < 3; ++j) {
				lodThresholds_[i+j] = (i==0 ? 0.0f : lodThresholds[i-1]);
			}
		}
		else if (shift < 0) {
			for (int32_t j = -shift; j>0 && (i-j) > 0; --j) {
				lodThresholds_[i-j] = (i==2 ? std::numeric_limits<float>::max() : lodThresholds[i+1]);
			}
		}
	}
}

ClientData_rw<uint32_t> IndexedShape::mapInstanceIDs(int mapMode) {
	return idVec_->mapClientData<uint32_t>(mapMode);
}

ClientData_rw<uint32_t> IndexedShape::mapInstanceCounts(int mapMode) {
	return countVec_->mapClientData<uint32_t>(mapMode);
}

ClientData_rw<uint32_t> IndexedShape::mapBaseInstances(int mapMode) {
	return baseVec_->mapClientData<uint32_t>(mapMode);
}

IndexedShape::MappedData::MappedData(
			const ref_ptr <ShaderInput> &idVec,
			const ref_ptr <ShaderInput> &countsVec,
			const ref_ptr <ShaderInput> &baseVec) :
		ids(idVec->mapClientData<uint32_t>(BUFFER_GPU_WRITE)),
		count(countsVec->mapClientData<uint32_t>(BUFFER_GPU_WRITE)),
		base(baseVec->mapClientData<uint32_t>(BUFFER_GPU_WRITE)) {
}

IndexedShape::MappedData::~MappedData() {
	base.unmap();
	count.unmap();
	ids.unmap();
}

void IndexedShape::mapInstanceData_internal() {
	mappedInstanceIDs_.emplace(idVec_, countVec_, baseVec_);
}

void IndexedShape::unmapInstanceData_internal() {
	mappedInstanceIDs_.reset();
}

uint32_t *IndexedShape::mappedInstanceIDs() {
	return mappedInstanceIDs_->ids.w.data();
}

uint32_t *IndexedShape::mappedInstanceCounts() {
	return mappedInstanceIDs_->count.w.data();
}

uint32_t *IndexedShape::mappedBaseInstance() {
	return mappedInstanceIDs_->base.w.data();
}
