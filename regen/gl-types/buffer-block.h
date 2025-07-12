#ifndef REGEN_BUFFER_BLOCK_H_
#define REGEN_BUFFER_BLOCK_H_

#include <regen/gl-types/buffer-object.h>
#include "regen/scene/scene-input.h"
#include "regen/utility/threading.h"
#include "staging-buffer.h"

namespace regen {
	/**
	 * \brief A BufferBlock is a Buffer Object that is used to store and retrieve data from within the OpenGL Shading Language.
	 *
	 * This base class is used to create Uniform Buffer Objects (UBO) and Shader Storage Buffer Objects (SSBO).
	 */
	class BufferBlock : public BufferObject {
	public:
		static constexpr const char *TYPE_NAME = "BufferBlock";

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
		 * @param usage the buffer usage.
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
		 * @param name name of the new buffer block
		 */
		BufferBlock(const BufferBlock &other);

		/**
		 * Copy constructor. Does not copy GPU data, both objects will share the same buffer.
		 * @param other another buffer object
		 * @param name name of the new buffer block
		 */
		explicit BufferBlock(const BufferObject &other);

		~BufferBlock() override = default;

		/**
		 * @return the reference to the buffer object.
		 */
		auto& blockReference() const { return ref_; }

		/**
		 * @return true if the block is a uniform block.
		 */
		auto isUBO() const { return blockQualifier_ == UNIFORM; }

		/**
		 * @return true if the block is a shader storage block.
		 */
		auto isSSBO() const { return blockQualifier_ == BUFFER; }

		/**
		 * @return the storage qualifier of the block.
		 */
		Qualifier blockQualifier() const { return blockQualifier_; }

		/**
		 * @return the memory layout of the block.
		 */
		BufferMemoryLayout memoryLayout() const { return memoryLayout_; }

		/**
		 * @return the update hint for the staging buffer.
		 */
		BufferUpdateFlags stagingUpdateHint() const { return stagingFlags_.updateHints; }

		/**
		 * @return the map mode for the staging buffer.
		 */
		BufferMapMode stagingMapMode() const { return stagingFlags_.mapMode; }

		/**
		 * @return the access mode for the staging buffer.
		 */
		BufferAccessMode stagingAccessMode() const { return stagingFlags_.accessMode; }

		/**
		 * @return the buffering mode for the staging buffer.
		 */
		BufferingMode stagingBuffering() const { return stagingFlags_.bufferingMode; }

		/**
		 * Set the update hint for the staging buffer.
		 * In case no explicit staging buffer is used, this will also set the update hint for the main buffer.
		 * @param hint the update hint to set.
		 */
		void setStagingUpdateHints(const BufferUpdateFlags &hints);

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
		 * Set the buffering mode for the staging buffer.
		 * In case no explicit staging buffer is used, this will also set the buffering mode for the main buffer.
		 * @param mode the buffering mode to set.
		 */
		void setBufferingMode(BufferingMode mode);

		/**
		 * Enable a synchronization flag for the buffer object.
		 * @param flag the synchronization flag to set.
		 */
		void setSyncFlag(BufferSyncFlag flag) { stagingFlags_.syncFlags |= flag; }

		/**
		 * Check if a specific synchronization flag is set.
		 * @param flag the synchronization flag to check.
		 * @return true if the flag is set, false otherwise.
		 */
		bool hasSyncFlag(BufferSyncFlag flag) const { return (stagingFlags_.syncFlags & flag) != 0; }

		/**
		 * @return true if the block has a binding index.
		 */
		bool has_bindingIndex() const { return bindingIndex_ >= 0; }

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
		 * @return get string representation of the block name.
		 */
		std::string getBlockName() const;

		/**
		 * Add a uniform to the UBO.
		 * @param input the shader input.
		 */
		void addBlockInput(const ref_ptr<ShaderInput> &input, const std::string &name = "");

		/**
		 * @return the list of uniforms.
		 */
		auto &blockInputs() const { return inputs_; }

		/**
		 * Update the block buffer.
		 * Should be called each frame, is a no-op if no data has changed.
		 * @param forceUpdate force update.
		 */
		void update(bool forceUpdate = false);

		/**
		 * Binds the uniform block to the given shader location.
		 */
		void enableBufferBlock(GLint loc);

		/**
		 * Lock the UBO, preventing updates.
		 */
		void lock() { lock_.lock(); }

