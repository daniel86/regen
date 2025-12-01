#include "radix-sort.h"
#include "regen/gl-types/gl-param.h"
#include "regen/utility/conversion.h"
#include "prefix-scan.h"

//#define RADIX_DEBUG_HISTOGRAM
//#define RADIX_DEBUG_RESULT
//#define RADIX_DEBUG_CORRECTNESS

using namespace regen;

RadixSort_GPU::RadixSort_GPU(uint32_t numKeys, uint32_t numLayers)
		: State(),
		  numKeys_(numKeys),
		  numLayers_(numLayers) {
}

void RadixSort_GPU::setOutputBuffer(const ref_ptr<SSBO> &values, bool isDoubleBuffered) {
	userValueBuffer_ = values;
	useSingleValueBuffer_ = isDoubleBuffered;
}

void RadixSort_GPU::setRadixBits(uint32_t bits) {
	if (bits > 0u && bits <= 32u) {
		radixBits_ = bits;
		numBuckets_ = 1u << bits;
	}
	else {
		REGEN_WARN("RadixSort: radix bits must be in range [1, 32].");
	}
}

void RadixSort_GPU::setSortGroupSize(uint32_t size) {
	if (size > 0u && math::isPowerOfTwo(size)) {
		sortGroupSize_ = size;
		if (radixHistogramPass_.get()) {
			REGEN_WARN("Sort group size must be set before creating the resources.");
		}
	} else {
		REGEN_WARN("Sort group size must be a power of two.");
	}
}

void RadixSort_GPU::setScanGroupSize(uint32_t size) {
	if (size > 0u && math::isPowerOfTwo(size)) {
		scanGroupSize_ = size;
		if (prefixScan_.get()) {
			REGEN_WARN("Scan group size must be set before creating the resources.");
		}
	} else {
		REGEN_WARN("Scan group size must be a power of two.");
	}
}

ref_ptr<SSBO>& RadixSort_GPU::valueBuffer() {
	return valueBuffer_[outputIdx_];
}

