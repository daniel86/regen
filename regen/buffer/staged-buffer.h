#ifndef REGEN_STAGED_BUFFER_H_
#define REGEN_STAGED_BUFFER_H_

#include "buffer-object.h"
#include "regen/scene/scene-input.h"
#include "regen/utility/threading.h"
#include "staging-buffer.h"

namespace regen {
	/**
	 * \brief A buffer object that can be used for staging data in a staging system.
	 *
	 * StagedBuffer is a base class for buffer objects that can be used in a staging system.
	 * It provides functionality to manage staged inputs, update dirty segments, and copy data
	 * to the GPU.
	 */
	class StagedBuffer : public BufferObject, public ShaderInput {
	public:
		// The minimum number of segments for partial updates in temporary mapped buffers.
		static uint32_t MIN_SEGMENTS_PARTIAL_TEMPORARY;
		// The maximum update ratio for partial updates in temporary mapped buffers.
		static float MAX_UPDATE_RATIO_PARTIAL_TEMPORARY;
		// Frame range for update detection
		static uint32_t UPDATE_RATE_RANGE;

		/**
		 * Create a staged buffer.
		 * @param name the name of the buffer.
		 * @param target the buffer target.
		 * @param hints the buffer update hints.
		 * @param memoryLayout the memory layout.
		 */
		StagedBuffer(const std::string &name,
				BufferTarget target,
				const BufferUpdateFlags &hints,
				BufferMemoryLayout memoryLayout);

		/**
		 * Copy constructor. Does not copy GPU data, both objects will share the same buffer.
		 */
		StagedBuffer(const StagedBuffer &other, const std::string &name = "");

		~StagedBuffer() override;

		/**
		 * Add a uniform to the staged buffer.
		 * @param input the shader input.
		 */
		void addStagedInput(const ref_ptr<ShaderInput> &input, const std::string &name = "");

		/**
		 * Remove an input by name.
		 * @param name the name of the block input to remove.
		 */
		void removeStagedInput(std::string_view name);

		/**
		 * @return a flag indicating if the buffer is valid.
		 */
		inline bool isBufferValid() const { return isBufferValid_; }

		/**
		 * Set the buffering mode for the staging buffer.
		 * In case no explicit staging buffer is used, this will also set the buffering mode for the main buffer.
		 * @param mode the buffering mode to set.
		 */
		void setBufferingMode(BufferingMode mode);

		/**
		 * Enable a synchronization flag for the buffer object.
		 * @param flag the synchronization flag to set.
		 */
		void setSyncFlag(BufferSyncFlag flag) {
			stagingFlags_.syncFlags |= flag;
			flags_.syncFlags |= flag;
		}

		/**
		 * Update the buffer inputs and their offsets.
		 * Also compute the required size of the block, and build a list of dirty segments.
		 * @return the required size of the block in bytes.
		 */
		uint32_t updateStagedInputs();

		/**
		 * @return true if the buffer has any dirty segments.
		 */
		bool hasDirtySegments() const { return numDirtySegments_ > 0; }

		/**
		 * @return the number of dirty segments in the buffer.
		 */
		uint32_t numDirtySegments() const { return numDirtySegments_; }

		/**
		 * Update the buffer.
		 * Should be called each frame, is a no-op if no data has changed.
		 * @param forceUpdate force update.
		 */
		void update(bool forceUpdate = false);

		/**
		 * @return the list of uniforms.
		 */
		auto &stagedInputs() const { return inputs_; }

		/**
		 * Update the draw buffer, possibly adopting a new buffer range.
		 */
		void updateDrawBuffer();

		/**
		 * @return the reference to the draw buffer.
		 */
		const ref_ptr<BufferReference> &drawBufferRef() const { return drawBufferRef_; }

		/**
		 * @return the draw buffer name, i.e. the buffer ID.
		 */
		inline uint32_t drawBufferName() const { return drawBufferRef_->bufferID(); }

		/**
		 * @return the size of the draw buffer in bytes.
		 */
		inline uint32_t drawBufferSize() const { return drawBufferRef_->allocatedSize(); }

		/**
		 * @return the address of the draw buffer range within larger buffer.
		 */
		inline uint32_t drawBufferAddress() const { return drawBufferRef_->address(); }

		/**
		 * Copy the data to the draw buffer.
		 * @param forceUpdate force update, even if no segments are dirty.
		 */
		void copyStagingData(bool forceUpdate = false);

		/**
		 * Assigns an offset relative to segments in multi-buffering where this
		 * BO starts in each segment of the staging buffer.
		 * This is needed in case multiple BOs are sharing the same staging buffer.
		 * @param offset the offset in bytes to set.
		 */
		void setStagingOffset(uint32_t offset);

		/**
		 * Reset the staging buffer, removing it from the staging system if requested.
		 * @param removeFromStagingSystem if true, the buffer block will be removed from the staging system.
		 */
		void resetStagingBuffer(bool removeFromStagingSystem);

		/**
		 * @return the flags for the staging buffer.
		 */
		const BufferFlags &stagingFlags() const { return stagingFlags_; }

		/**
		 * @return the update hint for the staging buffer.
		 */
		BufferUpdateFlags stagingUpdateHint() const { return stagingFlags_.updateHints; }

