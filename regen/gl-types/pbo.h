#ifndef REGEN_PBO_H_
#define REGEN_PBO_H_

#include <regen/gl-types/gl-object.h>

namespace regen {
	/**
	 * \brief Pixel Buffer Objects (PBO) are OpenGL Buffer Objects that are used for asynchronous pixel transfers.
	 */
	class PBO : public GLObject {
	public:
		explicit PBO(unsigned int numBuffers = 1);

		~PBO() override = default;

		PBO(const PBO &) = delete;

		/**
		 * Bind the PBO as a pack buffer.
		 * @param index the buffer index.
		 */
		void bindPackBuffer(unsigned int index = 0) const;

		/**
		 * Bind the PBO as an unpack buffer.
		 * @param index the buffer index.
		 */
		void bindUnpackBuffer(unsigned int index = 0) const;

	protected:
	};
} // namespace

#endif /* REGEN_PBO_H_ */
