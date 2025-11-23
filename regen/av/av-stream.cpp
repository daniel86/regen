/*
 * av-stream.cpp
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#include <regen/config.h>
#include <regen/utility/threading.h>

#include "av-stream.h"

using namespace regen;

AudioVideoStream::AudioVideoStream(AVStream *stream, int index, uint32_t cachedBytesLimit)
		: stream_(nullptr),
		  codecCtx_(nullptr),
		  codec_(nullptr),
		  index_(-1),
		  cachedBytes_(0),
		  cachedBytesLimit_(cachedBytesLimit),
		  isActive_(false) {
	open(stream, index, true);
}

AudioVideoStream::AudioVideoStream(uint32_t cachedBytesLimit)
		: stream_(nullptr),
		  codecCtx_(nullptr),
		  codec_(nullptr),
		  index_(-1),
		  cachedBytes_(0),
		  cachedBytesLimit_(cachedBytesLimit),
		  isActive_(false) {
}

AudioVideoStream::~AudioVideoStream() {
	close();
}

void AudioVideoStream::close() {
	if (codecCtx_) {
		avcodec_free_context(&codecCtx_);
		codecCtx_ = nullptr;
	}
}

void AudioVideoStream::open(AVStream *stream, int index, bool initial) {
	if (!initial) {
		clearQueue();
	}
	// Destroy the codec context if it exists
	close();
	// Create a new codec context for this stream
	codecCtx_ = avcodec_alloc_context3(nullptr);
	avcodec_parameters_to_context(codecCtx_, stream->codecpar);

	// Find the decoder for the video stream
	codec_ = avcodec_find_decoder(codecCtx_->codec_id);
	if (!codec_) {
		throw Error("Unsupported codec!");
	}
	// Open codec
	if (avcodec_open2(codecCtx_, codec_, nullptr) < 0) {
		throw Error("Could not open codec.");
	}
	stream_ = stream;
	index_ = index;
	cachedBytes_ = 0;
	isActive_ = true;
}

uint32_t AudioVideoStream::numFrames() {
	boost::lock_guard<boost::mutex> lock(decodingLock_);
	return decodedFrames_.size();
}

void AudioVideoStream::pushFrame(AVFrame *frame, uint32_t frameSize) {
	{
		boost::lock_guard<boost::mutex> lock(decodingLock_);
		cachedBytes_ += frameSize;
	}
	uint32_t cachedBytes, numCachedFrames;
	if (cachedBytesLimit_ > 0u) {
		while (isActive_) {
			cachedBytes = cachedBytes_;
			numCachedFrames = decodedFrames_.size();
			if (cachedBytes < cachedBytesLimit_ || numCachedFrames < 3) {
				break;
			} else {
				usleepRegen(20000);
			}
		}
	}
	if (isActive_) {
		boost::lock_guard<boost::mutex> lock(decodingLock_);
		decodedFrames_.push(frame);
		frameSizes_.push(frameSize);
	}
}

AVFrame *AudioVideoStream::frontFrame() {
	boost::lock_guard<boost::mutex> lock(decodingLock_);
	return decodedFrames_.front();
}

void AudioVideoStream::popFrame() {
	boost::lock_guard<boost::mutex> lock(decodingLock_);
	decodedFrames_.pop();
	cachedBytes_ -= frameSizes_.front();
	frameSizes_.pop();
}
