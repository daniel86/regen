#include "dirty-list.h"
#include <algorithm>

using namespace regen;

DirtyList::DirtyList() = default;

void DirtyList::Range::merge(const Range& other) {
	uint32_t new_start = std::min(offset, other.offset);
	uint32_t new_end = std::max(end(), other.end());
	offset = new_start;
	size = new_end - new_start;
}

void DirtyList::insert(uint32_t offset, uint32_t size) {
	if (size == 0) return;
	Range newRange{offset, size};

	// first check if we can merge with another range
	// assuming we do not have too many dirt ranges, it should be fine to do a linear search here.
	// Note that there still can be some fragmentation, as the new range may overlap with another range.
    for (uint32_t rangeIdx = 0; rangeIdx < count_; ++rangeIdx) {
		auto &range = ranges_[rangeIdx];
		if (range.overlaps(newRange)) {
			range.merge(newRange);
			return;  // merged, no need to add
		}
	}

	// Unable to merge, add as a new range.
	if (count_ == ranges_.size()) {
		ranges_.resize(count_ + 16);  // grow by chunk
	}
	ranges_[count_++] = newRange;
}

void DirtyList::append(uint32_t offset, uint32_t size) {
	if (size == 0) return;

	if (count_ == 0) {
		ranges_.push_back({offset, size});
		count_ = 1;
		return;
	}

	Range newRange{offset, size};
	auto &lastRange = ranges_[count_ - 1];
	if (lastRange.overlaps(newRange)) {
		lastRange.merge(newRange);
	} else {
		if (count_ == ranges_.size()) {
			ranges_.resize(count_ + 16);  // grow by chunk
		}
		ranges_[count_++] = newRange;
	}
}

void DirtyList::coalesce() {
	if (count_ <= 1) return;

	if (count_ <= 16) {
		// Use insertion sort for small count_
		for (uint32_t i = 1u; i < count_; ++i) {
			Range key = ranges_[i];
			uint32_t j = i;
			while (j > 0 && ranges_[j - 1].offset > key.offset) {
				ranges_[j] = ranges_[j - 1];
				--j;
			}
			ranges_[j] = key;
		}
	} else {
		std::sort(ranges_.begin(), ranges_.begin() + count_,
			[](const Range& a, const Range& b) {
				return a.offset < b.offset;
			});
	}

	uint32_t write = 0u;
	for (uint32_t i = 1u; i < count_; ++i) {
		auto &leftRange = ranges_[write];
		auto &rightRange = ranges_[i];
		if (leftRange.end() >= rightRange.offset) {
			// overlap
			leftRange.size = rightRange.end() - leftRange.offset;
		} else {
			// no overlap, move to next write idx
			ranges_[++write] = rightRange;
		}
	}

	count_ = write + 1;
}

void DirtyList::subtract(const DirtyList& other) {
    uint32_t write = 0;
    uint32_t j = 0;

    for (uint32_t i = 0; i < count_; ++i) {
        Range current = ranges_[i];
        uint32_t currentEnd = current.end();

        while (j < other.count_ && other.ranges_[j].end() <= current.offset) {
            ++j;
        }

        // Process overlapping parts
        while (j < other.count_ && other.ranges_[j].offset < currentEnd) {
            const Range& b = other.ranges_[j];
            uint32_t bEnd = b.end();

            if (b.offset <= current.offset && bEnd >= currentEnd) {
                // Fully covered
                current.size = 0;
                break;
            }

            if (b.offset <= current.offset && bEnd < currentEnd) {
                // Clip from left
                current.offset = bEnd;
                current.size = currentEnd - current.offset;
                ++j;
                continue;
            }

            if (b.offset > current.offset && bEnd >= currentEnd) {
                // Clip from right
                current.size = b.offset - current.offset;
                break;
            }

            if (b.offset > current.offset && bEnd < currentEnd) {
                // Split into two ranges
                if (write < ranges_.size())
                    ranges_[write++] = { current.offset, b.offset - current.offset };
                current.offset = bEnd;
                current.size = currentEnd - current.offset;
                ++j;
                continue;
            }

            ++j;
        }

        if (current.size > 0 && write < ranges_.size()) {
            ranges_[write++] = current;
        }
    }

    count_ = write;
}