void RadixSort_GPU::createResources() {
	{
		radixHistogramPass_ = ref_ptr<ComputePass>::alloc("regen.compute.sort.radix.histogram");
		radixHistogramPass_->computeState()->shaderDefine("NUM_SORT_KEYS", REGEN_STRING(numKeys_));
		radixHistogramPass_->computeState()->setNumWorkUnits(
				static_cast<int>(numKeys_),
				static_cast<int>(numLayers_),
				1);
		radixHistogramPass_->computeState()->setGroupSize(sortGroupSize_, 1, 1);
	}
	const uint32_t numKeysTotal = numKeys_ * numLayers_;
	const uint32_t numWorkGroups = radixHistogramPass_->computeState()->numWorkGroups().x;
	REGEN_INFO("GPU radix sort with " << numKeys_ << " keys, "
			<< numWorkGroups << " work groups, radix bits: " << radixBits_
			<< ", sort group size: " << sortGroupSize_
			<< ", scan group size: " << scanGroupSize_);

	// Temporary Buffers for sorting.
	keyBuffer_ = ref_ptr<SSBO>::alloc("KeyBuffer",
			BufferUpdateFlags::FULL_PER_FRAME,
			SSBO::RESTRICT);
	if (useCompaction_) {
		auto numVisibleKeys = ref_ptr<ShaderInput1ui>::alloc("numVisibleKeys", numLayers_);
		numVisibleKeys->set_forceArray(true);
		keyBuffer_->addStagedInput(numVisibleKeys);
	}
	auto keys = ref_ptr<ShaderInput1ui>::alloc("keys", numKeysTotal);
	keys->set_forceArray(true);
	keyBuffer_->addStagedInput(keys);
	keyBuffer_->update();

	if (useSingleValueBuffer_) {
		auto values1 = ref_ptr<ShaderInput1ui>::alloc("values", numKeysTotal * 2);
		values1->set_forceArray(true);
		if (userValueBuffer_.get()) {
			valueBuffer_[0] = ref_ptr<SSBO>::alloc(*userValueBuffer_.get(), "ValueBuffer");
			while (!valueBuffer_[0]->stagedInputs().empty()) {
				valueBuffer_[0]->removeStagedInput(valueBuffer_[0]->stagedInputs().front().name_);
			}
		}
		if (!valueBuffer_[0].get()) {
			valueBuffer_[0] = ref_ptr<SSBO>::alloc("ValueBuffer",
					BufferUpdateFlags::FULL_PER_FRAME,
					SSBO::RESTRICT);
		}
		valueBuffer_[0]->addStagedInput(values1);
		valueBuffer_[0]->setStagingAccessMode(BUFFER_GPU_ONLY);
		valueBuffer_[0]->setStagingMapMode(BUFFER_MAP_DISABLED);
		valueBuffer_[0]->update();
	} else {
		if (userValueBuffer_.get()) {
			valueBuffer_[0] = userValueBuffer_;
		} else {
			valueBuffer_[0] = ref_ptr<SSBO>::alloc("ValueBuffer1",
					BufferUpdateFlags::FULL_PER_FRAME, SSBO::RESTRICT);
			auto values1 = ref_ptr<ShaderInput1ui>::alloc("values", numKeysTotal);
			values1->set_forceArray(true);
			valueBuffer_[0]->addStagedInput(values1);
			valueBuffer_[0]->update();
		}
		valueBuffer_[1] = ref_ptr<SSBO>::alloc("ValueBuffer2",
				BufferUpdateFlags::FULL_PER_FRAME, SSBO::RESTRICT);
		auto values2 = ref_ptr<ShaderInput1ui>::alloc("values", numKeysTotal);
		values2->set_forceArray(true);
		valueBuffer_[1]->addStagedInput(values2);
		valueBuffer_[1]->update();
	}

	const uint32_t histogramSize = numBuckets_ * numWorkGroups;
	globalHistogramBuffer_ = ref_ptr<SSBO>::alloc("HistogramBuffer",
			BufferUpdateFlags::FULL_PER_FRAME,
			SSBO::RESTRICT);
	globalHistogramBuffer_->addStagedInput(ref_ptr<ShaderInput1ui>::alloc(
			"globalHistogram", histogramSize * numLayers_));
	globalHistogramBuffer_->update();

	{ // radix histogram
		radixHistogramPass_->setInput(globalHistogramBuffer_);
		radixHistogramPass_->setInput(keyBuffer_);
		radixHistogramPass_->computeState()->shaderDefine("NUM_LAYERS", REGEN_STRING(numLayers_));
		radixHistogramPass_->computeState()->shaderDefine("HISTOGRAM_SIZE", REGEN_STRING(histogramSize));
		StateConfigurer shaderCfg;
		shaderCfg.define("NUM_RADIX_BUCKETS", REGEN_STRING(numBuckets_));
		shaderCfg.define("ONE_LESS_NUM_RADIX_BUCKETS", REGEN_STRING(numBuckets_ - 1));
		if (useSingleValueBuffer_) {
			radixHistogramPass_->setInput(valueBuffer_[0]);
			shaderCfg.define("RADIX_CONTIGUOUS_VALUE_BUFFERS", "1");
		}
		shaderCfg.addState(radixHistogramPass_.get());
		radixHistogramPass_->createShader(shaderCfg.cfg());
		// retrieve locations for quick state switching in radix passes
		if (useSingleValueBuffer_) {
			histogramReadIndex_ = radixHistogramPass_->shaderState()->shader()->uniformLocation("readOffset");
		} else {
			histogramReadIndex_ = radixHistogramPass_->shaderState()->shader()->uniformLocation("ValueBuffer");
		}
		histogramBitOffsetIndex_ = radixHistogramPass_->shaderState()->shader()->uniformLocation("radixBitOffset");
		if (histogramReadIndex_ == -1) {
			REGEN_WARN("RadixSort: histogram read index not found in shader.");
		}
		if (histogramBitOffsetIndex_ == -1) {
			REGEN_WARN("RadixSort: histogram bit offset index not found in shader.");
		}
	}
	{
		auto prefixScan = ref_ptr<PrefixScan>::alloc(globalHistogramBuffer_, numLayers_);
		prefixScan->setScanGroupSize(scanGroupSize_);
		// assuming the number of elements to sort does not change, so does the histogram size
		prefixScan->setHasHistogramConstantSize(true);
		prefixScan->createResources();
		prefixScan_ = prefixScan;
	}

	{ // radix sort
		radixScatterPass_ = ref_ptr<ComputePass>::alloc("regen.compute.sort.radix.scatter");
		radixScatterPass_->computeState()->shaderDefine("NUM_SORT_KEYS", REGEN_STRING(numKeys_));
		radixScatterPass_->computeState()->shaderDefine("NUM_LAYERS", REGEN_STRING(numLayers_));
		radixScatterPass_->computeState()->shaderDefine("HISTOGRAM_SIZE", REGEN_STRING(histogramSize));
		radixScatterPass_->computeState()->setNumWorkUnits(
				static_cast<int>(numKeys_),
				static_cast<int>(numLayers_),
				1);
		radixScatterPass_->computeState()->setGroupSize(sortGroupSize_, 1, 1);
		radixScatterPass_->setInput(globalHistogramBuffer_);
		radixScatterPass_->setInput(keyBuffer_);
		StateConfigurer shaderCfg;
		shaderCfg.define("NUM_RADIX_BUCKETS", REGEN_STRING(numBuckets_));
		shaderCfg.define("ONE_LESS_NUM_RADIX_BUCKETS", REGEN_STRING(numBuckets_ - 1));
		if (useSingleValueBuffer_) {
			radixScatterPass_->setInput(valueBuffer_[0]);
			shaderCfg.define("RADIX_CONTIGUOUS_VALUE_BUFFERS", "1");
		}
		shaderCfg.addState(radixScatterPass_.get());
		radixScatterPass_->createShader(shaderCfg.cfg());
		// retrieve locations for quick state switching in radix passes
		if (useSingleValueBuffer_) {
			scatterReadIndex_ = radixScatterPass_->shaderState()->shader()->uniformLocation("readOffset");
			scatterWriteIndex_ = radixScatterPass_->shaderState()->shader()->uniformLocation("writeOffset");
		} else {
			scatterReadIndex_ = radixScatterPass_->shaderState()->shader()->uniformLocation("ReadBuffer");
			scatterWriteIndex_ = radixScatterPass_->shaderState()->shader()->uniformLocation("WriteBuffer");
		}
		scatterBitOffsetIndex_ = radixScatterPass_->shaderState()->shader()->uniformLocation("radixBitOffset");
		if (scatterReadIndex_ == -1) {
			REGEN_WARN("RadixSort: scatter read index not found in shader.");
		}
		if (scatterWriteIndex_ == -1) {
			REGEN_WARN("RadixSort: scatter write index not found in shader.");
		}
		if (scatterBitOffsetIndex_ == -1) {
			REGEN_WARN("RadixSort: scatter bit offset index not found in shader.");
		}
	}
}

