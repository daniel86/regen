#include "radix-sort.h"
#include "regen/gl-types/gl-param.h"
#include "regen/utility/conversion.h"
#include "prefix-scan.h"

//#define RADIX_DEBUG_HISTOGRAM
//#define RADIX_DEBUG_RESULT
//#define RADIX_DEBUG_CORRECTNESS

using namespace regen;

RadixSort::RadixSort(uint32_t numKeys)
		: State(),
		  numKeys_(numKeys) {
}

void RadixSort::setOutputBuffer(const ref_ptr<SSBO> &values) {
	userValueBuffer_ = values;
}

void RadixSort::setRadixBits(uint32_t bits) {
	if (bits > 0u && bits <= 32u) {
		radixBits_ = bits;
		numBuckets_ = 1u << bits;
	}
	else {
		REGEN_WARN("RadixSort: radix bits must be in range [1, 32].");
	}
}

void RadixSort::setSortGroupSize(uint32_t size) {
	if (size > 0u && math::isPowerOfTwo(size)) {
		sortGroupSize_ = size;
		if (radixHistogramPass_.get()) {
			REGEN_WARN("Sort group size must be set before creating the resources.");
		}
	} else {
		REGEN_WARN("Sort group size must be a power of two.");
	}
}

void RadixSort::setScanGroupSize(uint32_t size) {
	if (size > 0u && math::isPowerOfTwo(size)) {
		scanGroupSize_ = size;
		if (prefixScan_.get()) {
			REGEN_WARN("Scan group size must be set before creating the resources.");
		}
	} else {
		REGEN_WARN("Scan group size must be a power of two.");
	}
}

ref_ptr<SSBO>& RadixSort::valueBuffer() {
	return valueBuffer_;
}

