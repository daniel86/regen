#ifndef REGEN_VIDEO_TEXTURE_H_
#define REGEN_VIDEO_TEXTURE_H_

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <regen/textures/texture.h>
#include <regen/gl-types/render-state.h>
#include <regen/av/demuxer.h>
#include <regen/animation/animation.h>

namespace regen {
	/**
	 * \brief A texture that displays a video.
	 *
	 * The texture pixel data is updated using a video stream.
	 * Decoding is done using libav in a separate thread.
	 */
	class VideoTexture : public Texture2D, public Animation {
	public:
		/**
		 * \brief An error occurred during video processing.
		 */
		class Error : public std::runtime_error {
		public:
			/**
			 * @param message the error message.
			 */
			explicit Error(const std::string &message) : std::runtime_error(message) {}
		};

		VideoTexture();

		~VideoTexture() override;

		/**
		 * @return seconds processed in stream.
		 */
		auto elapsedSeconds() const { return elapsedSeconds_; }

		/**
		 * Stream file at given path.
		 */
		void set_file(std::string_view file);

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
		void seekTo(double p);

		/**
		 * Seek forward given amount of seconds.
		 */
		void seekForward(double seconds);

		/**
		 * Seek backward given amount of seconds.
		 */
		void seekBackward(double seconds);

		/**
		 * @return the demuxer used for decoding packets.
		 */
		auto &demuxer() const { return demuxer_; }

		/**
		 * The audio source of this media (maybe a null reference).
		 */
		ref_ptr<AudioSource> audioSource();

		// override
		void cpuUpdate(double dt) override;

		void gpuUpdate(RenderState *rs, double dt) override;

	protected:
		ref_ptr<Demuxer> demuxer_;

		boost::thread decodingThread_;
		boost::mutex decodingLock_;
		boost::mutex textureUpdateLock_;
		bool closeFlag_;
		bool seeked_;
		bool fileToLoaded_;

		float elapsedSeconds_;

		ref_ptr<VideoStream> vs_;
		ref_ptr<AudioSource> as_;
		double idleInterval_;
		double interval_;
		double dt_;
		int64_t intervalMili_;
		AVFrame *lastFrame_;
		byte *videoImageData_ = nullptr;

		void decode();

		void stopDecodingThread();
	};
} // namespace

#endif /* REGEN_VIDEO_TEXTURE_H_ */
