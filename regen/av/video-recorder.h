#ifndef REGEN_VIDEO_RECORDER_H_
#define REGEN_VIDEO_RECORDER_H_

#include <GL/glew.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <regen/gl-types/fbo.h>
#include <regen/animations/animation.h>
#include "video-encoder.h"
#include "regen/gl-types/pbo.h"

namespace regen {
	/**
	 * \brief Records a video from a FBO and saves it to a file.
	 */
	class VideoRecorder : public Animation {
	public:
		/**
		 * \brief Creates a new video recorder.
		 * @param fbo the FBO to record from.
		 * @param attachment the attachment to record.
		 */
		explicit VideoRecorder(const ref_ptr<FBO> &fbo, GLenum attachment = GL_COLOR_ATTACHMENT0);

		VideoRecorder(const VideoRecorder &other) = delete;

		~VideoRecorder() override;

		/**
		 * Changes the FBO to record from.
		 * Can be done while recording.
		 * @param fbo the FBO to record from.
		 * @param attachment the attachment to record.
		 */
		void setFrameBuffer(const ref_ptr<FBO> &fbo, GLenum attachment = GL_COLOR_ATTACHMENT0);

		/**
		 * \brief Sets the filename to save the video to.
		 * @param filename the filename.
		 */
		void setFilename(const std::string &filename) { filename_ = filename; }

		/**
		 * @return the filename to save the video to.
		 */
		auto &filename() const { return filename_; }

		/**
		 * \brief Sets the frames per second for the video.
		 * @param fps the frames per second.
		 */
		void setFramesPerSecond(unsigned int fps) { encoder_->setFramesPerSecond(fps); }

		/**
		 * \brief Initializes the video recorder.
		 */
		void initialize();

		/**
		 * \brief Finalizes the video recording.
		 */
		void finalize();

		// Override Animation::glAnimate
		void glAnimate(RenderState *rs, GLdouble dt) override;

	protected:
		ref_ptr<VideoEncoder> encoder_;
		ref_ptr<Animation> encoderThread_;
		ref_ptr<FBO> encoderFBO_;
		ref_ptr<FBO> fbo_;
		ref_ptr<PBO> pbo_;
		int pboIndex_ = 0;
		GLenum attachment_;
		unsigned int frameSize_;
		double elapsedTime_ = 0.0;
		std::string filename_;

		AVFormatContext *formatCtx_ = nullptr;
		AVCodecContext *codecCtx_ = nullptr;
		AVStream *stream_ = nullptr;

		void updateFrameBuffer();
	};
} // namespace

#endif /* REGEN_VIDEO_RECORDER_H_ */
