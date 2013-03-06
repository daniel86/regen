/*
 * demuxer.cpp
 *
 *  Created on: 10.04.2012
 *      Author: daniel
 */

#include "demuxer.h"
using namespace ogle;

Demuxer::Demuxer(AVFormatContext *formatCtx)
: formatCtx_(formatCtx)
{
  // Retrieve stream information
  if(avformat_find_stream_info(formatCtx_, NULL)<0) {
    throw new DemuxerError("Couldn't find stream information");
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

VideoStream* Demuxer::videoStream()
{
  return videoStream_.get();
}
AudioStream* Demuxer::audioStream()
{
  return audioStream_.get();
}

void Demuxer::decode(AVPacket *packet)
{
  // Is this a packet from the video stream?
  if(packet->stream_index == videoStream_->index())
  {
    videoStream_->decode(packet);
  }
  else if (packet->stream_index == audioStream_->index())
  {
    audioStream_->decode(packet);
  }
  else
  {
    // Free the packet that was allocated by av_read_frame
    av_free_packet(packet);
  }
}
