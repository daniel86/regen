/*
 * demuxer.cpp
 *
 *  Created on: 10.04.2012
 *      Author: daniel
 */

#include <regen/utility/logging.h>

#include "demuxer.h"

using namespace regen;

GLboolean Demuxer::initialled_ = GL_FALSE;

static void avLogCallback(void *, int level, const char *msg, va_list args) {
	char buffer[256];
	int count = vsprintf(buffer, msg, args);
	buffer[count - 1] = '\0';
	switch (level) {
		case AV_LOG_ERROR: REGEN_ERROR(buffer);
			break;
		case AV_LOG_INFO: REGEN_INFO(buffer);
			break;
		case AV_LOG_DEBUG:
			//REGEN_DEBUG(buffer);
			break;
		case AV_LOG_WARNING: REGEN_WARN(buffer);
			break;
	}
}

void Demuxer::initAVLibrary() {
	if (!initialled_) {
		av_log_set_callback(avLogCallback);
		initialled_ = GL_TRUE;
	}
}

Demuxer::Demuxer(const std::string &file)
		: formatCtx_(nullptr),
		  pauseFlag_(GL_TRUE),
		  repeatStream_(GL_FALSE) {
	initAVLibrary();

	seeked_ = GL_FALSE;
	seek_.isRequired = GL_FALSE;

	set_file(file);
}

Demuxer::Demuxer()
		: formatCtx_(nullptr),
		  pauseFlag_(GL_TRUE),
		  repeatStream_(GL_FALSE) {
	initAVLibrary();

	seeked_ = GL_FALSE;
	seek_.isRequired = GL_FALSE;
}

Demuxer::~Demuxer() {
	// Close the video file
	if (formatCtx_) avformat_close_input(&formatCtx_);
}

void Demuxer::set_repeat(GLboolean repeat) {
	repeatStream_ = repeat;
}

GLboolean Demuxer::repeat() const {
	return repeatStream_;
}

GLfloat Demuxer::totalSeconds() const {
	return formatCtx_ ? (formatCtx_->duration / (GLdouble) AV_TIME_BASE) : 0.0f;
}

GLboolean Demuxer::isPlaying() const {
	return !pauseFlag_;
}

GLboolean Demuxer::hasInput() const {
	return formatCtx_ != nullptr;
}

void Demuxer::set_file(const std::string &file) {
	// (re)open file
	if (formatCtx_) {
		avformat_close_input(&formatCtx_);
		formatCtx_ = nullptr;
	}
	if (avformat_open_input(&formatCtx_, file.c_str(), nullptr, nullptr) != 0) {
		throw Error(REGEN_STRING("Couldn't open file " << file));
	}

	// Retrieve stream information
	if (avformat_find_stream_info(formatCtx_, nullptr) < 0) {
		throw Error("Couldn't find stream information");
	}

	// Find the first video/audio stream
	videoStreamIndex_ = -1;
	audioStreamIndex_ = -1;
	for (GLuint i = 0u; i < formatCtx_->nb_streams; ++i) {
		auto stream = formatCtx_->streams[i];
		auto codec_type = stream->codecpar->codec_type;
		if (videoStreamIndex_ == -1 && codec_type == AVMEDIA_TYPE_VIDEO) { videoStreamIndex_ = i; }
		else if (audioStreamIndex_ == -1 && codec_type == AVMEDIA_TYPE_AUDIO) { audioStreamIndex_ = i; }
	}
	if (videoStreamIndex_ != -1) {
		videoStream_ = ref_ptr<VideoStream>::alloc(
				formatCtx_->streams[videoStreamIndex_], videoStreamIndex_, 100);
	}
	if (audioStreamIndex_ != -1) {
		audioStream_ = ref_ptr<AudioSource>::alloc(
				formatCtx_->streams[audioStreamIndex_], audioStreamIndex_, -1);
	}
}

void Demuxer::play() {
	pauseFlag_ = GL_FALSE;
}

void Demuxer::pause() {
	pauseFlag_ = GL_TRUE;
	if (audioStream_.get()) {
		audioStream_->pause();
	}
	clearQueue();
}

void Demuxer::togglePlay() {
	if (pauseFlag_) { play(); }
	else { pause(); }
}

void Demuxer::stop() {
	clearQueue();
	seekTo(0.0);
	pause();
}

void Demuxer::clearQueue() {
	if (videoStream_.get()) {
		videoStream_->clearQueue();
		avcodec_flush_buffers(videoStream_->codec());
	}
	if (audioStream_.get()) {
		audioStream_->clearQueue();
		avcodec_flush_buffers(audioStream_->codec());
	}
}

void Demuxer::setInactive() {
	if (videoStream_.get()) { videoStream_->setInactive(); }
	if (audioStream_.get()) { audioStream_->setInactive(); }
}

void Demuxer::seekTo(GLdouble p) {
	if (!formatCtx_) { return; }
	p = std::max(0.0, std::min(1.0, p));
	seek_.isRequired = GL_TRUE;
	seek_.flags &= ~AVSEEK_FLAG_BYTE;
	seek_.rel = 0;
	seek_.pos = p * formatCtx_->duration;
	if (formatCtx_->start_time != AV_NOPTS_VALUE) {
		seek_.pos += formatCtx_->start_time;
	}
}

GLboolean Demuxer::decode() {
	AVPacket packet;

	SeekPosition seek = seek_;
	seek_.isRequired = GL_FALSE;
	if (seek.isRequired) {
		int64_t seek_min = seek.rel > 0 ? seek.pos - seek.rel + 2 : INT64_MIN;
		int64_t seek_max = seek.rel < 0 ? seek.pos - seek.rel - 2 : INT64_MAX;

		int ret = avformat_seek_file(formatCtx_,
									 -1, seek_min, seek.pos, seek_max, seek.flags);
		if (ret < 0) {
			REGEN_ERROR("seeking failed");
		} else {
			clearQueue();
			seeked_ = GL_TRUE;
		}
	} else if (pauseFlag_) {
		return GL_TRUE;
	}

	if (av_read_frame(formatCtx_, &packet) < 0) {
		// end of stream reached
		if (repeatStream_) {
			seekTo(0.0);
			av_packet_unref(&packet);
			return GL_FALSE;
		} else {
			stop();
			return GL_TRUE;
		}
	}

	// Is this a packet from the video stream?
	if (packet.stream_index == videoStreamIndex_) {
		videoStream_->decode(&packet);
	} else if (packet.stream_index == audioStreamIndex_) {
		audioStream_->decode(&packet);
	} else {
		// Free the packet that was allocated by av_read_frame
		av_packet_unref(&packet);
	}

	return GL_FALSE;
}
