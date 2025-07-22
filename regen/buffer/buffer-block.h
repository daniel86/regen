#ifndef REGEN_BUFFER_BLOCK_H_
#define REGEN_BUFFER_BLOCK_H_

#include "buffer-object.h"
#include "regen/scene/scene-input.h"
#include "regen/utility/threading.h"
#include "staging-buffer.h"

namespace regen {
	/**
	 * \brief Manages data in a buffer object that can be bound as buffer blocks in shaders.
	 *
	 * Buffer blocks adopts draw buffer ranges used for draw operation and maybe have
	 * an area in staging for CPU access.
	 */
	class BufferBlock : public BufferObject {
	public:
		static constexpr const char *TYPE_NAME = "BufferBlock";

		// The minimum number of segments for partial updates in temporary mapped buffers.
		static uint32_t MIN_SEGMENTS_PARTIAL_TEMPORARY;
		// The maximum update ratio for partial updates in temporary mapped buffers.
		static float MAX_UPDATE_RATIO_PARTIAL_TEMPORARY;
		// Frame range for update detection
		static uint32_t UPDATE_RATE_RANGE;

		/**
		 * Load a BufferBlock from a scene input node.
		 * @param ctx the loading context.
		 * @param input the scene input node.
		 * @return a reference to the loaded BufferBlock.
		 */
		static ref_ptr<BufferBlock> load(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * Buffer block qualifiers for shader storage blocks.
		 */
		enum Qualifier {
			// Uniform block
			UNIFORM = 0,
			// Shader storage block
			BUFFER,
			// Input block
			IN,
			// Output block
			OUT
		};

		/**
		 * Create a buffer block.
		 * @param target the buffer target.
		 * @param hints the buffer update hints.
		 * @param storageQualifier the storage qualifier.
		 * @param memoryLayout the memory layout.
		 */
		BufferBlock(
				BufferTarget target,
				const BufferUpdateFlags &hints,
				Qualifier storageQualifier,
				BufferMemoryLayout memoryLayout);

		/**
		 * Copy constructor. Does not copy GPU data, both objects will share the same buffer.
		 * @param other another buffer block
		 */
		BufferBlock(const BufferBlock &other);

		/**
		 * Copy constructor. Does not copy GPU data, both objects will share the same buffer.
		 * @param other another buffer object
		 */
		explicit BufferBlock(const BufferObject &other);

		~BufferBlock() override;

		/**
		 * @return get string representation of the block name.
		 */
		std::string getBlockName() const;

		/**
		 * Add a uniform to the UBO.
		 * @param input the shader input.
		 */
		void addBlockInput(const ref_ptr<ShaderInput> &input, const std::string &name = "");

		/**
		 * Remove a block input by name.
		 * @param name the name of the block input to remove.
		 */
		void removeBlockInput(std::string_view name);

		/**
		 * @return true if the block is a uniform block.
		 */
		bool isUBO() const { return blockQualifier_ == UNIFORM; }

		/**
		 * @return true if the block is a shader storage block.
		 */
		bool isSSBO() const { return blockQualifier_ == BUFFER; }

		/**
		 * @return a flag indicating if the block is valid.
		 */
		bool isBlockValid() const { return isBlockValid_; }

		/**
		 * @return the storage qualifier of the block.
		 */
		Qualifier blockQualifier() const { return blockQualifier_; }

		/**
		 * @return the memory layout of the block.
		 */
		BufferMemoryLayout memoryLayout() const { return memoryLayout_; }

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
		 * Enable this buffer block for drawing, ensuring
		 * it is bound to the correct shader location.
		 * @param loc the shader location to bind the block to.
		 */
		void enableBufferBlock(GLint loc);

		/**
		 * Binds the uniform block to the given shader location.
		 * @param loc the shader location to bind the block to.
		 */
		void bind(GLint loc);

		/**
		 * Set the binding index of the block.
		 * @param index the binding index.
		 */
		void set_bindingIndex(int index) { bindingIndex_ = index; }

		/**
		 * @return the binding index of the block.
		 */
		int bindingIndex() const { return bindingIndex_; }

		/**
		 * Update the block inputs and their offsets.
		 * Also compute the required size of the block, and build a list of dirty segments.
		 * @return the required size of the block in bytes.
		 */
		uint32_t updateBlockInputs();

		/**
		 * @return true if the block has any dirty segments.
		 */
		bool hasDirtySegments() const { return numDirtySegments_ > 0; }

		/**
		 * Update the block buffer.
		 * Should be called each frame, is a no-op if no data has changed.
		 * @param forceUpdate force update.
		 */
		void update(bool forceUpdate = false);

		/**
		 * @return the list of uniforms.
		 */
		auto &blockInputs() const { return inputs_; }

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
		uint32_t drawBufferName() const { return drawBufferRef_->bufferID(); }

		/**
		 * @return the size of the draw buffer in bytes.
		 */
		uint32_t drawBufferSize() const { return drawBufferRef_->allocatedSize(); }

		/**
		 * @return the address of the draw buffer range within larger buffer.
		 */
		uint32_t drawBufferAddress() const { return drawBufferRef_->address(); }

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
		void setStagingOffset(uint32_t offset) { shared_->stagingOffset_ = offset; }

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
		void setStagingAccessMode(BufferAccessMode mode);

		/**
		 * Get the update rate, which is the percentage of frames that had an update.
		 * @return the update rate as a float, where 0.0 means no updates and 1.0 means all frames had updates.
		 */
		float getUpdateRate() const;

		/**
		 * Set status for the current frame, i.e. if the buffer block was updated or not.
		 * @param isStalled true if the frame was stalled, false otherwise.
		 */
		void setUpdatedFrame(bool isStalled);

		/**
		 * Reset the update history, clearing the array of updated frames.
		 */
		void resetUpdateHistory();

	protected:
		Qualifier blockQualifier_;
		BufferMemoryLayout memoryLayout_;
		int bindingIndex_ = -1;

		bool hasClientData_ = true;
		bool isBlockValid_ = true;

		std::vector<NamedShaderInput> inputs_;
		ref_ptr<BufferReference> drawBufferRef_;
		ref_ptr<BufferRange> drawBufferRange_;
		uint32_t requiredSize_ = 0;
		uint32_t estimatedSize_ = 0;
		uint32_t updatedSize_ = 0;
		uint32_t stamp_ = 0;

		// the block inputs are used to store the shader inputs and their offsets in the buffer
		struct BlockInput {
			BlockInput() {
				// initially assume single-buffered, so we need only one last stamp.
				lastStamp.resize(1, 0);
			}

			BlockInput(const BlockInput &other) {
				input = other.input;
				offset = other.offset;
				lastStamp = other.lastStamp;
				alignedSize = other.alignedSize;
				inputSize = other.inputSize;
			}

			~BlockInput() {
				delete[] alignedData;
			}

			ref_ptr<ShaderInput> input;
			uint32_t offset = 0;
			std::vector<uint32_t> lastStamp;
			uint32_t alignedSize = 0;
			uint32_t inputSize = 0;
			byte *alignedData = nullptr;
		};

		std::vector<ref_ptr<BlockInput>> blockInputs_;

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
			float f_updateRange_ = 60.0f;
			// Count of frames that had a stall
			uint32_t updateCount_ = 0;
			// Current index in the stall detection array
			uint32_t updateIdx_ = 0;
			// Indicates if the update history has rotated, i.e. we have wrapped around the update index.
			bool hasUpdateRotated_ = false;
		};

