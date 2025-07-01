#ifndef REGEN_BUFFER_BLOCK_H_
#define REGEN_BUFFER_BLOCK_H_

#include <regen/gl-types/buffer-object.h>
#include "regen/scene/scene-input.h"
#include "regen/utility/threading.h"
#include "buffer-mapping.h"

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
		 * Storage qualifiers for shader storage blocks.
		 */
		enum StorageQualifier {
			// Uniform block
			UNIFORM = 0,
			// Shader storage block
			BUFFER,
			// Input block
			IN,
			// Output block
			OUT
		};
		enum MemoryLayout {
			STD140 = 0,
			STD430,
			PACKED,
			SHARED
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
			BufferUsage usage,
			StorageQualifier storageQualifier,
			MemoryLayout memoryLayout);

		~BufferBlock() override = default;

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

		/**
		 * Enforce a persistent mapping of the buffer block.
		 * The default mode is that the mapping mode is determined based on the buffer usage.
		 * @param isPersistent true if the buffer block should be persistently mapped.
		 */
		void setPersistentMapping(bool isPersistent);

		static ref_ptr<BufferBlock> load(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * @return the reference to the buffer object.
		 */
		auto& blockReference() const { return ref_; }

		/**
		 * @return true if the block is a uniform block.
		 */
		auto isUniformBlock() const { return storageQualifier_ == UNIFORM; }

		/**
		 * @return true if the block is a shader storage block.
		 */
		auto isShaderStorageBlock() const { return storageQualifier_ == BUFFER; }

		/**
		 * @return the storage qualifier of the block.
		 */
		StorageQualifier storageQualifier() const { return storageQualifier_; }

		/**
		 * @return the memory layout of the block.
		 */
		MemoryLayout memoryLayout() const { return memoryLayout_; }

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

	protected:
		StorageQualifier storageQualifier_;
		MemoryLayout memoryLayout_;
		int bindingIndex_ = -1;
		SpinLock lock_;

		struct BlockInput {
			BlockInput() = default;

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
			uint32_t lastStamp = 0;
			uint32_t alignedSize = 0;
			uint32_t inputSize = 0;
			byte *alignedData = nullptr;
		};
		struct BlockSegment {
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

		bool usePersistentMapping_ = false;
		ref_ptr<BufferMapping> persistentMapping_;

		std::vector<ref_ptr<BlockInput>> blockInputs_;
		std::vector<BlockSegment> nextSegments_;
		uint32_t numNextSegments_ = 0;
		std::vector<NamedShaderInput> inputs_;
		ref_ptr<BufferReference> ref_;
		uint32_t requiredSize_ = 0;
		uint32_t updatedSize_ = 0;
		uint32_t stamp_ = 0;
		bool hasClientData_ = false;
		bool isBlockValid_ = true;

		inline void resetSegments();

		inline BlockSegment& getLastSegment();

		inline BlockSegment& getNextSegment();

		void updateBlockInputs();

		void copyBufferData(char *bufferData, bool partialWrite);

		void copyBufferData1(char *bufferData, BlockInput &uboInput);

		void updateStridedData(BlockInput &uboInput);

		void updateNonPersistent();

		void updatePersistent(bool needsResize);

		void resize();
	};

	std::ostream &operator<<(std::ostream &out, const BufferBlock::MemoryLayout &v);
	std::ostream &operator<<(std::ostream &out, const BufferBlock::StorageQualifier &v);

	std::istream &operator>>(std::istream &in, BufferBlock::MemoryLayout &v);
	std::istream &operator>>(std::istream &in, BufferBlock::StorageQualifier &v);
} // namespace

#endif /* REGEN_BUFFER_BLOCK_H_ */
