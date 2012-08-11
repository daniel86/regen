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

class DemuxerError : public runtime_error {
public:
  DemuxerError(const string &message)
  : runtime_error(message)
  {
  }
};

/**
 * Manages passing packages to video/audio streams for further processing.
 * Only a single video/audio channel will be handled by this Demuxer.
 */
class Demuxer
{
public:
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

#endif /* DEMUXER_H_ */
