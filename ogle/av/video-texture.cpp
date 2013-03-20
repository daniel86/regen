/*
 * video.cpp
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#include <stdexcept>
#include <string>
#include <climits>
using namespace std;

extern "C" {
  #include <libavformat/avformat.h>
}

#include <ogle/utility/timeout-manager.h>
#include <ogle/utility/logging.h>
#include <ogle/config.h>

#include "video-texture.h"
using namespace ogle;

// Milliseconds to sleep per loop in idle mode.
#define IDLE_SLEEP_MS 30

VideoTexture::VideoTexture()
: Texture2D(1),
  Animation(GL_TRUE,GL_TRUE),
  closeFlag_(GL_FALSE),
  seeked_(GL_FALSE),
  elapsedSeconds_(0.0),
  interval_(idleInterval_),
  idleInterval_(IDLE_SLEEP_MS),
  dt_(0.0),
  intervalMili_(0),
  lastFrame_(NULL)
{
  demuxer_ = ref_ptr<Demuxer>::manage(new Demuxer);
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

void VideoTexture::set_file(const string &file)
{
  // exit decoding thread
  stopDecodingThread();

  demuxer_->set_file(file);
  as_ = demuxer_->audioStream();
  vs_ = demuxer_->videoStream();
  if(vs_) { // setup the texture target
    set_size(vs_->width(), vs_->height());
    set_internalFormat(vs_->texInternalFormat());
    set_format(vs_->texFormat());
    set_pixelType(vs_->texPixelType());
    bind();
    set_data(NULL);
    texImage();
    set_filter(GL_LINEAR, GL_LINEAR);
    set_wrapping(GL_REPEAT);
  }

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
#ifdef UNIX
      usleep( IDLE_SLEEP_MS*1000 );
#else
      boost::this_thread::sleep(boost::posix_time::milliseconds( IDLE_SLEEP_MS ));
#endif
    }
  }
}

void VideoTexture::animate(GLdouble animateDT)
{
  interval_ -= animateDT;
  dt_ += animateDT;
  if(interval_ > 0.0) { return; }
  if(!demuxer_->isPlaying()) { return; }

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
    AVFrame *droppedFrame = NULL;
    AVFrame *frame;
    do {
      if(droppedFrame) {
        GLfloat *t = (GLfloat*) droppedFrame->opaque;
        delete t;
        av_free(droppedFrame->data[0]);
        av_free(droppedFrame);
      }
      frame = vs_->frontFrame();
      droppedFrame = frame;
      vs_->popFrame();
      diff -= intervalMili_;
    } while(diff > intervalMili_ && vs_->numFrames()>2);
    diff += intervalMili_;

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
      intervalMili_ = max(0.0f,dt*1000.0f - diff);
    }
    else {
      seeked_ = GL_FALSE;
    }
    elapsedSeconds_ = *t;
    delete t;

    lastFrame_ = frame;
    interval_ += intervalMili_;
  }
  dt_ = 0.0;
}
void VideoTexture::glAnimate(RenderState *rs, GLdouble dt)
{
  // upload texture data to GL
  if(data() != NULL) {
    GLuint channel = rs->reserveTextureChannel();
    activate(channel);
    {
      boost::lock_guard<boost::mutex> lock(textureUpdateLock_);
      texImage();
      set_data(NULL);
    }
    rs->releaseTextureChannel();
  }
}

ref_ptr<AudioSource> VideoTexture::audioSource()
{
  if(demuxer_.get() && demuxer_->audioStream()) {
    return demuxer_->audioStream()->audioSource();
  } else {
    return ref_ptr<AudioSource>();
  }
}

const ref_ptr<Demuxer>& VideoTexture::demuxer() const
{ return demuxer_; }
