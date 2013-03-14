/*
 * ffmpeg-stream.h
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#ifndef FFMPEG_STREAM_H_
#define FFMPEG_STREAM_H_

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
}

#include <GL/glew.h>
#include <GL/gl.h>

#include <iostream>
#include <stdexcept>
#include <queue>
using namespace std;

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

namespace ogle {
/**
 * \brief Baseclass for libav streams.
 */
class AudioVideoStream
{
public:
  /**
   * \brief An error occurred during stream processing.
   */
  class Error : public runtime_error {
  public:
    /**
     * @param message the error message.
     */
    Error(const string &message) : runtime_error(message) {}
  };

  /**
   * @param stream a av stream handle.
   * @param index index in stream.
   * @param chachedBytesLimit limit for pre-loading.
   */
  AudioVideoStream(AVStream *stream, GLint index, GLuint chachedBytesLimit);
  ~AudioVideoStream();

  /**
   * The stream index as provided to the constructor.
   */
  GLint index() const;

  /**
   * The codec loaded.
   */
  AVCodecContext* codec() const;

  /**
   * Push a decoded frame onto queue of frames.
   * The frames may get processed in a seperate thread.
   */
  void pushFrame(AVFrame *frame, GLuint frameSize);
  /**
   * Front element of the queue.
   */
  AVFrame* frontFrame();
  /**
   * Pops front element from queue.
   */
  void popFrame();
  /**
   * Number of frames in queue.
   */
  GLuint numFrames();
  /**
   * The stream may block in decode() waiting to be able
   * to push a frame onto the queue that is full.
   * Calling setInactive() will make sure that the stream
   * drops out the block so that other media can be loaded.
   */
  void setInactive();

  /**
   * Decodes a single packet.
   * @param packet the packet.
   */
  virtual void decode(AVPacket *packet) = 0;
  /**
   * Clears the packet queue.
   */
  virtual void clearQueue() = 0;

protected:
  boost::mutex decodingLock_;
  AVStream *stream_;
  AVCodecContext *codecCtx_;
  AVCodec *codec_;
  GLint index_;

  queue<AVFrame*> decodedFrames_;
  queue<GLint> frameSizes_;

  GLuint cachedBytes_;
  GLuint chachedBytesLimit_;
  GLboolean isActive_;

  void open(AVStream *streams);
};

} // end ogle namespace

#endif /* FFMPEG_STREAM_H_ */