void RadixSort::createResources() {
	{
		radixHistogramPass_ = ref_ptr<ComputePass>::alloc("regen.compute.sort.radix.histogram");
		radixHistogramPass_->computeState()->shaderDefine("NUM_SORT_KEYS", REGEN_STRING(numKeys_));
		radixHistogramPass_->computeState()->setNumWorkUnits(static_cast<int>(numKeys_), 1, 1);
		radixHistogramPass_->computeState()->setGroupSize(sortGroupSize_, 1, 1);

		radixScatterPass_ = ref_ptr<ComputePass>::alloc("regen.compute.sort.radix.scatter");
		radixScatterPass_->computeState()->shaderDefine("NUM_SORT_KEYS", REGEN_STRING(numKeys_));
		radixScatterPass_->computeState()->setNumWorkUnits(static_cast<int>(numKeys_), 1, 1);
		radixScatterPass_->computeState()->setGroupSize(sortGroupSize_, 1, 1);
	}
	const uint32_t numWorkGroups = radixHistogramPass_->computeState()->numWorkGroups().x;
	REGEN_INFO("GPU radix sort with " << numKeys_ << " keys, "
			<< numWorkGroups << " work groups, radix bits: " << radixBits_
			<< ", sort group size: " << sortGroupSize_
			<< ", scan group size: " << scanGroupSize_);

	// Temporary Buffers for sorting.
	keyBuffer_ = ref_ptr<SSBO>::alloc("KeyBuffer",
			BufferUpdateFlags{ BUFFER_UPDATE_PER_FRAME, BUFFER_UPDATE_FULLY },
			SSBO::RESTRICT);
	auto keys = ref_ptr<ShaderInput1ui>::alloc("keys", numKeys_);
	keys->set_forceArray(true);
	keyBuffer_->addBlockInput(keys);
	keyBuffer_->update();

	if (userValueBuffer_.get()) {
		valueBuffer_ = userValueBuffer_;
	} else {
		valueBuffer_ = ref_ptr<SSBO>::alloc("ValueBuffer",
				BufferUpdateFlags{ BUFFER_UPDATE_PER_FRAME, BUFFER_UPDATE_FULLY },
				SSBO::RESTRICT);
		auto values1 = ref_ptr<ShaderInput1ui>::alloc("values", numKeys_ * 2);
		values1->set_forceArray(true);
		valueBuffer_->addBlockInput(values1);
		valueBuffer_->setStagingAccessMode(BUFFER_GPU_ONLY);
		valueBuffer_->setStagingMapMode(BUFFER_MAP_DISABLED);
		valueBuffer_->update();
	}

	globalHistogramBuffer_ = ref_ptr<SSBO>::alloc("HistogramBuffer",
			BufferUpdateFlags{ BUFFER_UPDATE_PER_FRAME, BUFFER_UPDATE_FULLY },
			SSBO::RESTRICT);
	globalHistogramBuffer_->addBlockInput(ref_ptr<ShaderInput1ui>::alloc(
			"globalHistogram", numBuckets_ * numWorkGroups));
	globalHistogramBuffer_->update();

	{ // radix histogram
		radixHistogramPass_->joinShaderInput(globalHistogramBuffer_);
		radixHistogramPass_->joinShaderInput(keyBuffer_);
		radixHistogramPass_->joinShaderInput(valueBuffer_);
		StateConfigurer shaderCfg;
		shaderCfg.define("NUM_RADIX_BUCKETS", REGEN_STRING(numBuckets_));
		shaderCfg.define("ONE_LESS_NUM_RADIX_BUCKETS", REGEN_STRING(numBuckets_ - 1));
		shaderCfg.addState(radixHistogramPass_.get());
		radixHistogramPass_->createShader(shaderCfg.cfg());
		// retrieve locations for quick state switching in radix passes
		histogramReadIndex_ = radixHistogramPass_->shaderState()->shader()->uniformLocation("readOffset");
		histogramBitOffsetIndex_ = radixHistogramPass_->shaderState()->shader()->uniformLocation("radixBitOffset");
	}
	{
		auto prefixScan = ref_ptr<PrefixScan>::alloc(globalHistogramBuffer_);
		prefixScan->setScanGroupSize(scanGroupSize_);
		// assuming the number of elements to sort does not change, so does the histogram size
		prefixScan->setHasHistogramConstantSize(true);
		prefixScan->createResources();
		prefixScan_ = prefixScan;
	}

	{ // radix sort
		radixScatterPass_->joinShaderInput(globalHistogramBuffer_);
		radixScatterPass_->joinShaderInput(keyBuffer_);
		radixScatterPass_->joinShaderInput(valueBuffer_);
		StateConfigurer shaderCfg;
		shaderCfg.define("NUM_RADIX_BUCKETS", REGEN_STRING(numBuckets_));
		shaderCfg.define("ONE_LESS_NUM_RADIX_BUCKETS", REGEN_STRING(numBuckets_ - 1));
		shaderCfg.addState(radixScatterPass_.get());
		radixScatterPass_->createShader(shaderCfg.cfg());
		// retrieve locations for quick state switching in radix passes
		scatterReadIndex_ = radixScatterPass_->shaderState()->shader()->uniformLocation("readOffset");
		scatterWriteIndex_ = radixScatterPass_->shaderState()->shader()->uniformLocation("writeOffset");
		scatterBitOffsetIndex_ = radixScatterPass_->shaderState()->shader()->uniformLocation("radixBitOffset");
	}
}

void RadixSort::enable(RenderState *rs) {
	State::enable(rs);
	sort(rs);
}

