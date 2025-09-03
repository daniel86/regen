#include "buffer-block.h"
#include "ubo.h"
#include "ssbo.h"
#include "regen/states/state.h"
#include "regen/scene/shader-input-processor.h"

using namespace regen;

BufferBlock::BufferBlock(
		const std::string &name,
		BufferTarget target,
		const BufferUpdateFlags &hints,
		Qualifier blockQualifier,
		BufferMemoryLayout memoryLayout)
		: StagedBuffer(name, target, hints, memoryLayout),
		  blockQualifier_(blockQualifier) {
	enableInput_ = [this](int loc) { enableBufferBlock(loc); };
	adoptBufferRange_ = [this](uint32_t requiredSize) {
		// enforce rebinding
		bindingIndex_ = -1;
		return adoptBufferRange(requiredSize);
	};
	isBufferBlock_ = true;
}

BufferBlock::BufferBlock(const StagedBuffer &other, const std::string &name)
		: StagedBuffer(other, name),
		  blockQualifier_(BufferBlock::BUFFER) {
	enableInput_ = [this](GLint loc) { enableBufferBlock(loc); };
	isBufferBlock_ = true;
	isVertexAttribute_ = false;
}

BufferBlock::~BufferBlock() = default;

void BufferBlock::enableBufferBlock(GLint loc) {
	if (!isBufferValid_) return;
	auto *rs = RenderState::get();

	prepareRebind(loc);
	if (!shared_->isGloballyStaged_) {
		update();
	}
	rs->bufferRange(glTarget_).apply(loc, *drawBufferRange_.get());
	if (shared_->stagingBuffer_.get()) {
		// mark the point of accessing a mapped buffer segment for draw operation,
		// which is needed to avoid writing to the buffer while it is being read.
		// only used in implicit staging mode, as the fence point is set after the
		// staging-to-main copy in explicit staging mode.
		// note: this is used in global and local staging modes.
		shared_->stagingBuffer_->markDrawAccessed(*drawBufferRange_.get());
	}
	bindingIndex_ = loc;
}

void BufferBlock::bind(GLint loc) {
	auto *rs = RenderState::get();
	prepareRebind(loc);
	rs->bufferRange(glTarget_).apply(loc, *drawBufferRange_.get());
	bindingIndex_ = loc;
}

void BufferBlock::prepareRebind(GLint loc) {
	if (bindingIndex_ != loc && bindingIndex_ != -1) {
		// seems the buffer switched to another index!
		// this is something the buffer manager should try to avoid, but there are some situations
		// where it might be difficult.
		// In case of doing the switch, we need to unbind the old binding index.
		auto *rs = RenderState::get();
		auto &actual = rs->bufferRange(glTarget_).value(bindingIndex_);
		if (actual.buffer_ == drawBufferRef_->bufferID() &&
			actual.offset_ == drawBufferRef_->address() &&
			actual.size_ == drawBufferRef_->allocatedSize()) {
			//REGEN_INFO("Rebinding buffer block " << name()
			//	<< " from binding index " << bindingIndex_ << " to " << loc);
			rs->bufferRange(glTarget_).apply(bindingIndex_, BufferRange::nullReference());
			bindingIndex_ = -1;
		}
	}
}

ref_ptr<BufferBlock> BufferBlock::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto blockType = input.getValue<std::string>("type", "ubo");

	BufferUpdateFlags updateFlags;
	updateFlags.frequency = input.getValue<BufferUpdateFrequency>("update-frequency", BUFFER_UPDATE_NEVER);
	updateFlags.scope = input.getValue<BufferUpdateScope>("update-scope", BUFFER_UPDATE_FULLY);

	ref_ptr<BufferBlock> block;
	if (blockType == "ubo") {
		block = ref_ptr<UBO>::alloc(input.getName(), updateFlags);
	} else if (blockType == "ssbo") {
		block = ref_ptr<SSBO>::alloc(input.getName(), updateFlags);
	} else {
		REGEN_WARN("Unknown buffer block type '" << blockType << "'. Using UBO.");
		block = ref_ptr<UBO>::alloc(input.getName(), updateFlags);
	}
	if (input.hasAttribute("access-mode")) {
		block->setClientAccessMode(
				input.getValue<ClientAccessMode>("access-mode", BUFFER_CPU_WRITE));
	}
	if (input.hasAttribute("map-mode")) {
		block->setBufferMapMode(
				input.getValue<BufferMapMode>("map-mode", BUFFER_MAP_DISABLED));
	}
	auto dummyState = ref_ptr<State>::alloc();

	for (auto &n: input.getChildren()) {
		if (n->getCategory() == "uniform" || n->getCategory() == "input") {
			auto uniform = scene::ShaderInputProcessor::createShaderInput(
					ctx.scene(), *n.get(), dummyState);
			if (uniform->isVertexAttribute()) {
				REGEN_WARN("UBO cannot contain vertex attributes. In node '" << n->getDescription() << "'.");
				continue;
			}
			auto name = n->getValue("name");
			block->addStagedInput(uniform, name);
		} else {
			REGEN_WARN("Unknown UBO child category '" << n->getCategory() << "'.");
		}
	}
	GL_ERROR_LOG();

	return block;
}

std::ostream &regen::operator<<(std::ostream &out, const BufferBlock::Qualifier &v) {
	switch (v) {
		case BufferBlock::Qualifier::UNIFORM:
			out << "uniform";
			break;
		case BufferBlock::Qualifier::BUFFER:
			out << "buffer";
			break;
		case BufferBlock::Qualifier::IN:
			out << "in";
			break;
		case BufferBlock::Qualifier::OUT:
			out << "out";
			break;
	}
	return out;
}

std::istream &regen::operator>>(std::istream &in, BufferBlock::Qualifier &v) {
	std::string val;
	in >> val;
	boost::to_lower(val);
	if (val == "uniform") v = BufferBlock::Qualifier::UNIFORM;
	else if (val == "buffer") v = BufferBlock::Qualifier::BUFFER;
	else if (val == "in") v = BufferBlock::Qualifier::IN;
	else if (val == "out") v = BufferBlock::Qualifier::OUT;
	else {
		REGEN_WARN("Unknown storage qualifier '" << val << "'. Using UNIFORM.");
		v = BufferBlock::Qualifier::UNIFORM;
	}
	return in;
}
