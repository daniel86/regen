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

class LibAVStreamError : public runtime_error {
public:
  LibAVStreamError(const string &message)
  : runtime_error(message)
  {
  }
};

class AudioVideoStream
{
public:
  AudioVideoStream(AVStream *stream, GLint index, GLuint chachedBytesLimit);
  ~AudioVideoStream();

  GLint index() const { return index_; }
  AVCodecContext* codec() const { return codecCtx_; }

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
  virtual void clearQueue() = 0;
  void setInactive();

  /**
   * Decodes single packet.
   */
  virtual void decode(AVPacket *packet) = 0;

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

#endif /* FFMPEG_STREAM_H_ */
