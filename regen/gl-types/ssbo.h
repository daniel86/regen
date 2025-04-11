#ifndef REGEN_SSBO_H_
#define REGEN_SSBO_H_

#include <regen/gl-types/buffer-block.h>

namespace regen {
	/**
	 * \brief A Shader Storage Buffer Object is a Buffer Object that is used to store and retrieve data from within the OpenGL Shading Language.
	 *
	 * SSBOs are a lot like Uniform Buffer Objects. Shader storage blocks are defined by Interface Block (GLSL)s in almost the same way as uniform blocks.
	 * Buffer objects that store SSBOs are bound to SSBO binding points, just as buffer objects for uniforms are bound to UBO binding points. And so forth.
	 */
	class SSBO : public BufferBlock, public ShaderInput {
	public:
		/**
		 * Memory qualifiers for shader storage blocks.
		 */
		enum MemoryQualifier {
			COHERENT = 0,
			VOLATILE,
			RESTRICT,
			READ_ONLY,
			WRITE_ONLY
		};

		SSBO(const std::string &name, BufferUsage usage);

		/**
		 * Copy constructor. Does not copy GPU data, both objects will share the same buffer.
		 * @param other another buffer object
		 * @param name name of the new buffer block
		 */
		explicit SSBO(const BufferObject &other, const std::string &name="");

		void set_memoryQualifier(MemoryQualifier qualifier) { memoryQualifier_ = qualifier; }

		MemoryQualifier memoryQualifier() const { return memoryQualifier_; }

		void write(std::ostream &out) const override;

	private:
		MemoryQualifier memoryQualifier_ = COHERENT;

		void initSSBO();
	};
} // namespace

#endif /* REGEN_SSBO_H_ */
