#include "prefix-scan.h"
#include "regen/gl-types/gl-param.h"

using namespace regen;

PrefixScan::PrefixScan(const ref_ptr<SSBO> &histogram, uint32_t numLayers)
		: State(),
		  numLayers_(numLayers),
		  globalHistogramBuffer_(histogram) {
}

uint32_t PrefixScan::getNumDesiredInvocations() const {
	auto x = math::nextPow2(currentHistogramSize_);
	if (!hasHistogramConstantSize_) {
		// avoid too many resizes when grid is small
		x = std::max(x, 128u);
	}
	return x;
}

PrefixScan::Mode PrefixScan::getScanMode(uint32_t numDesiredInvocations) {
	// we need to check if the currently selected scan mode hits some limits,
	// and if so, we need to switch to a different mode.
	if (scanMode_ == Mode::SERIAL) return scanMode_;
	uint32_t maxSharedMem = glParam<int>(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE);
	uint32_t maxWorkGroupInvocations = glParam<int>(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS);
	uint32_t parallelScanMemory = numDesiredInvocations * 4u; // 4 bytes per uint32
	bool isParallelScanPossible = (
			parallelScanMemory <= maxSharedMem &&
			numDesiredInvocations <= maxWorkGroupInvocations);
	if (isParallelScanPossible) {
		return Mode::PARALLEL;
	} else {
		return Mode::HIERARCHICAL;
	}
}

void PrefixScan::createSerialPass() {
	StateConfigurer shaderCfg;
	computeGlobalOffsets_s_ = ref_ptr<ComputePass>::alloc("regen.compute.prefix-scan.serial");
	computeGlobalOffsets_s_->computeState()->setNumWorkUnits(1, numLayers_, 1);
	computeGlobalOffsets_s_->computeState()->setGroupSize(1, 1, 1);
	computeGlobalOffsets_s_->setInput(globalHistogramBuffer_);
	if (hasHistogramConstantSize_) {
		shaderCfg.define("SCAN_HISTOGRAM_SIZE", REGEN_STRING(currentHistogramSize_));
	} else {
		shaderCfg.define("SCAN_DYNAMIC_HISTOGRAM_SIZE", "TRUE");
	}
	shaderCfg.addState(computeGlobalOffsets_s_.get());
	computeGlobalOffsets_s_->createShader(shaderCfg.cfg());
	if (!hasHistogramConstantSize_) {
		histogramSizeLoc_s_ = computeGlobalOffsets_s_->shaderState()->shader()->uniformLocation("histogramSize");
	}
}

void PrefixScan::updateSerialPass() {
	if (!computeGlobalOffsets_s_.get()) {
		createSerialPass();
		return;
	}
}

void PrefixScan::createParallelPass(uint32_t parallelScanInvocations) {
	computeGlobalOffsets_p_ = ref_ptr<ComputePass>::alloc("regen.compute.prefix-scan.parallel");
	computeGlobalOffsets_p_->computeState()->setNumWorkUnits(parallelScanInvocations, numLayers_, 1);
	computeGlobalOffsets_p_->computeState()->setGroupSize(parallelScanInvocations, 1, 1);
	computeGlobalOffsets_p_->setInput(globalHistogramBuffer_);
	computeGlobalOffsets_p_cfg_.addState(computeGlobalOffsets_p_.get());
	computeGlobalOffsets_p_cfg_.define("NUM_SCAN_THREADS", REGEN_STRING(parallelScanInvocations));
	if (hasHistogramConstantSize_) {
		computeGlobalOffsets_p_cfg_.define("SCAN_HISTOGRAM_SIZE", REGEN_STRING(currentHistogramSize_));
	} else {
		computeGlobalOffsets_p_cfg_.define("SCAN_DYNAMIC_HISTOGRAM_SIZE", "TRUE");
	}
	computeGlobalOffsets_p_->createShader(computeGlobalOffsets_p_cfg_.cfg());
	if (!hasHistogramConstantSize_) {
		histogramSizeLoc_p_ = computeGlobalOffsets_p_->shaderState()->shader()->uniformLocation("histogramSize");
	}
	REGEN_DEBUG("Using parallel scan for histogram with " << currentHistogramSize_ <<
			  " using " << parallelScanInvocations << " threads");
}

