#include <cfloat>
#include "bbox-buffer.h"
#include "regen/utility/conversion.h"

using namespace regen;

BBoxBuffer::BBoxBuffer(const std::string &name) :
	SSBO(name, BUFFER_USAGE_STREAM_COPY),
	bbox_(Vec3f::zero(), Vec3f::zero())
{
	addBlockInput(createUniform<ShaderInput4i,Vec4i>("bboxMin", Vec4i(0)));
	addBlockInput(createUniform<ShaderInput4i,Vec4i>("bboxMax", Vec4i(0)));
	update();
	bboxMapping_ = ref_ptr<BufferStructMapping<BoundingBoxBlock>>::alloc(
			MAP_READ | MAP_PERSISTENT | MAP_COHERENT,
			DOUBLE_BUFFER);
}

namespace regen {
	static inline int biasedBits(float f) {
		int i = conversion::floatBitsToInt(f);
		return i ^ ((i >> 31) & 0x7FFFFFFF);
	}

	static inline float biasedToFloat(int i) {
		return conversion::intBitsToFloat(i ^ ((i >> 31) & 0x7FFFFFFF));
	}
}

bool BBoxBuffer::updateBoundingBox() {
	bool hasChanged = false;
	bboxMapping_->readBuffer(blockReference(), GL_SHADER_STORAGE_BUFFER);
	if (bboxMapping_->hasReadData()) {
		auto &bbox = bboxMapping_->storageValue();
        bboxMin_.x = biasedToFloat(bbox.min.x);
        bboxMin_.y = biasedToFloat(bbox.min.y);
        bboxMin_.z = biasedToFloat(bbox.min.z);
        bboxMax_.x = biasedToFloat(bbox.max.x);
        bboxMax_.y = biasedToFloat(bbox.max.y);
        bboxMax_.z = biasedToFloat(bbox.max.z);
        auto d =
        	(bboxMin_ - bbox_.min).length() +
        	(bboxMax_ - bbox_.max).length();
		if (d > 0.01f) {
			hasChanged = true;
			bbox_.min = bboxMin_;
			bbox_.max = bboxMax_;
		}
	}
    return hasChanged;
}

void BBoxBuffer::clear() {
	// clear the bounding box buffer to zero
	static const BoundingBoxBlock zeroBlock = {
		Vec4i(biasedBits(FLT_MAX)),
		Vec4i(biasedBits(-FLT_MAX))
	};
	glNamedBufferSubData(
		blockReference()->bufferID(),
		blockReference()->address(),
		blockReference()->allocatedSize(),
		&zeroBlock);
}
