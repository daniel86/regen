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
#include <ogle/gl-types/render-state.h>
#include <ogle/av/demuxer.h>
#include <ogle/animations/animation.h>

namespace ogle {
/**
 * \brief A texture that displays a video.
 *
 * The texture pixel data is updated using a video stream.
 * Decoding is done using libav in a separate thread.
 */
class VideoTexture : public Texture2D, public Animation
{
public:
  /**
   * \brief An error occurred during video processing.
   */
  class Error : public runtime_error
  {
  public:
    /**
     * @param message the error message.
     */
    Error(const string &message) : runtime_error(message) {}
  };

  VideoTexture();
  ~VideoTexture();

  /**
   * @return seconds processed in stream.
   */
  GLfloat elapsedSeconds() const;

  /**
   * Stream file at given path.
   */
  void set_file(const string &file);

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
   * @return the demuxer used for decoding packets.
   */
  const ref_ptr<Demuxer>& demuxer() const;

  /**
   * The audio source of this media (maybe a null reference).
   */
  ref_ptr<AudioSource> audioSource();

  // override
  void animate(GLdouble dt);
  void glAnimate(RenderState *rs, GLdouble dt);

protected:
  ref_ptr<Demuxer> demuxer_;

  boost::thread decodingThread_;
  boost::mutex decodingLock_;
  boost::mutex textureUpdateLock_;
  GLboolean closeFlag_;
  GLboolean seeked_;

  GLfloat elapsedSeconds_;

  VideoStream *vs_;
  AudioStream *as_;
  GLdouble interval_;
  GLdouble idleInterval_;
  GLdouble dt_;
  boost::int64_t intervalMili_;
  AVFrame *lastFrame_;

  void decode();
  void stopDecodingThread();
};

} // end ogle namespace

#endif /* VIDEO_TEXTURE_H_ */
