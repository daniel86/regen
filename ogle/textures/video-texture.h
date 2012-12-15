/*
 * video-texture.h
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#ifndef VIDEO_TEXTURE_H_
#define VIDEO_TEXTURE_H_

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
// forward declaration
class VideoTextureUpdater;

/**
 * A texture that updates the pixel data using
 * a video stream.
 */
class VideoTexture : public Texture2D
{
public:
  VideoTexture();
  ~VideoTexture();

  void togglePlay();
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

  void seekToBegin();
  void seekForward(GLdouble seconds);
  void seekBackward(GLdouble seconds);

  /**
   * Stream file at given path.
   */
  void set_file(const string &file);

  /**
   * Repeat video of end position reached ?
   */
  void set_repeat(bool repeat);
  /**
   * Repeat video of end position reached ?
   */
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

  GLboolean repeatStream_;
  GLboolean closeFlag_;
  GLboolean pauseFlag_;

  struct SeekPosition {
    bool isRequired;
    int flags;
    int64_t pos;
  }seek_;

  void decode();

private:
  static GLboolean initialled_;
};

#endif /* VIDEO_TEXTURE_H_ */
