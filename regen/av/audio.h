/*
 * audio-source.h
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#ifndef AUDIO_SOURCE_H_
#define AUDIO_SOURCE_H_

#include <regen/config.h>

extern "C" {
  #include <libavcodec/version.h>
  #include <libavcodec/avcodec.h>
#ifdef HAS_LIBAVRESAMPLE
  #include <libavresample/avresample.h>
  #include <libavutil/opt.h>
#endif
}
#include <AL/al.h>
#include <AL/alc.h>

#include <regen/av/av-stream.h>
#include <regen/math/vector.h>
#include <regen/utility/ref-ptr.h>
#include <regen/utility/stack.h>

namespace regen {
  /**
   * \brief Global audio state.
   */
  class AudioLibrary : public AudioVideoStream
  {
  public:
    /**
     * Initialize the audio library.
     */
    static void initializeAL();
    /**
     * Can be called on application cleanup.
     */
    static void closeAL();
    /**
     * OpenAL distance model. AL_INVERSE_DISTANCE,
     * AL_INVERSE_DISTANCE_CLAMPED, AL_LINEAR_DISTANCE, AL_LINEAR_DISTANCE_CLAMPED,
     * AL_EXPONENT_DISTANCE, AL_EXPONENT_DISTANCE_CLAMPED, or AL_NONE.
     */
    static void set_distanceModel(ALenum v);
    /**
     * Doppler factor (default 1.0)
     */
    static void set_dopplerFactor(ALfloat v);
    /**
     * Speed of sound (default value 343.3)
     */
    static void set_speedOfSound(ALfloat v);
  protected:
    static ALboolean isALInitialized_;
  };
}
namespace regen {
  /**
   * \brief Configures the audio listener.
   */
  class AudioListener : public AudioVideoStream
  {
  public:
    /** Set Listener parameter. */
    static void set1f(const ALenum &p, const ALfloat &v);
    /** Get Listener parameter. */
    static ALfloat get1f(const ALenum &p);

    /** Set Listener parameter. */
    static void set3f(const ALenum &p, const Vec3f &v);
    /** Get Listener parameter. */
    static Vec3f get3f(const ALenum &p);

    /** Set Listener parameter. */
    static void set6f(const ALenum &p, const Vec6f &v);
    /** Get Listener parameter. */
    static Vec6f get6f(const ALenum &p);
  };
}
namespace regen {
  /**
   * \brief Source of audio in 3D space.
   */
  class AudioSource : public AudioVideoStream
  {
  public:
    /**
     * \brief Provides audio data.
     */
    struct AudioBuffer {
      AudioBuffer();
      ~AudioBuffer();
      /** AL id. */
      ALuint id_;
    };

    /**
     * @param chachedBytesLimit limit for pre-loading.
     */
    AudioSource(GLuint chachedBytesLimit);
    /**
     * @param stream a av stream handle.
     * @param index index in stream.
     * @param chachedBytesLimit limit for pre-loading.
     */
    AudioSource(AVStream *stream, GLint index, GLuint chachedBytesLimit);
    virtual ~AudioSource();

    /**
     * The audio source ID.
     */
    ALuint id() const;

    /**
     * @return elapsed time of last finished audio frame.
     */
    GLdouble elapsedTime() const;

    /** Set Source parameter. */
    void set1i(const ALenum &p, const ALint &v) const;
    /** Get Source parameter. */
    ALint get1i(const ALenum &p) const;

    /** Set Source parameter. */
    void set1f(const ALenum &p, const ALfloat &v);
    /** Get Source parameter. */
    ALfloat get1f(const ALenum &p) const;

    /** Set Source parameter. */
    void set3f(const ALenum &p, const Vec3f &v) const;
    /** Get Source parameter. */
    Vec3f get3f(const ALenum &p);

    /**
     * Start playing.
     */
    void play() const;
    /**
     * Stop playing.
     */
    void stop() const;
    /**
     * Pause playing.
     */
    void pause() const;
    /**
     * Rewind to start position.
     */
    void rewind() const;

    /**
     * This function queues a set of buffers on a source.
     * All buffers attached to a source will be played in sequence.
     */
    void push(const ref_ptr<AudioBuffer> &buffer);
    /**
     * Unqueues buffer that was queued previously.
     */
    void pop();

    /**
     * Opens libav stream.
     * Make sure to call this before decoding AVPacket's.
     * @param stream the libav stream.
     * @param index the stream index.
     * @param initial flag indicating if the open call comes from constructor.
     */
    void openAudioStream(AVStream *stream,
        GLint index, GLboolean initial=GL_FALSE);

    // override
    virtual void decode(AVPacket *packet);
    virtual void clearQueue();

  protected:
    struct AudioFrame {
      ref_ptr<AudioBuffer> buffer;
      AVFrame *avFrame;
      ALbyte *convertedFrame;
      GLdouble dts;
      void free();
    };

    ALuint id_;
    ALenum alType_;
    ALenum alChannelLayout_;
    ALenum alFormat_;
    ALint rate_;
    GLdouble elapsedTime_;

    Stack< ref_ptr<AudioBuffer> > queued_;
#ifdef HAS_LIBAVRESAMPLE
    struct AVAudioResampleContext *resampleContext_;
#endif

  private:
    AudioSource(const AudioSource&);
    AudioSource& operator=(const AudioSource&);
  };
} // namespace

#endif /* AUDIO_SOURCE_H_ */
