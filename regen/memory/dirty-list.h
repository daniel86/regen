#ifndef REGEN_DIRTY_LIST_H_
#define REGEN_DIRTY_LIST_H_

#include <vector>
#include <cstdint>

namespace regen {
	/**
	 * \brief A list of dirty ranges in a buffer.
	 *
	 * This class is used to track which parts of a buffer have been modified
	 * and need to be updated in the GPU.
	 */
	class DirtyList {
	public:
		/**
		 * \brief A range of dirty data in the buffer.
		 *
		 * This struct represents a range of data that has been modified.
		 * It contains an offset and a size, and provides methods to check
		 * for overlaps with other ranges and to merge with other ranges.
		 */
		struct Range {
			// the offset in bytes from the start of the buffer
			uint32_t offset = 0u;
			// size in bytes
			uint32_t size = 0u;

			/**
			 * @return the end offset of the range.
			 */
			inline uint32_t end() const{ return offset + size; }

			/**
			 * Check if this range overlaps with another range.
			 * @param other The other range to check against.
			 * @return true if the ranges overlap, false otherwise.
			 */
			inline bool overlaps(const Range& other) const {
				return offset < other.end() && other.offset < end();
			}

			/**
			 * Merge this range with another range.
			 * The resulting range will cover both ranges.
			 * @param other The other range to merge with.
			 */
			void merge(const Range& other);
		};

		/**
		 * \brief Default constructor.
		 * Initializes an empty dirty list.
		 */
		DirtyList();

		/**
		 * \brief Get a dirty range by index.
		 * @param idx The index of the dirty range.
		 * @return Reference to the dirty range at the specified index.
		 */
		inline Range& operator[](uint32_t idx) { return ranges_[idx]; }

		/**
		 * \brief Get a dirty range by index.
		 * @param idx The index of the dirty range.
		 * @return Reference to the dirty range at the specified index.
		 */
		inline const Range& operator[](uint32_t idx) const { return ranges_[idx]; }

		/**
		 * \brief Get the list of dirty ranges.
		 * @return Pointer to the array of dirty ranges.
		 */
		inline Range* ranges() { return ranges_.data(); }

		/**
		 * Get the number of dirty ranges in the list.
		 * @return The number of dirty ranges.
		 */
		inline uint32_t count() const { return count_; }

		/**
		 * Check if the dirty list is empty.
		 * @return true if there are no dirty ranges, false otherwise.
		 */
		inline bool empty() const { return count_ == 0; }

		/**
		 * Clear the dirty list.
		 * Resets the count of dirty ranges to zero.
		 */
		inline void clear() { count_ = 0; }

		/**
		 * Insert a new dirty range into the list.
		 * If the range overlaps with an existing range, it will be merged.
		 * @param offset The offset in bytes from the start of the buffer.
		 * @param size The size of the dirty range in bytes.
		 */
		void insert(uint32_t offset, uint32_t size);

		/**
		 * Append a new dirty range to the list.
		 * This method does only check for overlaps with the last range in the list.
		 * If the new range overlaps with the last range, it will be merged.
		 * @param offset The offset in bytes from the start of the buffer.
		 * @param size The size of the dirty range in bytes.
		 */
		void append(uint32_t offset, uint32_t size);

		/**
		 * Note: both lists must be coalesced before calling this method.
		 * @param other
		 */
		void subtract(const DirtyList &other);

	protected:
		std::vector<Range> ranges_;
		uint32_t count_ = 0;
	};
} // namespace

#endif /* REGEN_DIRTY_LIST_H_ */
