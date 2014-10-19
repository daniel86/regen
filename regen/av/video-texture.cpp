/*
 * video.cpp
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#include <stdexcept>
#include <string>
#include <climits>

extern "C" {
  #include <libavformat/avformat.h>
}

#include <regen/utility/threading.h>
#include <regen/utility/logging.h>
#include <regen/gl-types/gl-util.h>
#include <regen/config.h>

#include "video-texture.h"
using namespace regen;

// Milliseconds to sleep per loop in idle mode.
#define IDLE_SLEEP_MS 30000

VideoTexture::VideoTexture()
: Texture2D(1),
  Animation(GL_TRUE,GL_TRUE),
  closeFlag_(GL_FALSE),
  seeked_(GL_FALSE),
  fileToLoaded_(GL_FALSE),
  elapsedSeconds_(0.0),
  interval_(idleInterval_),
  idleInterval_(IDLE_SLEEP_MS/1000),
  dt_(0.0),
  intervalMili_(0),
  lastFrame_(NULL)
{
  demuxer_ = ref_ptr<Demuxer>::alloc();
  decodingThread_ = boost::thread(&VideoTexture::decode, this);
}
VideoTexture::~VideoTexture()
{
  stopDecodingThread();
  if(lastFrame_) {
    av_free(lastFrame_->data[0]);
    av_free(lastFrame_);
  }
}

GLfloat VideoTexture::elapsedSeconds() const
{
  return elapsedSeconds_;
}

void VideoTexture::play()
{
  boost::lock_guard<boost::mutex> lock(decodingLock_);
  demuxer_->play();
}
void VideoTexture::pause()
{
  boost::lock_guard<boost::mutex> lock(decodingLock_);
  demuxer_->pause();
}
void VideoTexture::togglePlay()
{
  boost::lock_guard<boost::mutex> lock(decodingLock_);
  demuxer_->togglePlay();
}
void VideoTexture::stop()
{
  boost::lock_guard<boost::mutex> lock(decodingLock_);
  demuxer_->stop();
}

void VideoTexture::seekToBegin()
{
  seekTo(0.0);
}
void VideoTexture::seekForward(GLdouble seconds)
{
  seekTo((elapsedSeconds_ + seconds)/demuxer_->totalSeconds());
}
void VideoTexture::seekBackward(GLdouble seconds)
{
  seekTo((elapsedSeconds_ - seconds)/demuxer_->totalSeconds());
}
void VideoTexture::seekTo(GLdouble p)
{
  boost::lock_guard<boost::mutex> lock(decodingLock_);
  demuxer_->seekTo(p);
  elapsedSeconds_ = p*demuxer_->totalSeconds();
  seeked_ = GL_TRUE;
}

void VideoTexture::stopDecodingThread()
{
  // stop VideoTexture::decode()
  closeFlag_ = GL_TRUE;
  demuxer_->setInactive();
  // wait for the decoding thread to stop
  decodingThread_.join();
}

void VideoTexture::set_file(const std::string &file)
{
  // exit decoding thread
  stopDecodingThread();

  demuxer_->set_file(file);
  as_ = demuxer_->audioStream();
  vs_ = demuxer_->videoStream();
  if(vs_.get()) { // setup the texture target
    set_rectangleSize(vs_->width(), vs_->height());
    set_internalFormat(vs_->texInternalFormat());
    set_format(vs_->texFormat());
    set_pixelType(vs_->texPixelType());
  }
  fileToLoaded_ = GL_TRUE;

  // start decoding
  closeFlag_ = GL_FALSE;
  decodingThread_ = boost::thread(&VideoTexture::decode, this);
}

void VideoTexture::decode()
{
  GLboolean isIdle;
  while(!closeFlag_)
  {
    if(demuxer_->hasInput()) {
      isIdle = demuxer_->decode();
    } else {
      isIdle = GL_TRUE;
    }
    if(isIdle) {
      // demuxer has nothing to do lets sleep a while
      usleepRegen(IDLE_SLEEP_MS);
    }
  }
}

void VideoTexture::animate(GLdouble animateDT)
{
  if(!demuxer_->isPlaying()) { return; }
  interval_ -= animateDT;
  dt_ += animateDT;
  if(interval_ > 0.0) { return; }

  GLuint numFrames = vs_->numFrames();
  GLboolean isIdle = (numFrames == 0);

  if(isIdle) {
    // no frames there to show
    interval_ += idleInterval_;
  }
  else {
    boost::int64_t diff = (dt_ - intervalMili_);

    // pop the first frame dropping some frames
    // if we are not fast enough showing frames
    AVFrame *frame = vs_->frontFrame();
    vs_->popFrame();

    {
      // queue calling texImage
      boost::lock_guard<boost::mutex> lock(textureUpdateLock_);
      if(lastFrame_) {
        av_free(lastFrame_->data[0]);
        av_free(lastFrame_);
      }
      set_data(frame->data[0]);
    }

    // set next interval
    GLfloat *t = (GLfloat*) frame->opaque;
    if(!seeked_) {
      // set timeout interval to time difference to last frame plus a correction
      // value because the last timeout call was not exactly the wanted interval
      GLfloat dt = (*t)-elapsedSeconds_;
      intervalMili_ = std::max(0.0f,dt*1000.0f - diff);
    }
    else {
      seeked_ = GL_FALSE;
    }
    elapsedSeconds_ = *t;
    delete t;

    if(demuxer_->audioStream().get()) {
      // synchronize with audio
      intervalMili_ += (elapsedSeconds_ -
          demuxer_->audioStream()->elapsedTime())*2;
    }

    lastFrame_ = frame;
    interval_ = intervalMili_;
  }
  dt_ = 0.0;
}
void VideoTexture::glAnimate(RenderState *rs, GLdouble dt)
{
  GL_ERROR_LOG();
  GLuint channel = rs->reserveTextureChannel();
  begin(rs,channel);
  if(fileToLoaded_) { // setup the texture target
    set_data(NULL);
    texImage();
    filter().push(GL_LINEAR);
    wrapping().push(GL_REPEAT);
    fileToLoaded_ = GL_FALSE;
  }
  // upload texture data to GL
  if(data() != NULL) {
    boost::lock_guard<boost::mutex> lock(textureUpdateLock_);
    texImage();
    set_data(NULL);
  }
  end(rs,channel);
  rs->releaseTextureChannel();
  GL_ERROR_LOG();
}

ref_ptr<AudioSource> VideoTexture::audioSource()
{
  if(demuxer_.get()) {
    return demuxer_->audioStream();
  } else {
    return ref_ptr<AudioSource>();
  }
}

const ref_ptr<Demuxer>& VideoTexture::demuxer() const
{ return demuxer_; }
