/*
 * av-stream.cpp
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#include <ogle/config.h>

#include "av-stream.h"
using namespace ogle;

AudioVideoStream::AudioVideoStream(AVStream *stream, GLint index,  GLuint chachedBytesLimit)
: stream_(stream),
  index_(index),
  cachedBytes_(0),
  chachedBytesLimit_(chachedBytesLimit),
  isActive_(GL_TRUE)
{
  open(stream);
}
AudioVideoStream::~AudioVideoStream()
{
}

GLint AudioVideoStream::index() const
{
  return index_;
}
AVCodecContext* AudioVideoStream::codec() const
{
  return codecCtx_;
}

void AudioVideoStream::setInactive()
{
  isActive_ = GL_FALSE;
}

void AudioVideoStream::open(AVStream *stream)
{
  // Get a pointer to the codec context for the video stream
  codecCtx_ = stream->codec;

  // Find the decoder for the video stream
  codec_ = avcodec_find_decoder(codecCtx_->codec_id);
  if(codec_ == NULL)
  {
    throw new AudioVideoStreamError("Unsupported codec!");
  }
  // Open codec
  if(avcodec_open2(codecCtx_, codec_, NULL) < 0)
  {
    throw new AudioVideoStreamError("Could not open codec.");
  }
}

GLuint AudioVideoStream::numFrames()
{
  boost::lock_guard<boost::mutex> lock(decodingLock_);
  return decodedFrames_.size();
}

void AudioVideoStream::pushFrame(AVFrame *frame, GLuint frameSize)
{
  {
    boost::lock_guard<boost::mutex> lock(decodingLock_);
    cachedBytes_ += frameSize;
  }
  GLuint cachedBytes, numCachedFrames;
  if(chachedBytesLimit_ > 0.0f) {
    while(isActive_) {
      cachedBytes = cachedBytes_;
      numCachedFrames = decodedFrames_.size();
      if(cachedBytes < chachedBytesLimit_ || numCachedFrames<3) {
        break;
      }
      else {
#ifdef UNIX
        usleep(20000);
#else
        boost::this_thread::sleep(boost::posix_time::milliseconds( 20 ));
#endif
      }
    }
  }
  if(isActive_) {
    boost::lock_guard<boost::mutex> lock(decodingLock_);
    decodedFrames_.push(frame);
    frameSizes_.push(frameSize);
  }
}

AVFrame* AudioVideoStream::frontFrame()
{
  boost::lock_guard<boost::mutex> lock(decodingLock_);
  return decodedFrames_.front();
}

void AudioVideoStream::popFrame()
{
  boost::lock_guard<boost::mutex> lock(decodingLock_);
  decodedFrames_.pop();
  cachedBytes_ -= frameSizes_.front();
  frameSizes_.pop();
}
