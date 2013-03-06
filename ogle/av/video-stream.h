/*
 * video-stream.h
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#ifndef VIDEO_STREAM_H_
#define VIDEO_STREAM_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <ogle/av/av-stream.h>

namespace ogle {

class VideoStreamError : public runtime_error {
public:
  VideoStreamError(const string &message)
  : runtime_error(message)
  {
  }
};

class VideoStream : public AudioVideoStream
{
public:
  VideoStream(AVStream *stream, GLint index, GLuint chachedBytesLimit);
  virtual ~VideoStream();

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
  GLenum texInternalFormat() const;
  /**
   * Format for GL texture to match frame data.
   */
  GLenum texFormat() const;
  /**
   * Pixel type for GL texture to match frame data.
   */
  GLenum texPixelType() const;

  // override
  virtual void decode(AVPacket *packet);
  virtual void clearQueue();

protected:
  struct SwsContext *swsCtx_;

  AVStream *stream_;
  GLint width_, height_;
};

} // end ogle namespace

#endif /* VIDEO_STREAM_H_ */
