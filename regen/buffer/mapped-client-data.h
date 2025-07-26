#ifndef REGEN_MAPPED_CLIENT_DATA_H_
#define REGEN_MAPPED_CLIENT_DATA_H_

#include <regen/regen.h>

namespace regen {
	/**
	 * A low-level interface for read/write access to client data of shader input.
	 */
	struct MappedClientData {
		/**
		 * Default constructor.
		 * @param r the read data.
		 * @param r_index the read index.
		 * @param w the write data.
		 * @param w_index the write index.
		 */
		MappedClientData(const byte *r, int r_index, byte *w, int w_index)
				: r(r), w(w), r_index(r_index), w_index(w_index) {}

		/**
		 * Read-only constructor.
		 * @param r the read data.
		 * @param r_index the read index.
		 */
		MappedClientData(const byte *r, int r_index)
				: r(r), w(nullptr), r_index(r_index), w_index(-1) {}

		/**
		 * The mapped data for reading.
		 */
		const byte *r;
		/**
		 * The mapped data for writing.
		 */
		byte *w;
		/**
		 * The read index.
		 */
		int r_index;
		/**
		 * The write index.
		 */
		int w_index;
	};
} // namespace

#endif /* REGEN_MAPPED_CLIENT_DATA_H_ */