void PrefixScan::updateParallelPass(uint32_t parallelScanInvocations) {
	if (!computeGlobalOffsets_p_.get()) {
		createParallelPass(parallelScanInvocations);
		return;
	}
	auto currentNumInvocations = computeGlobalOffsets_p_->computeState()->workGroupSize().x;
	if (currentNumInvocations != parallelScanInvocations) {
		// The number of required invocations changed, this means we need to recompile the shader
		// as the shared memory uses an array of size parallelScanInvocations.
		computeGlobalOffsets_p_->computeState()->setNumWorkUnits(parallelScanInvocations, numLayers_, 1);
		computeGlobalOffsets_p_->computeState()->setGroupSize(parallelScanInvocations, 1, 1);
		computeGlobalOffsets_p_cfg_.define("NUM_SCAN_THREADS", REGEN_STRING(parallelScanInvocations));
		computeGlobalOffsets_p_cfg_.define("CS_LOCAL_SIZE_X", REGEN_STRING(parallelScanInvocations));
		computeGlobalOffsets_p_->createShader(computeGlobalOffsets_p_cfg_.cfg());
		histogramSizeLoc_p_ = computeGlobalOffsets_p_->shaderState()->shader()->uniformLocation("histogramSize");
		REGEN_DEBUG("PrefixScan: Number of parallel scan invocations changed from "
						   << currentNumInvocations << " to " << parallelScanInvocations);
	}
}

void PrefixScan::createHierarchicalPass() {
	// divide the histogram into "blocks" of scanGroupSize_
	int32_t numBlocks = ceil(static_cast<float>(currentHistogramSize_) / static_cast<float>(scanGroupSize_));
	// need to enforce power of two below
	auto numBlocks2 = static_cast<int32_t>(math::nextPow2(numBlocks));

	{ // global memory for the offsets
		blockOffsetsBuffer_ = ref_ptr<SSBO>::alloc(
			"BlockOffsetsBuffer",
			BufferUpdateFlags::FULL_PER_FRAME,
			SSBO::RESTRICT);
		blockOffsetsBuffer_->addStagedInput(ref_ptr<ShaderInput1ui>::alloc(
			"blockOffsets", numBlocks * numLayers_));
		blockOffsetsBuffer_->stagedInputs()[0].in_->set_forceArray(true);
		blockOffsetsBuffer_->update();
	}
	{ // pass 1: local offsets
		computeLocalOffsets_ = ref_ptr<ComputePass>::alloc("regen.compute.prefix-scan.local");
		computeLocalOffsets_->computeState()->setGroupSize(scanGroupSize_, 1, 1);
		computeLocalOffsets_->computeState()->setNumWorkUnits(currentHistogramSize_, numLayers_, 1);
		computeLocalOffsets_->setInput(globalHistogramBuffer_);
		computeLocalOffsets_->setInput(blockOffsetsBuffer_);

		StateConfigurer shaderCfg;
		if (hasHistogramConstantSize_) {
			shaderCfg.define("SCAN_HISTOGRAM_SIZE", REGEN_STRING(currentHistogramSize_));
			shaderCfg.define("SCAN_NUM_BLOCKS", REGEN_STRING(numBlocks));
		} else {
			shaderCfg.define("SCAN_DYNAMIC_HISTOGRAM_SIZE", "TRUE");
		}
		shaderCfg.addState(computeLocalOffsets_.get());
		computeLocalOffsets_->createShader(shaderCfg.cfg());
		if (!hasHistogramConstantSize_) {
			histogramSizeLoc_h_l_ = computeLocalOffsets_->shaderState()->shader()->uniformLocation("histogramSize");
			numBlocksLoc_h_l_ = computeLocalOffsets_->shaderState()->shader()->uniformLocation("numBlocks");
		}
	}
	{ // pass 2: global offsets
		computeGlobalOffsets_h_ = ref_ptr<ComputePass>::alloc("regen.compute.prefix-scan.global");
		computeGlobalOffsets_h_->computeState()->setGroupSize(numBlocks2, 1, 1);
		computeGlobalOffsets_h_->computeState()->setNumWorkUnits(numBlocks2, numLayers_, 1);
		computeGlobalOffsets_h_->setInput(blockOffsetsBuffer_);
		if (hasHistogramConstantSize_) {
			computeGlobalOffsets_h_cfg_.define("SCAN_NUM_BLOCKS", REGEN_STRING(numBlocks));
		} else {
			computeGlobalOffsets_h_cfg_.define("SCAN_DYNAMIC_HISTOGRAM_SIZE", "TRUE");
		}
		computeGlobalOffsets_h_cfg_.addState(computeGlobalOffsets_h_.get());
		computeGlobalOffsets_h_->createShader(computeGlobalOffsets_h_cfg_.cfg());
		if (!hasHistogramConstantSize_) {
			numBlocksLoc_h_g_ = distributeOffsets_->shaderState()->shader()->uniformLocation("numBlocks");
		}
	}
	{ // pass 3: distribute offsets
		distributeOffsets_ = ref_ptr<ComputePass>::alloc("regen.compute.prefix-scan.distribute");
		distributeOffsets_->computeState()->setGroupSize(scanGroupSize_, 1, 1);
		distributeOffsets_->computeState()->setNumWorkUnits(currentHistogramSize_, numLayers_, 1);
		distributeOffsets_->setInput(globalHistogramBuffer_);
		distributeOffsets_->setInput(blockOffsetsBuffer_);

		StateConfigurer shaderCfg;
		if (hasHistogramConstantSize_) {
			shaderCfg.define("SCAN_HISTOGRAM_SIZE", REGEN_STRING(currentHistogramSize_));
			shaderCfg.define("SCAN_NUM_BLOCKS", REGEN_STRING(numBlocks));
		} else {
			shaderCfg.define("SCAN_DYNAMIC_HISTOGRAM_SIZE", "TRUE");
		}
		shaderCfg.addState(distributeOffsets_.get());
		distributeOffsets_->createShader(shaderCfg.cfg());
		if (!hasHistogramConstantSize_) {
			histogramSizeLoc_h_d_ = distributeOffsets_->shaderState()->shader()->uniformLocation("histogramSize");
			numBlocksLoc_h_d_ = distributeOffsets_->shaderState()->shader()->uniformLocation("numBlocks");
		}
	}
	currentNumBlocks_ = numBlocks;
	REGEN_DEBUG("Using hierarchical scan for histogram with " << currentHistogramSize_
														   << " elements, " << numBlocks << " blocks, wg size "
														   << scanGroupSize_
														   << ", " << numBlocks2 << " work groups.");
}

