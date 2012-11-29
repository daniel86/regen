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
  VideoStream(AVStream *stream,
      int index,
      unsigned int chachedBytesLimit);
  virtual ~VideoStream();

  int width() const { return width_; }
  int height() const { return height_; }

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

  // FFMpegStream override
  virtual void decode(AVPacket *packet);

protected:
  struct SwsContext *swsCtx_;

  int width_, height_;
};

#endif /* VIDEO_STREAM_H_ */
