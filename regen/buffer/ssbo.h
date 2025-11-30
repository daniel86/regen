#ifndef REGEN_SSBO_H_
#define REGEN_SSBO_H_

#include "buffer-block.h"

namespace regen {
	/**
	 * \brief A Shader Storage Buffer Object is a Buffer Object that is used to store and retrieve data from within the OpenGL Shading Language.
	 *
	 * SSBOs are a lot like Uniform Buffer Objects. Shader storage blocks are defined by Interface Block (GLSL)s in almost the same way as uniform blocks.
	 * Buffer objects that store SSBOs are bound to SSBO binding points, just as buffer objects for uniforms are bound to UBO binding points. And so forth.
	 */
	class SSBO : public BufferBlock {
	public:
		/**
		 * Memory qualifiers for shader storage blocks.
		 */
		enum MemoryQualifier {
			// Normally, the compiler is free to assume that this shader invocation is the only invocation that modifies
			// values read through this variable. It also can freely assume that other shader invocations may not see
			// values written through this variable.
			// Using this qualifier is required to allow dependent shader invocations to communicate with one another,
			// as it enforces the coherency of memory accesses. Using this requires the appropriate memory barriers
			// to be executed, so that visibility can be achieved.
			// When communicating between shader invocations for different rendering commands, glMemoryBarrier
			// should be used instead of this qualifier.
			COHERENT   = 1 << 0,
			// The compiler normally is free to assume that values accessed through variables will only change after
			// memory barriers or other synchronization. With this qualifier, the compiler assumes that the contents
			// of the storage represented by the variable could be changed at any time.
			VOLATILE   = 1 << 1,
			// Normally, the compiler must assume that you could access the same image/buffer object through separate
			// variables in the same shader. Therefore, if you write to one variable, and read from a second, the
			// compiler assumes that it is possible that you could be reading the value you just wrote. With this
			// qualifier, you are telling the compiler that this particular variable is the only variable that can
			// modify the memory visible through that variable within this shader invocation (other shader stages
			// don't count here). This allows the compiler to optimize reads/writes better.
			// You should use this wherever possible.
			RESTRICT   = 1 << 2,
			// Normally, the compiler allows you to read and write from variables as you wish. If you use this,
			// the variable can only be used for reading operations (atomic operations are forbidden as
			// they also count as writes).
			READ_ONLY  = 1 << 3,
			// Normally, the compiler allows you to read and write from variables as you wish. If you use this,
			// the variable can only be used for writing operations (atomic operations are forbidden as they
			// also count as reads).
			WRITE_ONLY = 1 << 4
		};

		SSBO(std::string_view name, const BufferUpdateFlags &hints, int memoryMask = 0);

		/**
		 * Copy constructor. Does not copy GPU data, both objects will share the same buffer.
		 * @param other another buffer object
		 * @param name name of the new buffer block
		 */
		explicit SSBO(const StagedBuffer &other, std::string_view name="");

		/**
		 * Sets the memory mask.
		 * @param mask the memory mask.
		 */
		void set_memoryMask(int mask) { memoryMask_ = mask; }

		/**
		 * Sets a flag in the memory mask.
		 * @param bit the flag to set.
		 */
		void setMemoryQualifier(MemoryQualifier bit) { memoryMask_ |= bit; }

		/**
		 * Checks if the memory mask has a specific flag.
		 * @param bit the flag to check.
		 * @return true if the flag is set, false otherwise.
		 */
		bool hasMemoryQualifier(MemoryQualifier bit) const { return (memoryMask_ & bit) != 0; }

		void write(std::ostream &out) const override;

	private:
		int memoryMask_ = 0;
	};
} // namespace

#endif /* REGEN_SSBO_H_ */
