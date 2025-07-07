#include <cfloat>
#include "bbox-buffer.h"
#include "regen/utility/conversion.h"

using namespace regen;

BBoxBuffer::BBoxBuffer(const std::string &name) :
	SSBO(name, BUFFER_HINT_UPDATE_STREAM),
	bbox_(Vec3f::zero(), Vec3f::zero())
{
	setBufferMapMode(BUFFER_MAP_PERSISTENT_COHERENT);
	setBufferAccessMode(BUFFER_CPU_READ);
	setBufferingMode(DOUBLE_BUFFER);
	addBlockInput(ref_ptr<ShaderInput4i>::alloc("bboxMin"));
	addBlockInput(ref_ptr<ShaderInput4i>::alloc("bboxMax"));
	update();
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

struct BoundingBoxBlock {
	Vec4i min;
	Vec4i max;
};

bool BBoxBuffer::updateBoundingBox() {
	bool hasChanged = false;
	bufferMapping_->readBuffer(*bufferDrawRange_.get());
	if (bufferMapping_->hasReadData()) {
		auto &bbox = *((BoundingBoxBlock*)bufferMapping_->clientData());
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
	// FIXME: cannot write to the buffer directly, as it is mapped persistently with read only access.
	//        instead use a shader.
	static const BoundingBoxBlock zeroBlock = {
		Vec4i(biasedBits(FLT_MAX)),
		Vec4i(biasedBits(-FLT_MAX))
	};
	setBufferData(&zeroBlock);
}
