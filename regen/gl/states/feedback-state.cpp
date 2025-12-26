#include "feedback-state.h"

using namespace regen;

FeedbackSpecification::FeedbackSpecification(uint32_t feedbackCount)
		: State(),
		  feedbackCount_(feedbackCount),
		  feedbackMode_(GL_INTERLEAVED_ATTRIBS),
		  feedbackStage_(GL_VERTEX_SHADER),
		  requiredBufferSize_(0) {
}

ref_ptr<ShaderInput> FeedbackSpecification::addFeedback(const ref_ptr<ShaderInput> &in) {
	// remove if already added
	if (feedbackAttributeMap_.count(in->name()) > 0) { removeFeedback(in.get()); }

	uint32_t feedbackCount = (feedbackCount_ == 0 ? in->numVertices() : feedbackCount_);
	feedbackCount_ = feedbackCount;

	// create feedback attribute
	ref_ptr<ShaderInput> feedback = ShaderInput::create(in);
	feedback->set_inputSize(feedbackCount * feedback->elementSize());
	feedback->set_numVertices(feedbackCount);
	feedback->set_isVertexAttribute(true);
	feedbackAttributes_.push_back(feedback);
	feedbackAttributeMap_[in->name()] = std::prev(feedbackAttributes_.end());

	requiredBufferSize_ += feedback->inputSize();

	return feedback;
}

void FeedbackSpecification::removeFeedback(ShaderInput *in) {
	auto it = feedbackAttributeMap_.find(in->name());
	if (it == feedbackAttributeMap_.end()) { return; }

	ref_ptr<ShaderInput> in_ = *(it->second);
	requiredBufferSize_ -= in_->inputSize();

	feedbackAttributes_.erase(it->second);
	feedbackAttributeMap_.erase(it);
}

ref_ptr<ShaderInput> FeedbackSpecification::getFeedback(const std::string &name) {
	auto it = feedbackAttributeMap_.find(name);
	if (it == feedbackAttributeMap_.end()) { ref_ptr<ShaderInput>(); }
	return *(it->second);
}

bool FeedbackSpecification::hasFeedback(const std::string &name) const {
	return feedbackAttributeMap_.count(name) > 0;
}



FeedbackState::FeedbackState(GLenum feedbackPrimitive, uint32_t feedbackCount)
		: FeedbackSpecification(feedbackCount),
		  feedbackPrimitive_(feedbackPrimitive) {
	feedbackMode_ = GL_SEPARATE_ATTRIBS;
	feedbackBuffer_ = ref_ptr<VBO>::alloc(
			TRANSFORM_FEEDBACK_BUFFER,
			BufferUpdateFlags::NEVER);
	allocatedBufferSize_ = 0;

	bufferRange_.buffer_ = 0;
	bufferRange_.offset_ = 0;
	bufferRange_.size_ = 0;
}

void FeedbackState::initializeResources() {
	if (requiredBufferSize_ != allocatedBufferSize_) {
		// free previously allocated data
		if (feedbackRef_.get()) { BufferObject::orphanBufferRange(feedbackRef_.get()); }

		feedbackRef_ = feedbackBuffer_->adoptBufferRange(requiredBufferSize_);
		bufferRange_.buffer_ = feedbackRef_->bufferID();
		bufferRange_.offset_ = feedbackRef_->address();
		bufferRange_.size_ = requiredBufferSize_;
		allocatedBufferSize_ = requiredBufferSize_;

		if (feedbackMode_ == GL_INTERLEAVED_ATTRIBS) {
			GLsizei vertexSize = 0;
			for (auto & att : feedbackAttributes_) {
				vertexSize += static_cast<GLsizei>(att->elementSize());
			}
			uint32_t byteOffset = feedbackRef_->address();
			for (auto & att : feedbackAttributes_) {
				att->setMainBuffer(feedbackRef_, byteOffset);
				att->setVertexStride(vertexSize);
				byteOffset += att->elementSize();
			}
		} else {
			// set up separate attributes
			uint32_t byteOffset = feedbackRef_->address();
			for (auto & att : feedbackAttributes_) {
				att->setMainBuffer(feedbackRef_, byteOffset);
				att->setVertexStride(static_cast<GLsizei>(att->alignedElementSize()));
				byteOffset += att->inputSize();
			}
		}

	}
}

void FeedbackState::enable(RenderState *rs) {
	initializeResources();
	if (feedbackMode_ == GL_INTERLEAVED_ATTRIBS) {
		if (!rs->isTransformFeedbackAcive()) {
			rs->feedbackBufferRange().push(0, bufferRange_);
		}
		rs->beginTransformFeedback(feedbackPrimitive_);
	} else {
		if (!rs->isTransformFeedbackAcive()) {
			int bufferIndex = 0;
			for (auto & att : feedbackAttributes_) {
				bufferRange_.offset_ = att->mainBufferOffset();
				bufferRange_.size_ = att->inputSize();
				rs->feedbackBufferRange().push(bufferIndex, bufferRange_);
				bufferIndex += 1;
			}
		}
		rs->beginTransformFeedback(feedbackPrimitive_);
	}
}

void FeedbackState::disable(RenderState *rs) {
	if (feedbackMode_ == GL_INTERLEAVED_ATTRIBS) {
		rs->endTransformFeedback();
		if (!rs->isTransformFeedbackAcive()) {
			rs->feedbackBufferRange().pop(0);
		}
	}
	else {
		rs->endTransformFeedback();
		if (!rs->isTransformFeedbackAcive()) {
			for (uint32_t bufferIndex = 0u; bufferIndex < feedbackAttributes_.size(); ++bufferIndex) {
				rs->feedbackBufferRange().pop(bufferIndex);
			}
		}
	}
}

void FeedbackState::draw(uint32_t numInstances) {
	glDrawArraysInstancedEXT(
			feedbackPrimitive_,
			0,
			feedbackCount_,
			numInstances);
}
