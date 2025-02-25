#ifndef REGEN_UBO_H_
#define REGEN_UBO_H_

#include <map>
#include <regen/gl-types/gl-object.h>
#include <regen/gl-types/shader-input.h>

namespace regen {
	/**
	 * \brief Uniform Buffer Objects are a mechanism for sharing data
	 * between the CPU and the GPU.
	 * They are OpenGL Objects that allow you to store data in a
	 * buffer that can be accessed by shaders.
	 */
	class UBO : public GLObject {
	public:
		UBO();

		~UBO() override = default;

		UBO(const UBO &) = delete;

		/**
		 * Add a uniform to the UBO.
		 * @param input the shader input.
		 */
		void addUniform(const ref_ptr<ShaderInput> &input, const std::string &name);

		/**
		 * @param input the shader input.
		 */
		void updateUniform(const ref_ptr<ShaderInput> &input);

		/**
		 * Update the UBO.
		 * Should be called each frame, is a no-op if no data has changed.
		 * @param forceUpdate force update.
		 */
		void update(bool forceUpdate=false);

		/**
		 * @return the list of uniforms.
		 */
		auto &uniforms() const { return uniforms_; }

		/**
		 * @return the allocated size.
		 */
		GLuint allocatedSize() const { return allocatedSize_; }

		void bindBufferBase(GLuint bindingPoint) const;

		/**
		 * Lock the UBO, preventing updates.
		 */
		void lock() { mutex_.lock(); }

		/**
		 * Unlock the UBO, allowing updates.
		 */
		void unlock() { mutex_.unlock(); }

	protected:
		struct UBO_Input {
			UBO_Input() = default;
			UBO_Input(const UBO_Input &other) {
				input = other.input;
				offset = other.offset;
			}
			~UBO_Input() {
				if (alignedData) {
					delete[] alignedData;
				}
			}
			ref_ptr<ShaderInput> input;
			GLuint offset = 0;
			GLuint lastStamp = 0;
			GLuint alignedSize = 0;
			byte *alignedData = nullptr;
		};
		std::vector<UBO_Input> uboInputs_;
		std::vector<NamedShaderInput> uniforms_;
		GLuint allocatedSize_;
		GLuint requiredSize_;
		GLboolean requiresResize_;
		GLuint stamp_;
		std::mutex mutex_;

		GLboolean needsUpdate() const;

		void computePaddedSize();

		static void updateAlignedData(UBO_Input &uboInput);
	};
} // namespace

#endif /* REGEN_UBO_H_ */
