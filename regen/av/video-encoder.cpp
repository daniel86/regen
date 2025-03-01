extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <regen/utility/threading.h>
#include "video-encoder.h"
#include "regen/utility/logging.h"

using namespace regen;

VideoEncoder::VideoEncoder(unsigned int width, unsigned int height)
		: width_(static_cast<int>(width)),
		  height_(static_cast<int>(height)) {
	// create two initial frames
	for (int i = 0; i < 2; ++i) {
		auto *frame = new GLubyte[width_ * height_ * 3];
		framePool_.push_back(frame);
	}
}

VideoEncoder::~VideoEncoder() {
	boost::lock_guard<boost::mutex> lock1(poolLock_);
	boost::lock_guard<boost::mutex> lock2(encodingLock_);
	isActive_ = false;
	for (auto *frame: framePool_) {
		delete[] frame;
	}
	framePool_.clear();
	sws_freeContext(swsCtx_);
	av_frame_free(&frame_);
	av_packet_free(&pkt_);
}

void VideoEncoder::setOutputStream(AVStream *stream, AVFormatContext *formatCtx, AVCodecContext *codecCtx) {
	stream_ = stream;
	formatCtx_ = formatCtx;
	codecCtx_ = codecCtx;

	pkt_ = av_packet_alloc();
	// create a frame, it will be filled with pushed frame data
	frame_ = av_frame_alloc();
	frame_->format = codecCtx_->pix_fmt;
	frame_->width = codecCtx_->width;
	frame_->height = codecCtx_->height;
	if (av_frame_get_buffer(frame_, 32) < 0) {
		throw Error("Could not allocate frame buffer");
	}

	// create a context for conversion of GL_RGB pixel data to YUV420P
	swsCtx_ = sws_getContext(
			codecCtx_->width, codecCtx_->height,
			// input format is GL_RGB
			AV_PIX_FMT_RGB24,
			codecCtx_->width, codecCtx_->height,
			codecCtx_->pix_fmt,
			SWS_BILINEAR,
			nullptr,
			nullptr,
			nullptr);
	if (!swsCtx_) {
		throw Error("Could not create sws context");
	}
}

GLubyte *VideoEncoder::reserveFrame() {
	{
		boost::lock_guard<boost::mutex> lock(poolLock_);
		if (!framePool_.empty()) {
			auto *frame = framePool_.back();
			framePool_.pop_back();
			return frame;
		}
	}
	auto *frame = new GLubyte[width_ * height_ * 3];
	return frame;
}

void VideoEncoder::freeFrame(GLubyte *frame) {
	boost::lock_guard<boost::mutex> lock(poolLock_);
	framePool_.push_back(frame);
}

void VideoEncoder::pushFrame(GLubyte *frame) {
	boost::lock_guard<boost::mutex> lock(encodingLock_);
	encodedFrames_.push(frame);
}

void VideoEncoder::encodeFrame(const GLubyte *frameData) {
	int srcStride[] = {3 * codecCtx_->width};
	// flip the frame vertically
	int srcStrideFlipped[] = {-srcStride[0]};
	const uint8_t *srcSliceFlipped[] = {frameData + (codecCtx_->height - 1) * srcStride[0]};

	// convert to YUV420P
	sws_scale(swsCtx_,
			  srcSliceFlipped,
			  srcStrideFlipped,
			  0, codecCtx_->height,
			  frame_->data,
			  frame_->linesize);
	// set the frame's timestamp
	frame_->pts = frameCounter_ * (codecCtx_->time_base.den / codecCtx_->time_base.num / fps_);
	frameCounter_++;

	// Send the frame to the encoder
	if (avcodec_send_frame(codecCtx_, frame_) < 0) {
		throw Error("Error sending frame to codec context");
	}
	// Receive the packet from the encoder
	while (avcodec_receive_packet(codecCtx_, pkt_) == 0) {
		// Rescale the packet's timestamp
		av_packet_rescale_ts(pkt_, codecCtx_->time_base, stream_->time_base);
		pkt_->stream_index = stream_->index;
		// Write the packet to the file
		if (av_interleaved_write_frame(formatCtx_, pkt_) < 0) {
			throw Error("Error writing frame");
		}
		av_packet_unref(pkt_);
	}
}

void VideoEncoder::encodeNextFrame() {
	if (!isActive_) {
		return;
	}
	GLubyte *frame = nullptr;
	{
		boost::lock_guard<boost::mutex> lock(encodingLock_);
		if (encodedFrames_.empty()) {
			return;
		}
		frame = encodedFrames_.front();
		encodedFrames_.pop();
	}
	encodeFrame(frame);
	freeFrame(frame);
}

void VideoEncoder::finalizeEncoding() {
	isActive_ = false;

	avcodec_send_frame(codecCtx_, nullptr);
	while (avcodec_receive_packet(codecCtx_, pkt_) == 0) {
		av_packet_rescale_ts(pkt_, codecCtx_->time_base, stream_->time_base);
		pkt_->stream_index = stream_->index;
		av_interleaved_write_frame(formatCtx_, pkt_);
		av_packet_unref(pkt_);
	}

	av_write_trailer(formatCtx_);
	if (!(formatCtx_->oformat->flags & AVFMT_NOFILE)) {
		avio_closep(&formatCtx_->pb);
	}
}