void RadixSort::sort(RenderState *rs) {
	// now we can make the radix passes starting with values_[0] as input
	// and writing to values_[1]. Then we swap the buffers each pass.
	// In the end, we will have the sorted instanceIDs in values_[0].
	uint32_t readOffset = 0u;
	uint32_t writeOffset = numKeys_;
	for (uint32_t bitOffset = 0u; bitOffset < 32u; bitOffset += radixBits_) {
		// Run histogram pass. As a result we will have global counts for each bucket
		// and work group in the globalHistogramBuffer_.
		radixHistogramPass_->enable(rs);
		glUniform1ui(histogramBitOffsetIndex_, bitOffset);
		glUniform1ui(histogramReadIndex_, readOffset);
		radixHistogramPass_->disable(rs);
#ifdef RADIX_DEBUG_HISTOGRAM
		printHistogram(rs);
#endif

		// Run offsets pass. As a result we will have the offsets for each work group
		// in the globalHistogramBuffer_.
		prefixScan_->enable(rs);
		prefixScan_->disable(rs);
#ifdef RADIX_DEBUG_HISTOGRAM
		printHistogram(rs);
#endif

		// Finally, run the scatter pass. As a result we will have the sorted instanceIDs
		// in the values_[writeIndex] buffer.
		radixScatterPass_->enable(rs);
		glUniform1ui(scatterBitOffsetIndex_, bitOffset);
		glUniform1ui(scatterReadIndex_, readOffset);
		glUniform1ui(scatterWriteIndex_, writeOffset);
		radixScatterPass_->disable(rs);

		// swap read and write buffers
		std::swap(readOffset, writeOffset);
	}

#ifdef RADIX_DEBUG_RESULT
	printInstanceMap(rs);
#elifdef RADIX_DEBUG_CORRECTNESS
	printInstanceMap(rs);
#endif
}

void RadixSort::printHistogram(RenderState *rs) {
	// debug histogram
	auto numWorkGroups = radixHistogramPass_->computeState()->numWorkGroups().x;
	auto histogramData = (uint32_t *) glMapNamedBufferRange(
			globalHistogramBuffer_->blockReference()->bufferID(),
			globalHistogramBuffer_->blockReference()->address(),
			globalHistogramBuffer_->blockReference()->allocatedSize(),
			GL_MAP_READ_BIT);
	if (histogramData) {
		std::stringstream sss;
		sss << "    histogram: | ";
		for (uint32_t i = 0; i < numBuckets_ * numWorkGroups; ++i) {
			sss << histogramData[i] << " ";
			if ((i + 1) % numBuckets_ == 0) {
				sss << " | ";
			}
		}
		REGEN_INFO(" " << sss.str());
		glUnmapNamedBuffer(globalHistogramBuffer_->blockReference()->bufferID());
	}
}

void RadixSort::printInstanceMap(RenderState *rs) {
	// debug sorted output
	std::vector<double> distances(numKeys_);
	auto sortKeys = (uint32_t *) glMapNamedBufferRange(
			keyBuffer_->blockReference()->bufferID(),
			keyBuffer_->blockReference()->address(),
			keyBuffer_->blockReference()->allocatedSize(),
			GL_MAP_READ_BIT);
	if (sortKeys) {
		for (uint32_t i = 0; i < numKeys_; ++i) {
			distances[i] = conversion::uintToFloat(sortKeys[i]);
		}
		glUnmapNamedBuffer(keyBuffer_->blockReference()->bufferID());
	}

	auto idRef = valueBuffer_->blockReference();
	auto instanceIDs = (uint32_t *) glMapNamedBufferRange(
			idRef->bufferID(),
			idRef->address(),
			idRef->allocatedSize(),
			GL_MAP_READ_BIT);
	if (instanceIDs) {
		double lastDistance = 0.0;
		bool validSortedIDs_ = true;
		for (uint32_t i = 0; i < numKeys_; ++i) {
			auto mappedID = instanceIDs[i];
			if (mappedID >= numKeys_) {
				REGEN_ERROR("mappedID " << mappedID << " >= numInstances_ " << numKeys_);
				validSortedIDs_ = false;
				break;
			}
			auto distance = distances[mappedID];
			if (distance < lastDistance) {
				validSortedIDs_ = false;
			}
			lastDistance = distance;
		}
		if (validSortedIDs_) {
			REGEN_INFO("   sortedIDs are valid");
		} else {
			REGEN_INFO("   sortedIDs are INVALID");
		}
#ifdef RADIX_DEBUG_RESULT
		{
			std::stringstream sss;
			sss << "    ID data (" << numInstances_ << "): ";
			for (uint32_t i = 0; i < numInstances_; ++i) {
				if (instanceIDs[i] >= numInstances_) {
					break;
				}
				sss << instanceIDs[i] << " (" << distances[instanceIDs[i]] << ") ";
			}
			REGEN_INFO(" " << sss.str());
		}
#endif
		glUnmapNamedBuffer(idRef->bufferID());
	}
}
