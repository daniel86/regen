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
		 * @return the stamp.
		 */
		GLuint stamp() const;

		/**
		 * Add a uniform to the UBO.
		 * @param input the shader input.
		 */
		void addUniform(const ref_ptr<ShaderInput> &input);

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

	protected:
		struct UBO_Input {
			ref_ptr<ShaderInput> input;
			GLuint offset;
			GLuint lastStamp;
		};
		std::map<std::string, UBO_Input> uniformMap_;
		std::vector<ref_ptr<ShaderInput>> uniforms_;
		GLuint allocatedSize_;
		GLuint requiredSize_;
		GLboolean requiresResize_;
		GLuint stamp_;

		GLboolean needsUpdate() const;

		void computePaddedSize();
	};
} // namespace

#endif /* REGEN_UBO_H_ */
