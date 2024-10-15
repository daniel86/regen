/*
 * video-stream.cpp
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <regen/utility/logging.h>

#include "video-stream.h"

using namespace regen;

#define GL_RGB_PIXEL_FORMAT AV_PIX_FMT_RGB24

VideoStream::VideoStream(AVStream *stream, GLint index, GLuint chachedBytesLimit)
		: AudioVideoStream(stream, index, chachedBytesLimit) {
	stream_ = stream;
	width_ = codecCtx_->width;
	height_ = codecCtx_->height;
	if (width_ < 1 || height_ < 1) throw Error("invalid video size");

	REGEN_DEBUG("init video stream" <<
									" width=" << width_ <<
									" height=" << height_ <<
									" pix_fmt=" << codecCtx_->pix_fmt <<
									" bit_rate=" << codecCtx_->bit_rate <<
									".");

	// get sws context for converting from YUV to RGB
	swsCtx_ = sws_getContext(
			codecCtx_->width,
			codecCtx_->height,
			codecCtx_->pix_fmt,
			codecCtx_->width,
			codecCtx_->height,
			GL_RGB_PIXEL_FORMAT,
			SWS_FAST_BILINEAR,
			nullptr, nullptr, nullptr);
	currFrame_ = av_frame_alloc();
}

VideoStream::~VideoStream() {
	clearQueue();
	av_free(currFrame_);
}

void VideoStream::clearQueue() {
	while (!decodedFrames_.empty()) {
		AVFrame *f = frontFrame();
		popFrame();
		delete (float *) f->opaque;
		av_free(f);
	}
}

void VideoStream::decode(AVPacket *packet) {
	int ret = avcodec_send_packet(codecCtx_, packet);
	while (ret >= 0) {
		ret = avcodec_receive_frame(codecCtx_, currFrame_);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret < 0) {
			// Need more packets or end of file or Error during decoding
			return;
		}

		// YUV to RGB conversion
		AVFrame *rgb = av_frame_alloc();
		int numBytes = av_image_get_buffer_size(
				GL_RGB_PIXEL_FORMAT,
				codecCtx_->width,
				codecCtx_->height,
				1);
		if (numBytes < 1) {
			av_frame_free(&rgb);
			return;
		}
		av_image_fill_arrays(
				rgb->data,
				rgb->linesize,
				nullptr,
				GL_RGB_PIXEL_FORMAT,
				codecCtx_->width,
				codecCtx_->height,
				1);

		sws_scale(
				swsCtx_,
				currFrame_->data,
				currFrame_->linesize,
				0,
				codecCtx_->height,
				rgb->data,
				rgb->linesize);

		// Remember timestamp in frame
		auto dt = new float;
		*dt = packet->dts * av_q2d(stream_->time_base);
		rgb->opaque = dt;

		// Free packet and put the frame in queue of decoded frames
		av_packet_unref(packet);
		pushFrame(rgb, numBytes);

		// free package and put the frame in queue of decoded frames
		av_packet_unref(packet);
		av_free(currFrame_);
		currFrame_ = av_frame_alloc();
		pushFrame(rgb, numBytes);
	}
}

GLint VideoStream::width() const { return width_; }

GLint VideoStream::height() const { return height_; }

AVStream *VideoStream::stream() { return stream_; }

GLenum VideoStream::texInternalFormat() { return GL_RGB; }

GLenum VideoStream::texFormat() { return GL_RGB; }

GLenum VideoStream::texPixelType() { return GL_UNSIGNED_BYTE; }
