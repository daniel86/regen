/*
 * video.cpp
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#include <stdexcept>
#include <string>
using namespace std;

extern "C" {
  #include <libavformat/avformat.h>
}

#include <ogle/utility/timeout-manager.h>
#include <ogle/animations/animation-manager.h>
#include <ogle/animations/animation.h>

#include "video-texture.h"

// Milliseconds to sleep per loop in idle mode.
#define IDLE_SLEEP_MS 400

class VideoTextureUpdater : public Timeout, public Animation
{
public:
  Texture2D *tex_;
  VideoStream *vs_;
  AudioStream *as_;
  boost::posix_time::time_duration interval_;
  boost::posix_time::time_duration idleInterval_;
  boost::mutex textureUpdateLock_;
  AVFrame *lastFrame_;
  bool textureUpdated_;
  bool idle_;
  float timeFactor_;
  boost::int64_t intervalMili_;
  float sumSecs_;
  int count_;
  float lastDT_;

  VideoTextureUpdater(
      VideoStream *vs,
      AudioStream *as,
      Texture2D *tex)
  : Timeout(),
    Animation(),
    tex_(tex),
    vs_(vs),
    as_(as),
    idle_(true),
    lastFrame_(NULL),
    count_(0),
    sumSecs_(-1.0f)
  {
    idleInterval_ = boost::posix_time::time_duration(
        boost::posix_time::microseconds(IDLE_SLEEP_MS*1000.0));
    interval_ = idleInterval_;
  }
  ~VideoTextureUpdater()
  {
    if(lastFrame_) {
      av_free(lastFrame_->data[0]);
      av_free(lastFrame_);
    }
  }

  const boost::posix_time::time_duration interval() const
  {
    return interval_;
  }
  void doCallback(const boost::int64_t &milliSeconds)
  {
    AVFrame *frame;
    boost::int64_t diff = (milliSeconds - intervalMili_);
    if(idle_) {
      diff = 0;
    }

    unsigned int numFrames = vs_->numFrames();

    // no frames there to show
    if(numFrames == 0) {
      interval_ = idleInterval_;
      idle_ = true;
      //return;
    } else {
      idle_ = false;
    }

    if(!idle_) {
      if(lastFrame_) {
        av_free(lastFrame_->data[0]);
        av_free(lastFrame_);
      }

      // pop the first frame dropping some frames
      // if we are not fast enough showing frames
      AVFrame *droppedFrame = NULL;
      do {
        if(droppedFrame) {
          av_free(droppedFrame->data[0]);
          av_free(droppedFrame);
          droppedFrame = NULL;
        }
        frame = vs_->frontFrame();
        droppedFrame = frame;
        vs_->popFrame();
        diff -= intervalMili_;
      } while(diff > intervalMili_ && vs_->numFrames()>2);
      diff += intervalMili_;

      // set data on gl texture
      tex_->set_data( frame->data[0] );
      // queue calling texImage
      {
        boost::lock_guard<boost::mutex> lock(textureUpdateLock_);
        textureUpdated_ = true;
      }

      // set next interval
      float *t = (float*) frame->opaque;
      if((*t) < lastDT_) {
        // reset synchronization var
        sumSecs_ = -1.0f;
      } else {
        // set timeout interval to time difference to last frame plus a correction
        // value because the last timeout call was not exactly the wanted interval
        float dt = (*t)-lastDT_;
        intervalMili_ = dt*1000 - diff;
      }
      lastDT_ = *t;
      delete t;

      lastFrame_ = frame;
    } else {
      lastDT_ += milliSeconds/1000.0f;
    }
    interval_ = boost::posix_time::time_duration(
          boost::posix_time::microseconds(intervalMili_*1000.0));

    // synchronize with OpenAL
    if(!idle_ && as_) {
      ref_ptr<AudioSource> as = as_->audioSource();
      as->set_secOffset(sumSecs_);

      if(sumSecs_ < 0.0f) {
        sumSecs_ = 0.0f;
      } else {
        sumSecs_ += milliSeconds/1000.0f;
      }
    }
  }

  void updateAnimationGraphics(const double &dt)
  {
    {
      boost::lock_guard<boost::mutex> lock(textureUpdateLock_);
      if(!textureUpdated_) return;
      textureUpdated_ = false;
    }

    // upload texture data to gl
    tex_->bind();
    tex_->texImage();
    tex_->setupMipmaps(GL_DONT_CARE);
  }
  void doAnimate(const double &milliSeconds) {}
};


bool VideoTexture::initialled_ = false;

VideoTexture::VideoTexture()
: Texture2D(1),
  formatCtx_(NULL),
  decodingThread_( boost::thread(&VideoTexture::decode, this) ),
  closeFlag_( false ),
  pauseFlag_( true ),
  repeatStream_( false ),
  seekToBeginFlag_( false )
{
  if(!initialled_) {
    av_register_all();
    av_log_set_level(AV_LOG_ERROR);
    initialled_ = true;
  }
}
VideoTexture::~VideoTexture()
{
  if( textureUpdater_.get() != NULL ) {
    TimeoutManager::get().removeTimeout( textureUpdater_.get() );
    AnimationManager::get().removeAnimation( textureUpdater_ );
  }

  decodingLock_.lock(); {
    closeFlag_ = true;
  } decodingLock_.unlock();
  // wait until thread exited
  decodingThread_.join();

  // Close the video file
  if(formatCtx_) avformat_close_input(&formatCtx_);
}

ref_ptr<AudioSource> VideoTexture::audioSource()
{
  if(demuxer_->audioStream()) {
    return demuxer_->audioStream()->audioSource();
  } else {
    return ref_ptr<AudioSource>();
  }
}

void VideoTexture::decode()
{
  AVPacket packet;
  int readStatus;
  bool closed, paused, seekToBeginFlag;

  while( true ) {
    {
      boost::lock_guard<boost::mutex> lock(decodingLock_);
      closed = closeFlag_;
      paused = pauseFlag_;
      seekToBeginFlag = seekToBeginFlag_;
      if(seekToBeginFlag) seekToBeginFlag_ = false;
    }
    if(closed) {
      break;
    }

    if(seekToBeginFlag) {
      seekToBegin();
    } else if(paused) {
      // demuxer has nothing to do lets sleep a while
      boost::this_thread::sleep(boost::posix_time::milliseconds( IDLE_SLEEP_MS ));
      continue;
    }

    if(av_read_frame(formatCtx_, &packet) < 0) {
      // end of stream reached
      if(repeatStream_) {
        seekToBegin();
        av_free_packet(&packet);
      } else {
        stop();
      }
      continue;
    }

    demuxer_->decode(&packet);
  }
}

void VideoTexture::seekToBegin()
{
  av_seek_frame(formatCtx_, -1, 0, AVSEEK_FLAG_ANY);
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
  pauseFlag_ = false;
}

void VideoTexture::pause()
{
  boost::lock_guard<boost::mutex> lock(decodingLock_);
  pauseFlag_ = true;
  AudioStream *as = demuxer_->audioStream();
  if(as) {
    as->audioSource()->pause();
    while(as->audioSource()->state() == AL_PLAYING) {
      boost::this_thread::sleep(boost::posix_time::milliseconds( 40 ));
    }
  }
}

void VideoTexture::stop()
{
  {
    boost::lock_guard<boost::mutex> lock(decodingLock_);
    seekToBeginFlag_ = true;
  }
  pause();
}

void VideoTexture::set_file(const string &file)
{
  // Open video file
  if(avformat_open_input(&formatCtx_, file.c_str(), NULL, NULL) != 0)
  {
    throw new VideoError("Couldn't open file");
  }

  demuxer_ = ref_ptr<Demuxer>::manage( new Demuxer(formatCtx_) );
  // if there is a video stream create a texture
  VideoStream *vs = demuxer_->videoStream();
  if(vs) { // setup the texture target
    set_size(vs->width(), vs->height());
    set_internalFormat( vs->texInternalFormat() );
    set_format( vs->texFormat() );
    set_pixelType( vs->texPixelType() );
    bind();
    set_wrapping( GL_REPEAT );
    set_data( NULL );
    texImage();
    set_filter(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
    setupMipmaps(GL_DONT_CARE);

    pauseFlag_ = false;
    // get first video frame
    while( vs->numFrames() < 1 );
    {
      boost::lock_guard<boost::mutex> lock(decodingLock_);
      pauseFlag_ = true;
    }

    // update texture in timeout
    textureUpdater_ = ref_ptr<VideoTextureUpdater>::manage(
        new VideoTextureUpdater(vs, demuxer_->audioStream(), this) );
    TimeoutManager::get().addTimeout( textureUpdater_.get(), true );
    AnimationManager::get().addAnimation( textureUpdater_ );
  }
}