		/**
		 * Set the map mode for the staging buffer.
		 * In case no explicit staging buffer is used, this will also set the map mode for the main buffer.
		 * @param mode the map mode to set.
		 */
		void setStagingMapMode(BufferMapMode mode);

		/**
		 * Set the access mode for the staging buffer.
		 * In case no explicit staging buffer is used, this will also set the access mode for the main buffer.
		 * @param mode the access mode to set.
		 */
		void setStagingAccessMode(ClientAccessMode mode);

		/**
		 * Get the update rate, which is the percentage of frames that had an update.
		 * @return the update rate as a float, where 0.0 means no updates and 1.0 means all frames had updates.
		 */
		inline float getUpdateRate() const {
			if (shared_->hasUpdateRotated_) {
				return static_cast<float>(shared_->updateCount_) * shared_->f_updateRangeInv_;
			} else {
				return -1.0f; // not enough frames to compute the update rate
			}
		}

		/**
		 * Set status for the current frame, i.e. if the buffer block was updated or not.
		 * @param isStalled true if the frame was stalled, false otherwise.
		 */
		inline void setUpdatedFrame(bool isUpdated) { shared_->setUpdatedFrame(isUpdated); }

		/**
		 * Reset the update history, clearing the array of updated frames.
		 */
		void resetUpdateHistory();

	protected:
		bool hasClientData_ = true;
		bool isBufferValid_ = true;

		std::vector<NamedShaderInput> inputs_;
		ref_ptr<BufferReference> drawBufferRef_;
		ref_ptr<BufferRange> drawBufferRange_;
		uint32_t requiredSize_ = 0;
		uint32_t adoptedSize_ = 0;
		uint32_t estimatedSize_ = 0;
		uint32_t updatedSize_ = 0;
		uint32_t stamp_ = 0;

		// a function ptr member that is used to adopt a buffer range for the draw buffer.
		std::function<ref_ptr<BufferReference>(uint32_t)> adoptBufferRange_;

		// the block inputs are used to store the shader inputs and their offsets in the buffer
		struct StagedInput {
			StagedInput() = default;

			StagedInput(const StagedInput &other) {
				input = other.input;
				offset = other.offset;
				lastStamp = other.lastStamp;
				inputSize = other.inputSize;
			}

			ref_ptr<ShaderInput> input;
			uint32_t offset = 0;
			std::vector<uint32_t> lastStamp = {0, 0};
			uint32_t inputSize = 0;
		};

		std::vector<ref_ptr<StagedInput>> stagedInputs_;

		// dirty segments are used to track which parts of the buffer have changed
		struct SegmentRange {
			uint32_t startIdx = 0; // start index of the segment in the blockInputs vector
			uint32_t endIdx = 0; // end index of the segment in the blockInputs vector
		};
		std::vector<SegmentRange> dirtySegmentRanges_;
		std::vector<BufferRange2ui> dirtyBufferRanges_;
		uint32_t numDirtySegments_ = 0;

		BufferFlags stagingFlags_;
		std::optional<BufferingMode> userDefinedBufferingMode_ = std::nullopt;

		struct Shared {
			Shared() : copyCount_(1) {}

			~Shared() {
				delete[] updatedFrames_;
			}

			ref_ptr<StagingBuffer> stagingBuffer_;
			// the offset in each staging buffer segment where the block data starts
			uint32_t stagingOffset_ = 0u;
			// the number of segments in the staging buffer, used for multi-buffering
			uint32_t numBufferSegments_ = 1u;
			// indicates if the block is globally staged, i.e. if it is managed by the staging system
			bool isGloballyStaged_ = false;
			// number of copies that are around
			std::atomic<uint32_t> copyCount_;

			// Array for update detection, true indicates we had an update in a frame.
			// We record last n frames for computing the update rate.
			bool *updatedFrames_ = nullptr;
			// Range for update detection
			uint32_t updateRange_ = 60u;
			float f_updateRangeInv_ = 1.0f / 60.0f;
			// Count of frames that had a stall
			uint32_t updateCount_ = 0;
			// Current index in the stall detection array
			uint32_t updateIdx_ = 0;
			// Indicates if the update history has rotated, i.e. we have wrapped around the update index.
			bool hasUpdateRotated_ = false;

			void setUpdatedFrame(bool isUpdated);
		};

		ref_ptr<Shared> shared_;

		inline void resetDirtySegments();

		inline void createNextDirtySegment();

		void setDirtyRange(uint32_t dirtyIdx, StagedInput &input, uint32_t inputIdx);

		void appendToDirtyRange(uint32_t dirtyIdx, StagedInput &input, uint32_t inputIdx);

		inline uint32_t &lastInputStamp(StagedInput &blockInput);

		void updateStorageFlags();

		void enableWriteAccess();

		void setStagingBuffering(BufferingMode mode);

		void copyDirtyData(byte *bufferData, uint32_t mapOffset);

		void copyFullData(byte *bufferData, uint32_t mapOffset);

		void updateNonMapped();

		void updateTemporaryMapped();

		void updatePersistentMapped();

		bool updateReadBuffer();

		void resetDataStamps();

		void markBufferDirty();

		void createStagingBuffer();

		void queueStagingUpdate();
	};
} // namespace

#endif /* REGEN_BUFFER_BLOCK_H_ */
