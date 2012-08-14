/*
 * av-stream.cpp
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#include "av-stream.h"

AudioVideoStream::AudioVideoStream(
    AVStream *stream,
    int index,
    unsigned int chachedBytesLimit)
: index_(index),
  stream_(stream),
  cachedBytes_(0),
  chachedBytesLimit_(chachedBytesLimit)
{
  open(stream);
}
AudioVideoStream::~AudioVideoStream()
{
  clearQueue();
}

void AudioVideoStream::open(AVStream *stream)
{
  // Get a pointer to the codec context for the video stream
  codecCtx_ = stream->codec;

  // Find the decoder for the video stream
  codec_ = avcodec_find_decoder(codecCtx_->codec_id);
  if(codec_ == NULL)
  {
    throw new LibAVStreamError("Unsupported codec!");
  }
  // Open codec
  if(avcodec_open2(codecCtx_, codec_, NULL) < 0)
  {
    throw new LibAVStreamError("Could not open codec.");
  }
}

unsigned int AudioVideoStream::numFrames()
{
  boost::lock_guard<boost::mutex> lock(decodingLock_);
  return decodedFrames_.size();
}

void AudioVideoStream::pushFrame(AVFrame *frame, unsigned int frameSize)
{
  {
    boost::lock_guard<boost::mutex> lock(decodingLock_);
    cachedBytes_ += frameSize;
  }
  unsigned int cachedBytes, numCachedFrames;
  if(chachedBytesLimit_ > 0.0f) {
    while(true) {
      {
        boost::lock_guard<boost::mutex> lock(decodingLock_);
        cachedBytes = cachedBytes_;
        numCachedFrames = decodedFrames_.size();
      }
      if(cachedBytes < chachedBytesLimit_ || numCachedFrames<3) {
        break;
      } else {
        boost::this_thread::sleep(boost::posix_time::milliseconds( 10 ));
      }
    }
  }
  boost::lock_guard<boost::mutex> lock(decodingLock_);
  decodedFrames_.push(frame);
  frameSizes_.push(frameSize);
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

void AudioVideoStream::clearQueue()
{
  while(decodedFrames_.size()>0) {
    AVFrame *f = frontFrame();
    popFrame();
    av_free(f);
  }
}