		ref_ptr<Shared> shared_;

		inline void resetDirtySegments();

		inline void createNextDirtySegment();

		void setDirtyRange(uint32_t dirtyIdx, BlockInput &input, uint32_t inputIdx);

		void appendToDirtyRange(uint32_t dirtyIdx, BlockInput &input, uint32_t inputIdx);

		inline uint32_t &lastInputStamp(BlockInput &blockInput);

		void updateStorageFlags();

		void enableWriteAccess();

		void setStagingBuffering(BufferingMode mode);

		void copyDirtyData(byte *bufferData, uint32_t mapOffset);

		void copyFullData(byte *bufferData, uint32_t mapOffset);

		void copyBlockInput(BlockInput &blockInput, byte *bufferData, uint32_t mapOffset);

		void updateStridedData(BlockInput &uboInput);

		void updateNonMapped();

		void updateTemporaryMapped();

		void updatePersistentMapped();

		bool updateReadBuffer();

		void prepareRebind(GLint loc);

		void resetDataStamps();

		void markBufferDirty();

		int32_t getBufferedIndex(uint32_t stamp, const std::vector<uint32_t> &bufferedStamps) const;
	};

	std::ostream &operator<<(std::ostream &out, const BufferBlock::Qualifier &v);

	std::istream &operator>>(std::istream &in, BufferBlock::Qualifier &v);
} // namespace

#endif /* REGEN_BUFFER_BLOCK_H_ */