void PrefixScan::updateHierarchicalPass() {
	if (!computeGlobalOffsets_h_.get()) {
		createHierarchicalPass();
		return;
	}
	// divide the histogram into "blocks" of scanGroupSize_
	uint32_t numBlocks = ceil(static_cast<float>(currentHistogramSize_) / static_cast<float>(scanGroupSize_));
	// need to enforce power of two below
	auto numBlocks2 = math::nextPow2(numBlocks);
	{ // global memory for the offsets -> resize to numBlocks * numLayers_
		auto &offsets = blockOffsetsBuffer_->stagedInputs()[0].in_;
		auto oldNumBlocks = offsets->numArrayElements();
		if (oldNumBlocks != numBlocks * numLayers_) {
			offsets->set_numArrayElements(numBlocks * numLayers_);
			blockOffsetsBuffer_->update();
		}
	}
	{ // pass 1: local offsets -> change numWorkUnits
		computeLocalOffsets_->computeState()->setNumWorkUnits(currentHistogramSize_, numLayers_, 1);
	}
	{ // pass 3: distribute offsets
		distributeOffsets_->computeState()->setNumWorkUnits(currentHistogramSize_, numLayers_, 1);
	}
	{ // pass 2: global offsets
		auto oldNumBlocks2 = computeGlobalOffsets_h_->computeState()->workGroupSize().x;
		if (oldNumBlocks2 != numBlocks2) {
			computeGlobalOffsets_h_->computeState()->setGroupSize(numBlocks2, 1, 1);
			computeGlobalOffsets_h_->computeState()->setNumWorkUnits(numBlocks2, numLayers_, 1);
			computeGlobalOffsets_h_cfg_.define("CS_LOCAL_SIZE_X", REGEN_STRING(numBlocks2));
			computeGlobalOffsets_h_cfg_.define("SCAN_NUM_BLOCKS", REGEN_STRING(numBlocks));
			computeGlobalOffsets_h_->createShader(computeGlobalOffsets_h_cfg_.cfg());
			REGEN_DEBUG("PrefixScan: Number of global scan invocations changed from "
					   << oldNumBlocks2 << " to " << numBlocks2);
		}
	}
	currentNumBlocks_ = numBlocks;
}

void PrefixScan::createResources() {
	currentHistogramSize_ = globalHistogramBuffer_->allocatedSize() / (sizeof(uint32_t) * numLayers_);
	// compute offsets by scanning. We prefer here to do a single-pass parallel scan, if possible.
	// But we need to check if the histogram fits into shared memory and if the number of
	// work group invocations is not too high. Else we need to do a hierarchical scan.
	auto numDesiredInvocations = getNumDesiredInvocations();
	scanMode_ = getScanMode(numDesiredInvocations);
	switch (scanMode_) {
		case Mode::HIERARCHICAL:
			createHierarchicalPass();
			break;
		case Mode::PARALLEL:
			updateParallelPass(numDesiredInvocations);
			break;
		case Mode::SERIAL:
			createSerialPass();
			break;
	}
}

