#ifndef REGEN_BUFFER_CONTAINER_H_
#define REGEN_BUFFER_CONTAINER_H_

#include <regen/states/state.h>
#include <regen/gl-types/ubo.h>
#include <regen/gl-types/tbo.h>
#include <regen/textures/texture-buffer.h>
#include "regen/gl-types/ssbo.h"

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
	class BufferContainer : public State, public HasInput {
	public:
		/**
		 * Constructor that takes a list of all shader input objects of this container.
		 * The required buffers are allocated automatically, no need to call
		 * allocateBuffers() afterwards.
		 */
		BufferContainer(const std::string &bufferName,
				const std::vector<NamedShaderInput> &inputs,
				BufferUsage bufferUsage = BUFFER_USAGE_DYNAMIC_DRAW);

		/**
		 * No-arg constructor.
		 * Add individual ShaderInput objects using the addInput() method,
		 * once done, call the allocateBuffers() method to allocate the required buffers.
		 */
		explicit BufferContainer(const std::string &bufferName,
				BufferUsage bufferUsage = BUFFER_USAGE_DYNAMIC_DRAW);

		/**
		 * Copy constructor.
		 * @param other the other buffer container.
		 */
		BufferContainer(const BufferContainer &other) = delete;

		/**
		 * Add a shader input to the container.
		 * @param input the shader input object.
		 */
		void addInput(const ref_ptr<ShaderInput> &input, const std::string &name = "");

		/**
		 * Allocate the buffers for the shader inputs.
		 * Must be called after all inputs have been added.
		 */
		void updateBuffer();

		/**
		 * Get the UBO or TBO for a given shader input.
		 * @param input the shader input object.
		 * @return the buffer object.
		 */
		ref_ptr<BufferObject> getBufferObject(const ref_ptr<ShaderInput> &input);

		// Override from State
		void enable(RenderState *rs) override;

	protected:
		std::vector<NamedShaderInput> namedInputs_;
		std::vector<ref_ptr<UBO>> ubos_;
		std::vector<ref_ptr<SSBO>> ssbos_;
		std::vector<ref_ptr<TBO>> tbos_;
		std::vector<ref_ptr<TextureBuffer>> textureBuffers_;
		std::map<ShaderInput*, ref_ptr<BufferObject>> bufferObjectOfInput_;
		bool isAllocated_ = true;
		BufferUsage bufferUsage_;
		std::string bufferName_;

		void createUBO(const std::vector<NamedShaderInput> &namedInputs);

		void createSSBO(const std::vector<NamedShaderInput> &namedInputs);

		void createTBO(const NamedShaderInput &namedInput);

		std::string getNextBufferName();
	};
} // namespace

#endif /* REGEN_BUFFER_CONTAINER_H_ */
