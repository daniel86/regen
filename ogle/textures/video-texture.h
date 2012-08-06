/*
 * video.h
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#ifndef VIDEO_H_
#define VIDEO_H_

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <ogle/gl-types/texture.h>
#include <ogle/av/demuxer.h>

class VideoError : public runtime_error
{
public:
  VideoError(const string &message)
  : runtime_error(message)
  {
  }
};
class VideoTextureUpdater;

class VideoTexture : public Texture2D
{
public:
  VideoTexture();
  ~VideoTexture();

  /**
   * Starts playing the media.
   */
  void play();
  /**
   * Pauses playing the media.
   */
  void pause();
  /**
   * Stops playing the media.
   */
  void stop();

  /**
   * Stream file at given filesystem path.
   */
  void set_file(const string &file);

  /**
   * Repeat video of end position reached ?
   */
  void set_repeat(bool repeat);
  bool repeat() const;

  /**
   * The audio source of this media.
   */
  ref_ptr<AudioSource> audioSource();

protected:
  ref_ptr<Demuxer> demuxer_;
  AVFormatContext *formatCtx_;

  boost::thread decodingThread_;
  boost::mutex decodingLock_;

  ref_ptr<VideoTextureUpdater> textureUpdater_;

  bool repeatStream_;
  bool closeFlag_;
  bool pauseFlag_;
  bool seekToBeginFlag_;

  void decode();
  void seekToBegin();

private:
  static bool initialled_;
};

#endif /* VIDEO_H_ */
