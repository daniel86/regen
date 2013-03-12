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
#include <ogle/animations/animation-manager.h>
#include <ogle/config.h>

#include "video-texture.h"
using namespace ogle;

// Milliseconds to sleep per loop in idle mode.
#define IDLE_SLEEP_MS 30

VideoTexture::VideoTextureUpdater::VideoTextureUpdater(
    VideoStream *vs, AudioStream *as, Texture2D *tex)
: Animation(),
  tex_(tex),
  vs_(vs),
  as_(as),
  lastFrame_(NULL),
  intervalMili_(0)
{
  seeked_ = GL_FALSE;
  elapsedSeconds_ = 0.0;
  idleInterval_ = IDLE_SLEEP_MS;
  interval_ = idleInterval_;
  dt_ = 0.0;
}
VideoTexture::VideoTextureUpdater::~VideoTextureUpdater()
{
  if(lastFrame_) {
    av_free(lastFrame_->data[0]);
    av_free(lastFrame_);
  }
}
void VideoTexture::VideoTextureUpdater::animate(GLdouble animateDT)
{
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
      tex_->set_data(frame->data[0]);
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
void VideoTexture::VideoTextureUpdater::glAnimate(RenderState *rs, GLdouble dt)
{
  // upload texture data to GL
  if(tex_->data() != NULL) {
    boost::lock_guard<boost::mutex> lock(textureUpdateLock_);
    GLuint channel = rs->reserveTextureChannel();
    tex_->activate(channel);
    tex_->texImage();
    tex_->set_data(NULL);
    rs->releaseTextureChannel();
  }
}
GLboolean VideoTexture::VideoTextureUpdater::useAnimation() const
{ return GL_TRUE; }
GLboolean VideoTexture::VideoTextureUpdater::useGLAnimation() const
{ return GL_TRUE; }

GLboolean VideoTexture::initialled_ = GL_FALSE;

VideoTexture::VideoTexture()
: Texture2D(1),
  formatCtx_(NULL),
  repeatStream_(GL_FALSE),
  closeFlag_(GL_FALSE),
  pauseFlag_(GL_TRUE),
  completed_(GL_FALSE)
{
  if(!initialled_) {
    av_register_all();
    av_log_set_level(AV_LOG_ERROR);
    initialled_ = GL_TRUE;
  }
  decodingThread_ = boost::thread(&VideoTexture::decode, this);
  seek_.isRequired = GL_FALSE;
}
VideoTexture::~VideoTexture()
{
  stopDecodingThread();
  // Close the video file
  if(formatCtx_) avformat_close_input(&formatCtx_);
}

GLfloat VideoTexture::totalSeconds() const
{
  return formatCtx_ ? (formatCtx_->duration/(GLdouble)AV_TIME_BASE) : 0.0f;
}
GLfloat VideoTexture::elapsedSeconds() const
{
  return textureUpdater_.get() ? textureUpdater_->elapsedSeconds_ : 0.0f;
}

ref_ptr<AudioSource> VideoTexture::audioSource()
{
  if(demuxer_.get() && demuxer_->audioStream()) {
    return demuxer_->audioStream()->audioSource();
  } else {
    return ref_ptr<AudioSource>();
  }
}

void VideoTexture::clearQueue()
{
  if(demuxer_.get()) {
    VideoStream *vs = demuxer_->videoStream();
    AudioStream *as = demuxer_->audioStream();
    if(vs!=NULL) {
      vs->clearQueue();
      avcodec_flush_buffers(vs->codec());
    }
    if(as!=NULL) {
      as->clearQueue();
      avcodec_flush_buffers(as->codec());
    }
  }
}

void VideoTexture::decode()
{
  AVPacket packet;
  SeekPosition seek;

  while(!closeFlag_)
  {
    seek = seek_;
    seek_.isRequired = GL_FALSE;

    if(seek.isRequired) {
      int64_t seek_min = seek.rel > 0 ? seek.pos - seek.rel + 2: INT64_MIN;
      int64_t seek_max = seek.rel < 0 ? seek.pos - seek.rel - 2: INT64_MAX;

      int ret = avformat_seek_file(formatCtx_,
          -1, seek_min, seek.pos, seek_max, seek.flags);
      if (ret < 0) {
        ERROR_LOG("error while seeking");
      }
      else {
        clearQueue();
        textureUpdater_->seeked_ = GL_TRUE;
      }
    }
    else if(pauseFlag_) {
      // demuxer has nothing to do lets sleep a while
#ifdef UNIX
      usleep( IDLE_SLEEP_MS*1000 );
#else
      boost::this_thread::sleep(boost::posix_time::milliseconds( IDLE_SLEEP_MS ));
#endif
      continue;
    }

    if(av_read_frame(formatCtx_, &packet) < 0) {
      // end of stream reached
      if(repeatStream_) {
        seekToBegin();
        av_free_packet(&packet);
      } else {
        stop();
        completed_ = GL_TRUE;
      }
      continue;
    }

    demuxer_->decode(&packet);
  }
}

void VideoTexture::seekToBegin()
{
  seekTo(0.0);
}
void VideoTexture::seekForward(GLdouble seconds)
{
  seekTo((elapsedSeconds() + seconds)/totalSeconds());
}
void VideoTexture::seekBackward(GLdouble seconds)
{
  seekTo((elapsedSeconds() - seconds)/totalSeconds());
}
void VideoTexture::seekTo(GLdouble p)
{
  if(!isFileSet()) { return; }
  p = max(0.0, min(1.0, p));
  seek_.isRequired = GL_TRUE;
  seek_.flags &= ~AVSEEK_FLAG_BYTE;
  seek_.rel = 0;
  seek_.pos = p * formatCtx_->duration;
  if(formatCtx_->start_time != AV_NOPTS_VALUE) {
    seek_.pos += formatCtx_->start_time;
  }
  textureUpdater_->elapsedSeconds_ = p*totalSeconds();
}

void VideoTexture::set_repeat(bool repeat)
{
  boost::lock_guard<boost::mutex> lock(decodingLock_);
  repeatStream_ = repeat;
}
bool VideoTexture::repeat() const
{
  return repeatStream_;
}

void VideoTexture::play()
{
  boost::lock_guard<boost::mutex> lock(decodingLock_);
  pauseFlag_ = GL_FALSE;
  completed_ = GL_FALSE;
}

GLboolean VideoTexture::isFileSet() const
{
  return demuxer_.get() != NULL;
}
GLboolean VideoTexture::isPlaying() const
{
  return !pauseFlag_;
}
GLboolean VideoTexture::isCompleted() const
{
  return completed_;
}

void VideoTexture::togglePlay()
{
  if(pauseFlag_) {
    play();
  } else {
    pause();
  }
}

void VideoTexture::pause()
{
  boost::lock_guard<boost::mutex> lock(decodingLock_);
  pauseFlag_ = true;
  if(demuxer_->audioStream()) {
    demuxer_->audioStream()->audioSource()->pause();
  }
  clearQueue();
}

void VideoTexture::stop()
{
  clearQueue();
  seekToBegin();
  pause();
}

void VideoTexture::stopDecodingThread()
{
  // stop VideoTexture::decode()
  closeFlag_ = GL_TRUE;
  if(demuxer_.get()) {
    // stop waiting for pushing frames
    if(demuxer_->videoStream()!=NULL) { demuxer_->videoStream()->setInactive(); }
    if(demuxer_->audioStream()!=NULL) { demuxer_->audioStream()->setInactive(); }
  }
  // remove texture updater animation
  if(textureUpdater_.get()) {
    //TimeoutManager::get().removeTimeout(textureUpdater_.get());
    AnimationManager::get().removeAnimation(ref_ptr<Animation>::cast(textureUpdater_));
  }
  // finally wait for the decoding thread to stop
  decodingThread_.join();
}

void VideoTexture::set_file(const string &file)
{
  // exit decoding thread
  stopDecodingThread();

  // (re)open video file
  if(formatCtx_) {
    avformat_close_input(&formatCtx_);
    formatCtx_ = NULL;
  }
  if(avformat_open_input(&formatCtx_, file.c_str(), NULL, NULL) != 0)
  {
    throw new Error("Couldn't open file");
  }

  demuxer_ = ref_ptr<Demuxer>::manage( new Demuxer(formatCtx_) );
  // if there is a video stream create a texture
  VideoStream *vs = demuxer_->videoStream();
  if(vs) { // setup the texture target
    set_size(vs->width(), vs->height());
    set_internalFormat(vs->texInternalFormat());
    set_format(vs->texFormat());
    set_pixelType(vs->texPixelType());
    bind();
    set_data(NULL);
    texImage();
    set_filter(GL_LINEAR, GL_LINEAR);
    set_wrapping(GL_REPEAT);

    // update texture in timeout
    textureUpdater_ = ref_ptr<VideoTextureUpdater>::manage(
        new VideoTextureUpdater(vs, demuxer_->audioStream(), this) );
    //TimeoutManager::get().addTimeout( textureUpdater_.get() );
    AnimationManager::get().addAnimation( ref_ptr<Animation>::cast(textureUpdater_) );
  }

  // start decoding
  closeFlag_ = GL_FALSE;
  decodingThread_ = boost::thread(&VideoTexture::decode, this);
}

