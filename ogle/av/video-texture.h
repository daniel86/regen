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
 * Decoding is done using libav in a separate thread.
 */
class VideoTexture : public Texture2D
{
public:
  VideoTexture();
  ~VideoTexture();

  /**
   * Was a video file set yet ?
   */
  GLboolean isFileSet() const;
  /**
   * Is the video currently decoding ?
   */
  GLboolean isPlaying() const;
  /**
   * EOF reached while playing the video.
   */
  GLboolean isCompleted() const;

  /**
   * Total number of seconds elapsed in the video.
   */
  GLfloat elapsedSeconds() const;
  /**
   * Total number of seconds of currently loaded video.
   */
  GLfloat totalSeconds() const;

  /**
   * Toggles between play and pause.
   */
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

  /**
   * Seek to initial frame.
   */
  void seekToBegin();
  /**
   * Seek to given position [0,1]
   */
  void seekTo(GLdouble p);
  /**
   * Seek forward given amount of seconds.
   */
  void seekForward(GLdouble seconds);
  /**
   * Seek backward given amount of seconds.
   */
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
   * The audio source of this media (maybe a null reference).
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
  GLboolean completed_;

  struct SeekPosition {
    bool isRequired;
    int flags;
    int64_t pos;
    int64_t rel;
  }seek_;

  void decode();
  void clearQueue();
  void stopDecodingThread();

private:
  static GLboolean initialled_;
};

#endif /* VIDEO_TEXTURE_H_ */
