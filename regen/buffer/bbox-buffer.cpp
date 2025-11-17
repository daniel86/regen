#include <cfloat>
#include "bbox-buffer.h"
#include "regen/utility/conversion.h"

using namespace regen;

struct BoundingBoxBlock {
	Vec4i min;
	Vec4i max;
};

namespace regen {
	static inline int biasedBits(float f) {
		int i = conversion::floatBitsToInt(f);
		return i ^ ((i >> 31) & 0x7FFFFFFF);
	}

	static inline float biasedToFloat(int i) {
		return conversion::intBitsToFloat(i ^ ((i >> 31) & 0x7FFFFFFF));
	}
}

BBoxBuffer::BBoxBuffer(
		const Bounds<Vec3f> &initialBounds,
		const std::string &name) :
	SSBO(name, BufferUpdateFlags::FULL_PER_FRAME),
	bbox_(Bounds<Vec3f>::create(initialBounds.min, initialBounds.max))
{
	// The parameters of our bounding box buffer or the boundaries encoded as integers.
	// Integers are used for atomic operations in the compute shader.
	addStagedInput(ref_ptr<ShaderInput4i>::alloc("bboxMin"));
	addStagedInput(ref_ptr<ShaderInput4i>::alloc("bboxMax"));

	// configure storage access:
	// - use persistent coherent mapping for the staging buffer, i.e. keep read buffer mapped
	setStagingMapMode(BUFFER_MAP_PERSISTENT_COHERENT);
	// - allow CPU read access on the staging buffer
	setStagingAccessMode(BUFFER_CPU_READ);
	// - use double-buffering for the staging buffer, i.e. use a ring buffer with 2 segments.
	//   so we will always be one frame behind the GPU.
	setBufferingMode(DOUBLE_BUFFER);

	// Adopt a static storage for clearing the draw buffer from which we read the bounding box.
	// This might be non-mappable storage, so to clear the buffer we will use dedicated static write buffer.
	static const BoundingBoxBlock zeroBlock = {
		Vec4i(biasedBits(FLT_MAX)),
		Vec4i(biasedBits(-FLT_MAX))
	};
	clearRef_ = BufferObject::adoptBufferRange(
		sizeof(BoundingBoxBlock),
		bufferPool(flags_.target, BUFFER_MODE_STATIC_WRITE));
	glNamedBufferSubData(
		clearRef_->bufferID(),
		clearRef_->address(),
		clearRef_->allocatedSize(),
		&zeroBlock);

	// create the draw buffer reference
	update();
	// upload the initial bounding box values to avoid having bad values in the buffer
	// for the first few frames.
	const BoundingBoxBlock initialValues = {
		Vec4i(biasedBits(initialBounds.min.x),
			  biasedBits(initialBounds.min.y),
			  biasedBits(initialBounds.min.z),
			  0), // w is unused, so set to 0
		Vec4i(biasedBits(initialBounds.max.x),
			  biasedBits(initialBounds.max.y),
			  biasedBits(initialBounds.max.z),
			  0) // w is unused, so set to 0
	};
	setBufferData(&initialValues);
	GL_ERROR_LOG();
}

bool BBoxBuffer::updateBoundingBox() {
	bool hasChanged = false;
	if (!shared_->isGloballyStaged_) {
		if (!shared_->stagingBuffer_->readBuffer(
				drawBufferRef_,
				*drawBufferRange_.get(),
				0u)) {
			REGEN_ERROR("Unable to read bounding box buffer data.");
			isBufferValid_ = false;
			return false;
		}
	}
	if (shared_->stagingBuffer_->hasReadData()) {
		auto &bbox = *((BoundingBoxBlock*)shared_->stagingBuffer_->readData());
        bboxMin_.x = biasedToFloat(bbox.min.x);
        bboxMin_.y = biasedToFloat(bbox.min.y);
        bboxMin_.z = biasedToFloat(bbox.min.z);
        bboxMax_.x = biasedToFloat(bbox.max.x);
        bboxMax_.y = biasedToFloat(bbox.max.y);
        bboxMax_.z = biasedToFloat(bbox.max.z);
        auto d =
        	(bboxMin_ - bbox_.min).lengthSquared() +
        	(bboxMax_ - bbox_.max).lengthSquared();
		if (d > 0.001f) {
			hasChanged = true;
			bbox_.min = bboxMin_;
			bbox_.max = bboxMax_;
		}
	}
    return hasChanged;
}

void BBoxBuffer::clear() {
	// clear the draw buffer, staging is just used for reading.
	glCopyNamedBufferSubData(
		clearRef_->bufferID(),
		drawBufferRef_->bufferID(),
		clearRef_->address(),
		drawBufferRef_->address(),
		clearRef_->allocatedSize());
}
