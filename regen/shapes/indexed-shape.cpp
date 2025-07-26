#include "indexed-shape.h"

using namespace regen;

IndexedShape::IndexedShape(
		const ref_ptr <Camera> &camera,
		const ref_ptr <BoundingShape> &shape) :
		camera_(camera), shape_(shape) {
}

bool IndexedShape::isVisible() const {
	return visible_;
}

bool IndexedShape::hasVisibleInstances() const {
	return instanceCount_ > 0;
}

ClientData_rw<unsigned int> IndexedShape::mapInstanceIDs(int mapMode) {
	return visibleVec_->mapClientData<unsigned int>(mapMode);
}

IndexedShape::MappedData::MappedData(const ref_ptr <ShaderInput1ui> &visibleVec) :
		mapped(visibleVec->mapClientData<unsigned int>(BUFFER_GPU_WRITE)) {
}

void IndexedShape::mapInstanceIDs_internal() {
	mappedInstanceIDs_.emplace(visibleVec_);
}

void IndexedShape::unmapInstanceIDs_internal() {
	mappedInstanceIDs_.reset();
}

unsigned int *IndexedShape::mappedInstanceIDs() {
	return mappedInstanceIDs_->mapped.w.data();
}