void RadixSort_GPU::enable(RenderState *rs) {
	State::enable(rs);
	if (useSingleValueBuffer_) {
		sortContiguous(rs);
	} else {
		sort(rs);
	}
#ifdef RADIX_DEBUG_RESULT
	printInstanceMap(rs);
#elifdef RADIX_DEBUG_CORRECTNESS
	printInstanceMap(rs);
#endif
}

void RadixSort_GPU::sortContiguous(RenderState *rs) {
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
}

void RadixSort_GPU::sort(RenderState *rs) {
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
}

void RadixSort_GPU::printHistogram(RenderState *rs) {
	// debug histogram
	auto numWorkGroups = radixHistogramPass_->computeState()->numWorkGroups().x;
	auto histogramData = (uint32_t *) glMapNamedBufferRange(
			globalHistogramBuffer_->drawBufferRef()->bufferID(),
			globalHistogramBuffer_->drawBufferRef()->address(),
			globalHistogramBuffer_->drawBufferRef()->allocatedSize(),
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
		glUnmapNamedBuffer(globalHistogramBuffer_->drawBufferRef()->bufferID());
	}
}

void RadixSort_GPU::printInstanceMap(RenderState *rs) {
	// debug sorted output
	std::vector<double> distances(numKeys_);
	auto sortKeys = (uint32_t *) glMapNamedBufferRange(
			keyBuffer_->drawBufferRef()->bufferID(),
			keyBuffer_->drawBufferRef()->address(),
			keyBuffer_->drawBufferRef()->allocatedSize(),
			GL_MAP_READ_BIT);
	if (sortKeys) {
		for (uint32_t i = 0; i < numKeys_; ++i) {
			distances[i] = conversion::uintToFloat(sortKeys[i]);
		}
		glUnmapNamedBuffer(keyBuffer_->drawBufferRef()->bufferID());
	}

	auto idRef = valueBuffer_[outputIdx_]->drawBufferRef();
	//auto idRef = valueBuffer_->drawBufferRef();
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
