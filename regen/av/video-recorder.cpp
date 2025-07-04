extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <regen/utility/logging.h>
#include <regen/gl-types/gl-util.h>
#include "video-recorder.h"

#define REGEN_VIDEO_RECORDER_FILE "/tmp/regen.mp4"

using namespace regen;

namespace regen {
	class VideoEncoderThread : public Animation {
	public:
		explicit VideoEncoderThread(VideoEncoder *encoder)
				: Animation(false, true),
				  encoder_(encoder) {
			setSynchronized(false);
			desiredFrameRate_ = 30.0f;
		}

		// TODO: rather do the animate() in VideoRecorder, but currently unsynchronized threads seem to
		//       have problems running together with synchronized GL animation call, maybe it is about the signals.
		void animate(GLdouble dt) override {
			encoder_->encodeNextFrame();
		}

	protected:
		VideoEncoder *encoder_;
	};
}


VideoRecorder::VideoRecorder(const ref_ptr<FBO> &fbo, GLenum attachment)
		: Animation(true, false),
		  fbo_(fbo),
		  attachment_(attachment),
		  filename_(REGEN_VIDEO_RECORDER_FILE) {
	auto &tex = fbo->colorTextures()[attachment - GL_COLOR_ATTACHMENT0];
	// encoder FBO uses RGB format, hence 3 bytes per texel
	frameSize_ = tex->numTexel() * 3;
	encoder_ = ref_ptr<VideoEncoder>::alloc(tex->width(), tex->height());
	// start encoding frames
	encoderThread_ = ref_ptr<VideoEncoderThread>::alloc(encoder_.get());
	encoderThread_->startAnimation();
}

VideoRecorder::~VideoRecorder() {
	if (encoderThread_.get()) {
		encoderThread_->stopAnimation();
		encoderThread_ = {};
	}
	avcodec_free_context(&codecCtx_);
	avformat_free_context(formatCtx_);
}

void VideoRecorder::finalize() {
	encoderThread_->stopAnimation();
	encoderThread_ = {};
	encoder_->finalizeEncoding();
}

