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

ref_ptr<SSBO>& RadixSort::sortedIndexBuffer() {
	return valueBuffer_[outputIdx_];
}

ref_ptr<SSBO>& RadixSort::inputIndexBuffer() {
	return valueBuffer_[inputIdx_];
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
	uint32_t numWorkGroups = radixHistogramPass_->computeState()->numWorkGroups().x;
	REGEN_INFO("GPU radix sort with " << numKeys_ << " keys, "
			<< numWorkGroups << " work groups, radix bits: " << radixBits_
			<< ", sort group size: " << sortGroupSize_
			<< ", scan group size: " << scanGroupSize_);

	// Temporary Buffers for sorting.
	keyBuffer_ = ref_ptr<SSBO>::alloc("KeyBuffer",
			BUFFER_USAGE_STREAM_COPY, SSBO::RESTRICT);
	auto keys = ref_ptr<ShaderInput1ui>::alloc("keys", numKeys_);
	keys->set_forceArray(true);
	keyBuffer_->addBlockInput(keys);
	keyBuffer_->update();

	if (userValueBuffer_.get()) {
		valueBuffer_[0] = userValueBuffer_;
	} else {
		valueBuffer_[0] = ref_ptr<SSBO>::alloc("ValueBuffer1",
				BUFFER_USAGE_STREAM_COPY, SSBO::RESTRICT);
		auto values1 = ref_ptr<ShaderInput1ui>::alloc("values", numKeys_);
		values1->set_forceArray(true);
		valueBuffer_[0]->addBlockInput(values1);
		valueBuffer_[0]->update();
	}
	valueBuffer_[1] = ref_ptr<SSBO>::alloc("ValueBuffer2",
			BUFFER_USAGE_STREAM_COPY, SSBO::RESTRICT);
	auto values2 = ref_ptr<ShaderInput1ui>::alloc("values", numKeys_);
	values2->set_forceArray(true);
	valueBuffer_[1]->addBlockInput(values2);
	valueBuffer_[1]->update();

	globalHistogramBuffer_ = ref_ptr<SSBO>::alloc("HistogramBuffer",
			BUFFER_USAGE_STREAM_COPY, SSBO::RESTRICT);
	globalHistogramBuffer_->addBlockInput(ref_ptr<ShaderInput1ui>::alloc(
			"globalHistogram", numBuckets_ * numWorkGroups));
	globalHistogramBuffer_->update();

	{ // radix histogram
		radixHistogramPass_->joinShaderInput(globalHistogramBuffer_);
		radixHistogramPass_->joinShaderInput(keyBuffer_);
		StateConfigurer shaderCfg;
		shaderCfg.define("NUM_RADIX_BUCKETS", REGEN_STRING(numBuckets_));
		shaderCfg.define("ONE_LESS_NUM_RADIX_BUCKETS", REGEN_STRING(numBuckets_ - 1));
		shaderCfg.addState(radixHistogramPass_.get());
		radixHistogramPass_->createShader(shaderCfg.cfg());
		// retrieve locations for quick state switching in radix passes
		histogramReadIndex_ = radixHistogramPass_->shaderState()->shader()->uniformLocation("ValueBuffer");
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
		StateConfigurer shaderCfg;
		shaderCfg.define("NUM_RADIX_BUCKETS", REGEN_STRING(numBuckets_));
		shaderCfg.define("ONE_LESS_NUM_RADIX_BUCKETS", REGEN_STRING(numBuckets_ - 1));
		shaderCfg.addState(radixScatterPass_.get());
		radixScatterPass_->createShader(shaderCfg.cfg());
		// retrieve locations for quick state switching in radix passes
		scatterReadIndex_ = radixScatterPass_->shaderState()->shader()->uniformLocation("ReadBuffer");
		scatterWriteIndex_ = radixScatterPass_->shaderState()->shader()->uniformLocation("WriteBuffer");
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
	uint32_t readIndex = 0u;
	uint32_t writeIndex = 1u;
	for (uint32_t bitOffset = 0u; bitOffset < 32u; bitOffset += radixBits_) {
		// Run histogram pass. As a result we will have global counts for each bucket
		// and work group in the globalHistogramBuffer_.
		radixHistogramPass_->enable(rs);
		glUniform1ui(histogramBitOffsetIndex_, bitOffset);
		valueBuffer_[readIndex]->bind(histogramReadIndex_);
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
		valueBuffer_[readIndex]->bind(scatterReadIndex_);
		valueBuffer_[writeIndex]->bind(scatterWriteIndex_);
		radixScatterPass_->disable(rs);

		// swap read and write buffers
		std::swap(readIndex, writeIndex);
	}
	GL_ERROR_LOG();

#ifdef RADIX_DEBUG_RESULT
	printInstanceMap(rs);
#elifdef RADIX_DEBUG_CORRECTNESS
	printInstanceMap(rs);
#endif
}

void RadixSort::printHistogram(RenderState *rs) {
	// debug histogram
	auto numWorkGroups = radixHistogramPass_->computeState()->numWorkGroups().x;
	rs->shaderStorageBuffer().apply(globalHistogramBuffer_->blockReference()->bufferID());
	auto histogramData = (uint32_t *) glMapBufferRange(
			GL_SHADER_STORAGE_BUFFER,
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
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}
}

void RadixSort::printInstanceMap(RenderState *rs) {
	// debug sorted output
	std::vector<double> distances(numKeys_);
	rs->shaderStorageBuffer().apply(keyBuffer_->blockReference()->bufferID());
	auto sortKeys = (uint32_t *) glMapBufferRange(
			GL_SHADER_STORAGE_BUFFER,
			keyBuffer_->blockReference()->address(),
			keyBuffer_->blockReference()->allocatedSize(),
			GL_MAP_READ_BIT);
	if (sortKeys) {
		for (uint32_t i = 0; i < numKeys_; ++i) {
			distances[i] = conversion::uintToFloat(sortKeys[i]);
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}

	auto idRef = valueBuffer_[outputIdx_]->blockReference();
	rs->shaderStorageBuffer().apply(idRef->bufferID());
	auto instanceIDs = (uint32_t *) glMapBufferRange(
			GL_SHADER_STORAGE_BUFFER,
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
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}
}
