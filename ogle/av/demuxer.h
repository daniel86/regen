/*
 * demuxer.h
 *
 *  Created on: 10.04.2012
 *      Author: daniel
 */

#ifndef DEMUXER_H_
#define DEMUXER_H_

#include <ogle/av/video-stream.h>
#include <ogle/av/audio-stream.h>

namespace ogle {
/**
 * \brief libav stream demuxer.
 *
 * Manages passing packets to video/audio streams for further processing.
 * \note Only a single video/audio channel is handled by the Demuxer.
 */
class Demuxer
{
public:
  /**
   * \brief An error occurred during demuxing.
   */
  class Error : public runtime_error {
  public:
    Error(const string &message) : runtime_error(message) {}
  };

  Demuxer(AVFormatContext *formatCtx);

  /**
   * The video stream or NULL.
   */
  VideoStream* videoStream();
  /**
   * The audio stream or NULL.
   */
  AudioStream* audioStream();

  /**
   * Decodes a single av packet.
   */
  void decode(AVPacket *packet);

protected:
  AVFormatContext *formatCtx_;

  ref_ptr<VideoStream> videoStream_;
  ref_ptr<AudioStream> audioStream_;

  void init();
};

} // end ogle namespace

#endif /* DEMUXER_H_ */
