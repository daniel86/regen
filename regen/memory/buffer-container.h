#ifndef REGEN_BUFFER_CONTAINER_H_
#define REGEN_BUFFER_CONTAINER_H_

#include "ubo.h"
#include "tbo.h"
#include "regen/textures/texture-buffer.h"
#include "regen/memory/ssbo.h"
#include "regen/scene/state.h"

namespace regen {
	/**
	 * \brief A container for GL data stored in UBOs and TBOs.
	 *
	 * This class provides a high-level interface for managing per-object
	 * (read-only) data for rasterization. Individual ShaderInput objects can be
	 * added, and based on their size and type, the class will automatically
	 * populate UBOs and TBOs as needed such that no size constraints are
	 * violated. UBO is used as long as there are no individual inputs
	 * exceeding maximum UBO size. If the maximum UBO size is exceeded, a TBO is used.
	 * This is transparent for shader code, as in case of TBO some defines are added such
	 * that the shader code is generated to use TBOs instead of UBOs.
	 */
	class BufferContainer : public State {
	public:
		/**
		 * Constructor that takes a list of all shader input objects of this container.
		 * The required buffers are allocated automatically, no need to call
		 * allocateBuffers() afterwards.
		 */
		BufferContainer(std::string_view bufferName,
				const std::vector<NamedShaderInput> &inputs,
				const BufferUpdateFlags &hints);

		/**
		 * No-arg constructor.
		 * Add individual ShaderInput objects using the addInput() method,
		 * once done, call the allocateBuffers() method to allocate the required buffers.
		 */
		BufferContainer(std::string_view bufferName, const BufferUpdateFlags &hints);

		/**
		 * Copy constructor.
		 * @param other the other buffer container.
		 */
		BufferContainer(const BufferContainer &other) = delete;

		/**
		 * @return the buffer update hints.
		 */
		BufferUpdateFlags bufferUpdateHints() const { return bufferUpdateHints_; }

		/**
		 * @param mode the buffering mode to set.
		 */
		void setBufferingMode(BufferingMode mode) { bufferingMode_ = mode; }

		/**
		 * Add a shader input to the container.
		 * @param input the shader input object.
		 */
		void addStagedInput(const ref_ptr<ShaderInput> &input, std::string_view name = "");

		/**
		 * Allocate the buffers for the shader inputs.
		 * Must be called after all inputs have been added.
		 */
		void updateBuffer();

		/**
		 * Get the list of named shader inputs in this container.
		 * @return the named shader input list.
		 */
		const std::vector<NamedShaderInput> &namedInputs() const { return namedInputs_; }

		/**
		 * Get the list of UBOs in this container.
		 * @return the UBO list.
		 */
		const std::vector<ref_ptr<UBO>> &ubos() const { return ubos_; }

		/**
		 * Get the list of SSBOs in this container.
		 * @return the SSBO list.
		 */
		const std::vector<ref_ptr<SSBO>> &ssbos() const { return ssbos_; }

		/**
		 * Get the list of TBOs in this container.
		 * @return the TBO list.
		 */
		const std::vector<ref_ptr<TBO>> &tbos() const { return tbos_; }

		/**
		 * Get the list of TextureBuffers in this container.
		 * @return the TextureBuffer list.
		 */
		const std::vector<ref_ptr<TextureBuffer>> &textureBuffers() const { return textureBuffers_; }

		/**
		 * Get the list of all staged buffers in this container.
		 * @return the staged buffer list.
		 */
		const std::vector<ref_ptr<StagedBuffer>> &stagedBuffers() const { return stagedBuffers_; }

		/**
		 * Get the UBO or TBO for a given shader input.
		 * @param input the shader input object.
		 * @return the buffer object.
		 */
		ref_ptr<StagedBuffer> getBufferObject(const ref_ptr<ShaderInput> &input);

		/**
		 * Print the layout of the buffer container, i.e. what UBOs, TBOs and
		 * SSBOs are used and how much memory they use.
		 */
		void printLayout();

	protected:
		std::vector<NamedShaderInput> namedInputs_;
		std::vector<ref_ptr<UBO>> ubos_;
		std::vector<ref_ptr<SSBO>> ssbos_;
		std::vector<ref_ptr<TBO>> tbos_;
		std::vector<ref_ptr<TextureBuffer>> textureBuffers_;
		std::vector<ref_ptr<StagedBuffer>> stagedBuffers_;
		std::map<ShaderInput*, ref_ptr<StagedBuffer>> bufferObjectOfInput_;
		bool isAllocated_ = true;
		BufferUpdateFlags bufferUpdateHints_;
		std::optional<BufferingMode> bufferingMode_;
		std::string bufferName_;

		void createUBO(const std::vector<NamedShaderInput> &namedInputs);

		void createSSBO(const std::vector<NamedShaderInput> &namedInputs);

		void createTBO(const NamedShaderInput &namedInput);

		std::string getNextBufferName();
	};
} // namespace

#endif /* REGEN_BUFFER_CONTAINER_H_ */
