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

class LibAVStream
{
public:
  LibAVStream(
      AVStream *stream,
      int index,
      unsigned int chachedBytesLimit);
  ~LibAVStream();

  int index() const { return index_; }

  /**
   * Decodes single packet.
   */
  virtual void decode(AVPacket *packet) = 0;

  /**
   * Push a decoded frame onto queue of frames.
   * The frames may get processed in a seperate thread.
   */
  void pushFrame(AVFrame *frame, unsigned int frameSize);
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
  unsigned int numFrames();
  void clearQueue();

protected:
  boost::mutex decodingLock_;
  AVStream *stream_;
  AVCodecContext *codecCtx_;
  AVCodec *codec_;
  int index_;

  queue< AVFrame* > decodedFrames_;
  queue< int > frameSizes_;

  unsigned int cachedBytes_;
  unsigned int chachedBytesLimit_;

  void open(AVStream *streams);
};

#endif /* FFMPEG_STREAM_H_ */