void VideoRecorder::initialize() {
	auto &tex = fbo_->colorTextures()[attachment_ - GL_COLOR_ATTACHMENT0];
	auto width = static_cast<int>(tex->width());
	auto height = static_cast<int>(tex->height());
	auto fps = static_cast<int>(encoder_->fps());

	avformat_alloc_output_context2(&formatCtx_, nullptr, nullptr, filename_.c_str());
	if (!formatCtx_) {
		throw std::runtime_error("Could not allocate format context");
	}
	auto *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	//auto *codec = avcodec_find_encoder(AV_CODEC_ID_HEVC);
	//auto *codec = avcodec_find_encoder(AV_CODEC_ID_VP9);
	if (!codec) {
		throw std::runtime_error("Codec not found");
	}
	codecCtx_ = avcodec_alloc_context3(codec);
	if (!codecCtx_) {
		throw std::runtime_error("Could not allocate codec context");
	}

	// Create the codec context
	codecCtx_->width = width;
	codecCtx_->height = height;
	// for a fixed frame rate, set time_base to {1, fps}
	//codecCtx_->time_base = {1, fps};
	codecCtx_->time_base = {1, 1000};
	codecCtx_->framerate = {fps, 1};
	codecCtx_->pix_fmt = AV_PIX_FMT_YUV420P;
	if (formatCtx_->oformat->flags & AVFMT_GLOBALHEADER) {
		codecCtx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}
	if (avcodec_open2(codecCtx_, codec, nullptr) < 0) {
		throw std::runtime_error("Could not open codec");
	}

	stream_ = avformat_new_stream(formatCtx_, codec);
	if (!stream_) {
		throw std::runtime_error("Could not create stream");
	}
	avcodec_parameters_from_context(stream_->codecpar, codecCtx_);
	if (!(formatCtx_->oformat->flags & AVFMT_NOFILE)) {
		if (avio_open(&formatCtx_->pb, filename_.c_str(), AVIO_FLAG_WRITE) < 0) {
			throw std::runtime_error("Could not open output file");
		}
	}
	if (avformat_write_header(formatCtx_, nullptr) < 0) {
		throw std::runtime_error("Error occurred when opening output file");
	}
	encoder_->setOutputStream(stream_, formatCtx_, codecCtx_);

	// Create PBOs
	pbo_ = ref_ptr<PBO>::alloc(2);
	for (int i = 0; i < 2; ++i) {
		RenderState::get()->pixelPackBuffer().push(pbo_->ids()[i]);
		glBufferData(GL_PIXEL_PACK_BUFFER, frameSize_, nullptr, GL_STREAM_READ);
	}
	for (int i = 0; i < 2; ++i) {
		RenderState::get()->pixelPackBuffer().pop();
	}

	// create a FBO for the encoder with the same size as the input FBO,
	// but with a single color attachment that has the same format as used by the encoder
	encoderFBO_ = ref_ptr<FBO>::alloc(tex->width(), tex->height());
	encoderFBO_->addTexture(1,
							GL_TEXTURE_2D, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
	GL_ERROR_LOG();
}

void VideoRecorder::setFrameBuffer(const ref_ptr<FBO> &fbo, GLenum attachment) {
	fbo_ = fbo;
	attachment_ = attachment;
}

void VideoRecorder::updateFrameBuffer() {
	auto &tex = fbo_->colorTextures()[attachment_ - GL_COLOR_ATTACHMENT0];
	auto texWidth = static_cast<int>(tex->width());
	auto texHeight = static_cast<int>(tex->height());
	int nextIndex = (pboIndex_ + 1) % 2;
	GLenum filerMode = (texWidth == codecCtx_->width && texHeight == codecCtx_->height) ?  GL_NEAREST : GL_LINEAR;

	// blit the input FBO to the encoder FBO
	fbo_->applyReadBuffer(attachment_);
	encoderFBO_->applyDrawBuffers(GL_COLOR_ATTACHMENT0);
	glBlitNamedFramebuffer(fbo_->id(), encoderFBO_->id(),
					  0, 0, texWidth, texHeight,
					  0, 0, codecCtx_->width, codecCtx_->height,
					  GL_COLOR_BUFFER_BIT, filerMode);

	// set the target framebuffer to read
	RenderState::get()->readFrameBuffer().push(encoderFBO_->id());
	encoderFBO_->applyReadBuffer(GL_COLOR_ATTACHMENT0);

	// read pixels from framebuffer to PBO glReadPixels() should return immediately.
	RenderState::get()->pixelPackBuffer().push(pbo_->ids()[pboIndex_]);
	glReadPixels(0, 0,
				 codecCtx_->width, codecCtx_->height,
				 GL_RGB, GL_UNSIGNED_BYTE,
				 nullptr);
	RenderState::get()->pixelPackBuffer().pop();

	// map the other PBO to process its data
	auto *ptr = (GLubyte *) glMapNamedBuffer(pbo_->ids()[nextIndex], GL_READ_ONLY);
	if (ptr) {
		auto nextFrame = encoder_->reserveFrame();
		std::memcpy(nextFrame, ptr, frameSize_);
		glUnmapNamedBuffer(pbo_->ids()[nextIndex]);
		encoder_->pushFrame(nextFrame, elapsedTime_);
	}

	RenderState::get()->readFrameBuffer().pop();

	pboIndex_ = nextIndex;
}

void VideoRecorder::glAnimate(RenderState *rs, GLdouble dt) {
	elapsedTime_ += (dt / 1000.0);
	if (elapsedTime_ >= 1.0 / static_cast<double>(encoder_->fps())) {
		updateFrameBuffer();
		elapsedTime_ = 0.0;
	}
}
