#ifndef REGEN_BUFFER_BLOCK_H_
#define REGEN_BUFFER_BLOCK_H_

#include "staged-buffer.h"

namespace regen {
	/**
	 * \brief Manages data in a buffer object that can be bound as buffer blocks in shaders.
	 *
	 * Buffer blocks adopts draw buffer ranges used for draw operation and maybe have
	 * an area in staging for CPU access.
	 */
	class BufferBlock : public StagedBuffer {
	public:
		static constexpr const char *TYPE_NAME = "BufferBlock";

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
		BufferBlock(std::string_view name,
				BufferTarget target,
				const BufferUpdateFlags &hints,
				Qualifier storageQualifier,
				BufferMemoryLayout memoryLayout);

		/**
		 * Copy constructor. Does not copy GPU data, both objects will share the same buffer.
		 * @param other another buffer object
		 */
		explicit BufferBlock(const StagedBuffer &other, std::string_view name="");

		~BufferBlock() override;

		/**
		 * @return true if the block is a uniform block.
		 */
		bool isUBO() const { return blockQualifier_ == UNIFORM; }

		/**
		 * @return true if the block is a shader storage block.
		 */
		bool isSSBO() const { return blockQualifier_ == BUFFER; }

		/**
		 * @return the storage qualifier of the block.
		 */
		Qualifier blockQualifier() const { return blockQualifier_; }

		/**
		 * Enable this buffer block for drawing, ensuring
		 * it is bound to the correct shader location.
		 * @param loc the shader location to bind the block to.
		 */
		void enableBufferBlock(int loc);

		/**
		 * Binds the uniform block to the given shader location.
		 * @param loc the shader location to bind the block to.
		 */
		void bind(int loc);

		/**
		 * Set the binding index of the block.
		 * @param index the binding index.
		 */
		void set_bindingIndex(int index) { bindingIndex_ = index; }

		/**
		 * @return the binding index of the block.
		 */
		int bindingIndex() const { return bindingIndex_; }

	protected:
		Qualifier blockQualifier_;
		int bindingIndex_ = -1;

		void prepareRebind(int loc);
	};

	std::ostream &operator<<(std::ostream &out, const BufferBlock::Qualifier &v);

	std::istream &operator>>(std::istream &in, BufferBlock::Qualifier &v);
} // namespace

#endif /* REGEN_BUFFER_BLOCK_H_ */
