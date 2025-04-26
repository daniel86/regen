#include <cfloat>
#include "bbox-buffer.h"
#include "regen/utility/conversion.h"

using namespace regen;

BBoxBuffer::BBoxBuffer(const std::string &name) :
	SSBO(name, BUFFER_USAGE_STREAM_COPY),
	bbox_(Vec3f::zero(), Vec3f::zero())
{
	addBlockInput(createUniform<ShaderInput4i,Vec4i>("bboxPositiveMin", Vec4i(0)));
	addBlockInput(createUniform<ShaderInput4i,Vec4i>("bboxPositiveMax", Vec4i(0)));
	addBlockInput(createUniform<ShaderInput4i,Vec4i>("bboxNegativeMin", Vec4i(0)));
	addBlockInput(createUniform<ShaderInput4i,Vec4i>("bboxNegativeMax", Vec4i(0)));
	addBlockInput(createUniform<ShaderInput4i,Vec4i>("bboxPositiveFlags", Vec4i(0)));
	addBlockInput(createUniform<ShaderInput4i,Vec4i>("bboxNegativeFlags", Vec4i(0)));
	update();
	bboxMapping_ = ref_ptr<BufferStructMapping<BoundingBoxBlock>>::alloc(
			BufferMapping::READ | BufferMapping::PERSISTENT | BufferMapping::COHERENT,
			BufferMapping::DOUBLE_BUFFER);
}

void BBoxBuffer::clear() {
	// clear the bounding box buffer to zero
	static const BoundingBoxBlock zeroBlock = {
		Vec4i(conversion::floatBitsToInt(FLT_MAX)),
		Vec4i(conversion::floatBitsToInt(0.0f)),
		Vec4i(conversion::floatBitsToInt(FLT_MAX)),
		Vec4i(conversion::floatBitsToInt(0.0f)),
		Vec4i::zero(),
		Vec4i::zero()};
	RenderState::get()->shaderStorageBuffer().apply(blockReference()->bufferID());
	glClearBufferSubData(GL_SHADER_STORAGE_BUFFER,
		GL_RGBA32I,
		blockReference()->address(),
		blockReference()->allocatedSize(),
		GL_RGBA_INTEGER,
		GL_INT,
		&zeroBlock);
}

namespace regen {
	static inline float getBBoxMax(int neg, int pos, int hasPos) {
		return (hasPos == 1) ? conversion::intBitsToFloat(pos) : -conversion::intBitsToFloat(neg);
	}

	static inline float getBBoxMin(int neg, int pos, int hasNeg) {
		return (hasNeg == 1) ? -conversion::intBitsToFloat(neg) : conversion::intBitsToFloat(pos);
	}
}

bool BBoxBuffer::updateBoundingBox() {
	bool hasChanged = false;
	bboxMapping_->updateMapping(blockReference(), GL_SHADER_STORAGE_BUFFER);
	if (bboxMapping_->hasData()) {
		auto &bbox = bboxMapping_->storageValue();
        bboxMax_.x = getBBoxMax(bbox.negativeMin.x, bbox.positiveMax.x, bbox.positiveFlags.x);
        bboxMin_.x = getBBoxMin(bbox.negativeMax.x, bbox.positiveMin.x, bbox.negativeFlags.x);
        bboxMax_.y = getBBoxMax(bbox.negativeMin.y, bbox.positiveMax.y, bbox.positiveFlags.y);
        bboxMin_.y = getBBoxMin(bbox.negativeMax.y, bbox.positiveMin.y, bbox.negativeFlags.y);
        bboxMax_.z = getBBoxMax(bbox.negativeMin.z, bbox.positiveMax.z, bbox.positiveFlags.z);
        bboxMin_.z = getBBoxMin(bbox.negativeMax.z, bbox.positiveMin.z, bbox.negativeFlags.z);
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