void PrefixScan::updateHistogramSize(uint32_t newHistogramSize) {
	currentHistogramSize_ = newHistogramSize;
	auto numDesiredInvocations = getNumDesiredInvocations();
	scanMode_ = getScanMode(numDesiredInvocations);
	switch (scanMode_) {
		case Mode::HIERARCHICAL:
			updateHierarchicalPass();
			break;
		case Mode::PARALLEL:
			updateParallelPass(numDesiredInvocations);
			break;
		case Mode::SERIAL:
			updateSerialPass();
			break;
	}
}

void PrefixScan::enable(RenderState *rs) {
	State::enable(rs);
	scan(rs);
}

void PrefixScan::scan1(RenderState *rs) {
	switch (scanMode_) {
		case Mode::HIERARCHICAL:
			// Each workgroup scans WG_SIZE elements of in_globalHistogram and:
			//     - Writes scanned local offset to in_globalHistogram.
			//     - Writes total size of its block to in_blockOffsets[] array.
			computeLocalOffsets_->enable(rs);
			computeLocalOffsets_->disable(rs);
			// Perform parallel scan on the block offsets in order to
			// compute the global offsets for each work group.
			computeGlobalOffsets_h_->enable(rs);
			computeGlobalOffsets_h_->disable(rs);
			// Each workgroup distributes WG_SIZE elements from the
			// in_globalHistogram to the in_blockOffsets[] array.
			distributeOffsets_->enable(rs);
			distributeOffsets_->disable(rs);
			break;
		case Mode::PARALLEL:
			computeGlobalOffsets_p_->enable(rs);
			computeGlobalOffsets_p_->disable(rs);
			break;
		case Mode::SERIAL:
			computeGlobalOffsets_s_->enable(rs);
			computeGlobalOffsets_s_->disable(rs);
			break;
	}
}

void PrefixScan::scan2(RenderState *rs) {
	switch (scanMode_) {
		case Mode::HIERARCHICAL:
			computeLocalOffsets_->enable(rs);
			glUniform1ui(numBlocksLoc_h_l_, currentNumBlocks_);
			glUniform1ui(histogramSizeLoc_h_l_, currentHistogramSize_);
			computeLocalOffsets_->disable(rs);

			computeGlobalOffsets_h_->enable(rs);
			glUniform1ui(numBlocksLoc_h_g_, currentNumBlocks_);
			computeGlobalOffsets_h_->disable(rs);

			distributeOffsets_->enable(rs);
			glUniform1ui(numBlocksLoc_h_d_, currentNumBlocks_);
			glUniform1ui(histogramSizeLoc_h_d_, currentHistogramSize_);
			distributeOffsets_->disable(rs);
			break;
		case Mode::PARALLEL:
			computeGlobalOffsets_p_->enable(rs);
			glUniform1ui(histogramSizeLoc_p_, currentHistogramSize_);
			computeGlobalOffsets_p_->disable(rs);
			break;
		case Mode::SERIAL:
			computeGlobalOffsets_s_->enable(rs);
			glUniform1ui(histogramSizeLoc_s_, currentHistogramSize_);
			computeGlobalOffsets_s_->disable(rs);
			break;
	}
}

void PrefixScan::scan(RenderState *rs) {
	// Run offsets pass. As a result we will have the offsets for each work group
	// in the globalHistogramBuffer_.
	if (hasHistogramConstantSize_) {
		scan1(rs);
	} else {
		auto newHistogramSize = globalHistogramBuffer_->allocatedSize() / (sizeof(uint32_t) * numLayers_);
		auto hasHistogramSizeChanged = (newHistogramSize != currentHistogramSize_);
		if (hasHistogramSizeChanged) {
			updateHistogramSize(newHistogramSize);
			scan2(rs);
		}
		else if (hasInitializedUniforms_) {
			scan1(rs);
		}
		else {
			// make sure to set uniforms in first frame
			scan2(rs);
			hasInitializedUniforms_ = true;
		}
	}
#ifdef RADIX_DEBUG_HISTOGRAM
	printHistogram(rs);
#endif
}

void PrefixScan::printHistogram(RenderState *rs) {
	// debug histogram
	auto histogramData = (uint32_t *) glMapNamedBufferRange(
			globalHistogramBuffer_->drawBufferRef()->bufferID(),
			globalHistogramBuffer_->drawBufferRef()->address(),
			globalHistogramBuffer_->drawBufferRef()->allocatedSize(),
			GL_MAP_READ_BIT);
	if (histogramData) {
		std::stringstream sss;
		sss << "    histogram: | ";
		for (uint32_t i = 0; i < currentHistogramSize_; ++i) {
			sss << histogramData[i] << " ";
		}
		REGEN_INFO(" " << sss.str());
		glUnmapNamedBuffer(globalHistogramBuffer_->drawBufferRef()->bufferID());
	}
}
