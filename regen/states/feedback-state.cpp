/*
 * feedback-state.cpp
 *
 *  Created on: 27.01.2013
 *      Author: daniel
 */

#include "feedback-state.h"

using namespace regen;

FeedbackSpecification::FeedbackSpecification(GLuint feedbackCount)
		: State(),
		  feedbackCount_(feedbackCount),
		  feedbackMode_(GL_SEPARATE_ATTRIBS),
		  feedbackStage_(GL_VERTEX_SHADER),
		  requiredBufferSize_(0) {
}

void FeedbackSpecification::addFeedback(const ref_ptr<ShaderInput> &in) {
	// remove if already added
	if (feedbackAttributeMap_.count(in->name()) > 0) { removeFeedback(in.get()); }

	GLuint feedbackCount = (feedbackCount_ == 0 ? in->numVertices() : feedbackCount_);
	feedbackCount_ = feedbackCount;

	// create feedback attribute
	ref_ptr<ShaderInput> feedback = ShaderInput::create(in);
	feedback->set_inputSize(feedbackCount * feedback->elementSize());
	feedback->set_numVertices(feedbackCount);
	feedbackAttributes_.push_front(feedback);
	feedbackAttributeMap_[in->name()] = feedbackAttributes_.begin();

	requiredBufferSize_ += feedback->inputSize();
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

GLboolean FeedbackSpecification::hasFeedback(const std::string &name) const {
	return feedbackAttributeMap_.count(name) > 0;
}



FeedbackState::FeedbackState(GLenum feedbackPrimitive, GLuint feedbackCount)
		: FeedbackSpecification(feedbackCount),
		  feedbackPrimitive_(feedbackPrimitive) {
	feedbackBuffer_ = ref_ptr<VBO>::alloc(VBO::USAGE_FEEDBACK);
	allocatedBufferSize_ = 0;

	bufferRange_.buffer_ = 0;
	bufferRange_.offset_ = 0;
	bufferRange_.size_ = 0;
}

void FeedbackState::enable(RenderState *rs) {
	if (requiredBufferSize_ != allocatedBufferSize_) {
		// free previously allocated data
		if (vboRef_.get()) { regen::VBO::free(vboRef_.get()); }
		// allocate memory and upload to GL
		if (feedbackMode_ == GL_INTERLEAVED_ATTRIBS) {
			vboRef_ = feedbackBuffer_->allocInterleaved(feedbackAttributes_);
		} else {
			vboRef_ = feedbackBuffer_->allocSequential(feedbackAttributes_);
		}
		bufferRange_.buffer_ = vboRef_->bufferID();
		bufferRange_.size_ = requiredBufferSize_;
		allocatedBufferSize_ = requiredBufferSize_;
	}

	switch (feedbackMode_) {
		case GL_INTERLEAVED_ATTRIBS:
			enableInterleaved(rs);
			break;
		default:
			enableSeparate(rs);
			break;
	}
}

void FeedbackState::disable(RenderState *rs) {
	switch (feedbackMode_) {
		case GL_INTERLEAVED_ATTRIBS:
			disableInterleaved(rs);
			break;
		default:
			disableSeparate(rs);
			break;
	}
}

void FeedbackState::enableInterleaved(RenderState *rs) {
	if (!rs->isTransformFeedbackAcive()) {
		rs->feedbackBufferRange().push(0, bufferRange_);
	}
	rs->beginTransformFeedback(feedbackPrimitive_);
}

void FeedbackState::disableInterleaved(RenderState *rs) {
	rs->endTransformFeedback();
	if (!rs->isTransformFeedbackAcive()) {
		rs->feedbackBufferRange().pop(0);
	}
}

void FeedbackState::enableSeparate(RenderState *rs) {
	if (!rs->isTransformFeedbackAcive()) {
		GLint bufferIndex = 0;
		for (auto & att : feedbackAttributes_) {
			bufferRange_.offset_ = att->offset();
			bufferRange_.size_ = att->inputSize();
			rs->feedbackBufferRange().push(bufferIndex, bufferRange_);
			bufferIndex += 1;
		}
	}
	rs->beginTransformFeedback(feedbackPrimitive_);
}

void FeedbackState::disableSeparate(RenderState *rs) {
	rs->endTransformFeedback();
	if (!rs->isTransformFeedbackAcive()) {
		for (GLuint bufferIndex = 0u; bufferIndex < feedbackAttributes_.size(); ++bufferIndex) {
			rs->feedbackBufferRange().pop(bufferIndex);
		}
	}
}

void FeedbackState::draw(GLuint numInstances) {
	glDrawArraysInstancedEXT(
			feedbackPrimitive_,
			0,
			feedbackCount_,
			numInstances);
}
