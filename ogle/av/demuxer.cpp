/*
 * demuxer.cpp
 *
 *  Created on: 10.04.2012
 *      Author: daniel
 */

#include <ogle/utility/logging.h>

#include "demuxer.h"
using namespace ogle;

GLboolean Demuxer::initialled_ = GL_FALSE;

#if LIBAVFORMAT_VERSION_MAJOR>53
#define __CLOSE_INPUT__ avformat_close_input
#else
#define __CLOSE_INPUT__ av_close_input_file
#endif

void Demuxer::initAVLibrary()
{
  if(!initialled_) {
    av_register_all();
    //av_log_set_level(AV_LOG_ERROR);
    av_log_set_level(AV_LOG_VERBOSE);
    initialled_ = GL_TRUE;
  }
}

Demuxer::Demuxer(const string &file)
: formatCtx_(NULL), pauseFlag_(GL_TRUE), repeatStream_(GL_FALSE)
{
  initAVLibrary();

  seeked_ = GL_FALSE;
  seek_.isRequired = GL_FALSE;

  set_file(file);
}
Demuxer::Demuxer()
: formatCtx_(NULL), pauseFlag_(GL_TRUE), repeatStream_(GL_FALSE)
{
  initAVLibrary();

  seeked_ = GL_FALSE;
  seek_.isRequired = GL_FALSE;
}
Demuxer::~Demuxer()
{
  // Close the video file
  if(formatCtx_) __CLOSE_INPUT__(&formatCtx_);
}

void Demuxer::set_repeat(GLboolean repeat)
{
  repeatStream_ = repeat;
}
GLboolean Demuxer::repeat() const
{
  return repeatStream_;
}

GLfloat Demuxer::totalSeconds() const
{
  return formatCtx_ ? (formatCtx_->duration/(GLdouble)AV_TIME_BASE) : 0.0f;
}

GLboolean Demuxer::isPlaying() const
{
  return !pauseFlag_;
}
GLboolean Demuxer::hasInput() const
{
  return formatCtx_!=NULL;
}

void Demuxer::set_file(const string &file)
{
  // (re)open file
  if(formatCtx_) {
    __CLOSE_INPUT__(&formatCtx_);
    formatCtx_ = NULL;
  }
  if(avformat_open_input(&formatCtx_, file.c_str(), NULL, NULL) != 0)
  {
    throw new Error("Couldn't open file");
  }

  // Retrieve stream information
  if(avformat_find_stream_info(formatCtx_, NULL)<0) {
    throw new Error("Couldn't find stream information");
  }

  // Find the first video/audio stream
  int videoIndex_ = -1;
  int audioIndex_ = -1;
  for(unsigned int i=0; i<formatCtx_->nb_streams; ++i)
  {
    if(videoIndex_==-1 &&
        formatCtx_->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      videoIndex_ = i;
    }
    else if(audioIndex_==-1 &&
        formatCtx_->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
    {
      audioIndex_ = i;
    }
  }
  if(videoIndex_ != -1) {
    videoStream_ = ref_ptr<VideoStream>::manage(
        new VideoStream(
            formatCtx_->streams[videoIndex_],
            videoIndex_,
            100));
  }
  if(audioIndex_ != -1) {
    audioStream_ = ref_ptr<AudioStream>::manage(
        new AudioStream(
            formatCtx_->streams[audioIndex_],
            audioIndex_,
            -1));
  }
}

void Demuxer::play()
{
  pauseFlag_ = GL_FALSE;
}
void Demuxer::pause()
{
  pauseFlag_ = GL_TRUE;
  if(audioStream_.get()) {
    audioStream_->audioSource()->pause();
  }
  clearQueue();
}
void Demuxer::togglePlay()
{
  if(pauseFlag_)
  { play(); }
  else
  { pause(); }
}
void Demuxer::stop()
{
  clearQueue();
  seekTo(0.0);
  pause();
}

void Demuxer::clearQueue()
{
  if(videoStream_.get()) {
    videoStream_->clearQueue();
    avcodec_flush_buffers(videoStream_->codec());
  }
  if(audioStream_.get()) {
    audioStream_->clearQueue();
    avcodec_flush_buffers(audioStream_->codec());
  }
}

void Demuxer::setInactive()
{
  if(videoStream_.get()) { videoStream_->setInactive(); }
  if(audioStream_.get()) { audioStream_->setInactive(); }
}

void Demuxer::seekTo(GLdouble p)
{
  if(!formatCtx_) { return; }
  p = max(0.0, min(1.0, p));
  seek_.isRequired = GL_TRUE;
  seek_.flags &= ~AVSEEK_FLAG_BYTE;
  seek_.rel = 0;
  seek_.pos = p * formatCtx_->duration;
  if(formatCtx_->start_time != AV_NOPTS_VALUE) {
    seek_.pos += formatCtx_->start_time;
  }
}

GLboolean Demuxer::decode()
{
  AVPacket packet;

  SeekPosition seek = seek_;
  seek_.isRequired = GL_FALSE;
  if(seek.isRequired) {
    int64_t seek_min = seek.rel > 0 ? seek.pos - seek.rel + 2: INT64_MIN;
    int64_t seek_max = seek.rel < 0 ? seek.pos - seek.rel - 2: INT64_MAX;

    int ret = avformat_seek_file(formatCtx_,
        -1, seek_min, seek.pos, seek_max, seek.flags);
    if (ret < 0) {
      ERROR_LOG("seeking failed");
    }
    else {
      clearQueue();
      seeked_ = GL_TRUE;
    }
  }
  else if(pauseFlag_) {
    return GL_TRUE;
  }

  if(av_read_frame(formatCtx_, &packet) < 0) {
    // end of stream reached
    if(repeatStream_) {
      seekTo(0.0);
      av_free_packet(&packet);
      return GL_FALSE;
    } else {
      stop();
      return GL_TRUE;
    }
  }

  // Is this a packet from the video stream?
  if(packet.stream_index == videoStream_->index())
  {
    videoStream_->decode(&packet);
  }
  else if (packet.stream_index == audioStream_->index())
  {
    audioStream_->decode(&packet);
  }
  else
  {
    // Free the packet that was allocated by av_read_frame
    av_free_packet(&packet);
  }

  return GL_FALSE;
}

VideoStream* Demuxer::videoStream()
{ return videoStream_.get(); }
AudioStream* Demuxer::audioStream()
{ return audioStream_.get(); }
