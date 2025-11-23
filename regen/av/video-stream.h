/*
 * video-stream.h
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#ifndef VIDEO_STREAM_H_
#define VIDEO_STREAM_H_

#include <GL/glew.h>
#include <regen/av/av-stream.h>

namespace regen {
	/**
	 * \brief ffmpeg stream that provides texture data for GL texture.
	 */
	class VideoStream : public AudioVideoStream {
	public:
		/**
		 * @param stream the stream object.
		 * @param index the stream index.
		 * @param cachedBytesLimit size limit for pre loading.
		 */
		VideoStream(AVStream *stream, int index, uint32_t cachedBytesLimit);

		~VideoStream() override;

		/**
		 * The stream handle as provided to the constructor.
		 */
		auto *stream() { return stream_; }

		/**
		 * Video width in pixels.
		 */
		auto width() const { return width_; }

		/**
		 * Video height in pixels.
		 */
		auto height() const { return height_; }

		/**
		 * Format for GL texture to match frame data.
		 */
		static GLenum texInternalFormat();

		/**
		 * Format for GL texture to match frame data.
		 */
		static GLenum texFormat();

		/**
		 * Pixel type for GL texture to match frame data.
		 */
		static GLenum texPixelType();

		// override
		void decode(AVPacket *packet) override;

		// override
		void clearQueue() override;

	protected:
		struct SwsContext *swsCtx_;
		AVFrame *currFrame_;

		AVStream *stream_;
		int width_, height_;

		void doClearQueue();
	};
} // namespace

#endif /* VIDEO_STREAM_H_ */
