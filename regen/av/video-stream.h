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
		VideoStream(AVStream *stream, GLint index, GLuint cachedBytesLimit);

		~VideoStream() override;

		/**
		 * The stream handle as provided to the constructor.
		 */
		AVStream *stream();

		/**
		 * Video width in pixels.
		 */
		GLint width() const;

		/**
		 * Video height in pixels.
		 */
		GLint height() const;

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
		GLint width_, height_;

		void doClearQueue();
	};
} // namespace

#endif /* VIDEO_STREAM_H_ */
