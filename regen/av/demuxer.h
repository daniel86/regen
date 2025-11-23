#ifndef REGEN_DEMUXER_H_
#define REGEN_DEMUXER_H_

#include <regen/av/audio.h>
#include <regen/av/video-stream.h>
#include <regen/utility/ref-ptr.h>

namespace regen {
	/**
	 * \brief ffmpeg stream demuxer.
	 *
	 * Manages passing packets to video/audio streams for further processing.
	 * @note Only a single video/audio channel is handled by the Demuxer.
	 */
	class Demuxer {
	public:
		/**
		 * \brief An error occurred during demuxing.
		 */
		class Error : public std::runtime_error {
		public:
			/**
			 * @param message the error message.
			 */
			explicit Error(const std::string &message) : std::runtime_error(message) {}
		};

		/**
		 * Setup ffmpeg.
		 */
		static void initAVLibrary();

		Demuxer();

		/**
		 * @param file Stream file at given path.
		 */
		explicit Demuxer(std::string_view file);

		~Demuxer();

		/**
		 * Stream file at given path.
		 */
		void set_file(std::string_view file);

		/**
		 * Is the stream currently decoding ?
		 */
		bool isPlaying() const;

		/**
		 * @return true if the demuxer has an attached input file.
		 */
		bool hasInput() const;

		/**
		 * Total number of seconds elapsed in the stream.
		 */
		float elapsedSeconds() const;

		/**
		 * Total number of seconds of currently loaded stream.
		 */
		float totalSeconds() const;

		/**
		 * Repeat video of end position reached ?
		 */
		void set_repeat(bool repeat);

		/**
		 * Repeat video of end position reached ?
		 */
		bool repeat() const;

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
		 * The stream may block in decode() waiting to be able
		 * to push a frame onto the queue that is full.
		 * Calling setInactive() will make sure that the stream
		 * drops out the block so that other media can be loaded.
		 */
		void setInactive();

		/**
		 * Decodes a single av packet.
		 */
		bool decode();

		/**
		 * Seek to given position [0,1]
		 */
		void seekTo(double p);

		/**
		 * The video stream or NULL.
		 */
		auto videoStream() { return videoStream_; }

		/**
		 * The audio stream or NULL.
		 */
		auto audioStream() { return audioStream_; }

	protected:
		AVFormatContext *formatCtx_;

		ref_ptr<VideoStream> videoStream_;
		ref_ptr<AudioSource> audioStream_;

		bool pauseFlag_;
		bool repeatStream_;

		int videoStreamIndex_;
		int audioStreamIndex_;

		//float elapsedSeconds_;

		struct SeekPosition {
			bool isRequired;
			int flags;
			int64_t pos;
			int64_t rel;
		} seek_;
		bool seeked_;

		void clearQueue();

	private:
		static bool initialled_;
	};
} // namespace

#endif /* REGEN_DEMUXER_H_ */
