#ifndef REGEN_FREE_LIST_H_
#define REGEN_FREE_LIST_H_

#include "regen/gl/gl-rectangle.h"

namespace regen {
	/**
	 * \brief A data structure that maintains a list of contiguous ranges.
	 *
	 * Semantics of ranges is not pre-defined, can be e.g. free-lists or dirty-lists.
	 * We provide here one very fast way of building the list from contiguous blocks
	 * in order, and a bit slower method that inserts elements at arbitrary positions
	 * in the list. Insert operation attempts to merge inserted range with existing
	 * ranges in case they coincide.
	 */
	class FreeList {
	public:
		// default: 64 reserved nodes.
		static uint32_t NUM_RESERVED_NODES;

		/**
		 * Constructs an empty RangeList with one node ranging from 0 to fullSize.
		 * @param fullSize the size of the full range.
		 */
		explicit FreeList(uint32_t fullSize);

		/**
		 * @return the number of active nodes in the list.
		 */
		uint32_t getNumNodes() const { return activeCount_; }

		/**
		 * @return a score indicating the fragmentation of the list, in the range [0, 1].
		 */
		float getFragmentationScore() const;

		/**
		 * @return the size of the largest free range in the list.
		 */
		uint32_t getMaxFreeSize() const;

		/**
		 * Adopts a new sub-range of the list, if possible.
		 * The adopted range can then be exclusively used by the caller.
		 * The caller must further ensure to call orphan() on the range
		 * when it is no longer needed.
		 * @param size the size of the range to adopt.
		 * @return a pair of a boolean indicating whether the range was adopted, and the offset of the adopted range.
		 */
		[[nodiscard]]
		std::pair<bool,uint32_t> reserve(uint32_t size);

		/**
		 * Releases a previously adopted range, making it available for adoption again.
		 * @param size the size of the range to release.
		 * @param offset the offset of the range to release.
		 */
		void release(uint32_t size, uint32_t offset);

		/**
		 * Clears the free list, resetting it to a single node with full size.
		 */
		void clear(uint32_t fullSize);

	protected:
		const uint32_t fullSize_; //!< the full size of the free list
		struct Node {
			uint32_t offset; //!< the offset of the range
			uint32_t size;   //!< the size of the range
		};
		std::vector<Node> nodes_;
		uint32_t activeCount_;
	};
} // namespace

#endif /* REGEN_FREE_LIST_H_ */
