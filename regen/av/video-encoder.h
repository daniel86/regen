#ifndef REGEN_VIDEO_ENCODER_H_
#define REGEN_VIDEO_ENCODER_H_

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <GL/glew.h>

#include <iostream>
#include <stdexcept>
#include <queue>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

namespace regen {
	/**
	 * \brief Encodes video frames into a video stream.
	 */
	class VideoEncoder {
	public:
		/**
		 * \brief An error occurred during stream processing.
		 */
		class Error : public std::runtime_error {
		public:
			/**
			 * @param message the error message.
			 */
			explicit Error(const std::string &message) : std::runtime_error(message) {}
		};

		/**
		 * \brief Creates a new video encoder.
		 * @param width the width of the video.
		 * @param height the height of the video.
		 */
		VideoEncoder(unsigned int width, unsigned int height);

		~VideoEncoder();

		/**
		 * @return the width of the video.
		 */
		auto width() const { return width_; }

		/**
		 * @return the height of the video.
		 */
		auto height() const { return height_; }

		/**
		 * \brief Sets the output stream for the video encoder.
		 * @param stream the stream to write to.
		 * @param formatCtx the format context.
		 * @param codecCtx the codec context.
		 */
		void setOutputStream(AVStream *stream, AVFormatContext *formatCtx, AVCodecContext *codecCtx);

		/**
		 * \brief Sets the frames per second for the video.
		 * @param fps the frames per second.
		 */
		void setFramesPerSecond(unsigned int fps) { fps_ = fps; }

		/**
		 * @return the frames per second for the video.
		 */
		auto fps() const { return fps_; }

		/**
		 * \brief Finalizes the encoding process.
		 */
		void finalizeEncoding();

		/**
		 * @return a frame with allocated memory.
		 */
		GLubyte *reserveFrame();

		/**
		 * \brief Frees the memory of a frame.
		 * @param frame the frame to free.
		 */
		void freeFrame(GLubyte *frame);

		/**
		 * \brief Encodes the next frame.
		 */
		void encodeNextFrame();

		/**
		 * \brief Encodes a frame.
		 * @param frameData the frame data to encode.
		 */
		void encodeFrame(const GLubyte *frameData, double dt_seconds);

		/**
		 * \brief Pushes a frame to the encoder.
		 * @param frame the frame to push.
		 */
		void pushFrame(GLubyte *frame, double dt);

	protected:
		boost::mutex encodingLock_;
		boost::mutex poolLock_;
		AVFormatContext *formatCtx_ = nullptr;
		AVCodecContext *codecCtx_ = nullptr;
		struct SwsContext *swsCtx_ = nullptr;
		AVStream *stream_ = nullptr;
		AVFrame *frame_ = nullptr;
		int width_;
		int height_;

		std::queue<std::pair<GLubyte*,double>> encodedFrames_;
		std::vector<GLubyte *> framePool_;
		AVPacket *pkt_ = nullptr;
		double cumulativeTime_ = 0.0;
		unsigned int fps_ = 40;

		bool isActive_ = true;
	};
} // namespace

#endif /* REGEN_VIDEO_ENCODER_H_ */
