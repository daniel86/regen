#ifndef REGEN_CLIENT_DATA_BASE_H_
#define REGEN_CLIENT_DATA_BASE_H_

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
				: r(r), r_index(r_index), w(w), w_index(w_index) {}

		/**
		 * Read-only constructor.
		 * @param r the read data.
		 * @param r_index the read index.
		 */
		MappedClientData(const byte *r, int r_index)
				: r(r), r_index(r_index), w(nullptr), w_index(-1) {}

		/**
		 * The mapped data for reading.
		 */
		const byte *r;
		/**
		 * The read index.
		 */
		int r_index;
		/**
		 * The mapped data for writing.
		 */
		byte *w;
		/**
		 * The write index.
		 */
		int w_index;
	};
} // namespace

#endif /* REGEN_CLIENT_DATA_BASE_H_ */