		/**
		 * Unlock the UBO, allowing updates.
		 */
		void unlock() { lock_.unlock(); }

		/**
		 * Load a BufferBlock from a scene input node.
		 * @param ctx the loading context.
		 * @param input the scene input node.
		 * @return a reference to the loaded BufferBlock.
		 */
		static ref_ptr<BufferBlock> load(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * Set the minimum size for medium sized buffers.
		 * @param size the minimum size in bytes.
		 */
		static void setMediumBufferMinSize(uint32_t size) { MIN_SIZE_MEDIUM = size; }

		/**
		 * Set the minimum size for large sized buffers.
		 * @param size the minimum size in bytes.
		 */
		static void setLargeBufferMinSize(uint32_t size) { MIN_SIZE_LARGE = size; }

		/**
		 * Set the minimum size for very large sized buffers.
		 * @param size the minimum size in bytes.
		 */
		static void setVeryLargeBufferMinSize(uint32_t size) { MIN_SIZE_VERY_LARGE = size; }

		/**
		 * Set the minimum number of segments for partial updates in temporary mapped buffers.
		 * @param segments the minimum number of segments.
		 */
		static void setTemporaryMappingPartialMinSegments(uint32_t segments) {
			temporaryMappingPartialMinSegments = segments;
		}

		/**
		 * Set the maximum update ratio for partial updates in temporary mapped buffers.
		 * @param ratio the maximum update ratio.
		 */
		static void setTemporaryMappingPartialMaxUpdateRatio(float ratio) {
			temporaryMappingPartialMaxUpdateRatio = ratio;
		}

	protected:
		static uint32_t MIN_SIZE_MEDIUM;
		static uint32_t MIN_SIZE_LARGE;
		static uint32_t MIN_SIZE_VERY_LARGE;
		static uint32_t temporaryMappingPartialMinSegments;
		static float temporaryMappingPartialMaxUpdateRatio;

		Qualifier blockQualifier_;
		BufferMemoryLayout memoryLayout_;
		int bindingIndex_ = -1;
		SpinLock lock_;

		bool hasClientData_ = false;
		bool isBlockValid_ = true;

		std::vector<NamedShaderInput> inputs_;
		ref_ptr<BufferReference> ref_;
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
				if (alignedData) {
					delete[] alignedData;
				}
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
		struct DirtySegment {
			uint32_t offset = 0; // offset in the buffer
			uint32_t size = 0; // size of the segment in bytes
			uint32_t startIdx = 0; // start index of the segment in the blockInputs vector
			uint32_t endIdx = 0; // end index of the segment in the blockInputs vector

			void set(BlockInput &input, uint32_t inputIdx) {
				offset = input.offset;
				size = input.inputSize;
				startIdx = inputIdx;
				endIdx = inputIdx;
			}

			void append(BlockInput &input, uint32_t inputIdx) {
				size = input.offset - offset + input.inputSize;
				endIdx = inputIdx;
			}
		};
		std::vector<DirtySegment> dirtySegments_;
		uint32_t numDirtySegments_ = 0;

		BufferFlags stagingFlags_;
		std::optional<BufferingMode> userDefinedBufferingMode_ = std::nullopt;
		ref_ptr<StagingBuffer> stagingBuffer_;
		ref_ptr<BufferRange> drawBufferRange_;

		inline void resetDirtySegments();

		inline DirtySegment& getLastDirtySegment();

		inline DirtySegment& getNextDirtySegment();

		inline uint32_t& lastInputStamp(BlockInput &blockInput);

		void enableWriteAccess();

		void setStagingBuffering(BufferingMode mode);

		void enablePersistentMapping(bool partialWrite);

		void enablePersistentMapping_(bool useFlushExplicit);

		void updateBlockInputs();

		void copyBufferData(byte *bufferData, uint32_t mapOffset, bool partialWrite);

		void copyBufferData1(byte *bufferData, uint32_t mapOffset, BlockInput &uboInput);

		void updateStridedData(BlockInput &uboInput);

		void updateNonMapped();

		void updateTemporaryMapped();

		void updatePersistentMapped();

		void updateAllBuffers();

		void resize();

		BufferSizeClass getBufferSizeClass(uint32_t size);
	};

	std::ostream &operator<<(std::ostream &out, const BufferBlock::Qualifier &v);

	std::istream &operator>>(std::istream &in, BufferBlock::Qualifier &v);
} // namespace

#endif /* REGEN_BUFFER_BLOCK_H_ */
