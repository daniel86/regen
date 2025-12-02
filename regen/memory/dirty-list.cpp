#include "dirty-list.h"
#include <algorithm>

using namespace regen;

DirtyList::DirtyList() : ranges_(16) {}

void DirtyList::Range::merge(const Range& other) {
	uint32_t new_start = std::min(offset, other.offset);
	uint32_t new_end = std::max(end(), other.end());
	offset = new_start;
	size = new_end - new_start;
}

void DirtyList::insert(uint32_t offset, uint32_t size) {
    const Range newRange{offset, size};

    // Fast path: empty list
    if (count_ == 0) {
        ranges_[0] = newRange;
        count_ = 1;
        return;
    }

    // Reference to last range
    Range& last = ranges_[count_ - 1];

    // Fast path: strictly after last -> append
    if (offset >= last.end()) {
        if (offset == last.end()) {
            // Direct extension
            last.size += size;
        } else {
            // Append new range
            if (count_ == ranges_.size())
                ranges_.resize(ranges_.size() + 16);
            ranges_[count_++] = newRange;
        }
        return;
    }

    // Fast path: merge with last if overlapping
    if (last.overlaps(newRange)) {
        last.merge(newRange);
        return;
    }

    // -----------------------------------------
    // GENERAL CASE: Insert into a sorted list, then merge neighbors
    // -----------------------------------------

    // Ensure capacity before shifting
    if (count_ == ranges_.size()) {
        ranges_.resize(ranges_.size() + 16);
    }

    // Find insertion position (sorted insert)
    uint32_t pos = count_;
    while (pos > 0 && ranges_[pos - 1].offset > offset) {
    	// Make room for insertion by shifting right:
        ranges_[pos] = ranges_[pos - 1];
        --pos;
    }
    ranges_[pos] = newRange;
    ++count_;

    // Merge left neighbor if overlap
    if (pos > 0 && ranges_[pos - 1].overlaps(ranges_[pos])) {
        ranges_[pos - 1].merge(ranges_[pos]);
        // shift everything after pos one left
        for (uint32_t i = pos + 1; i < count_; ++i) {
            ranges_[i - 1] = ranges_[i];
        }
        --count_;
        pos -= 1;  // merged into left neighbor
    }

    // Merge right neighbor(s) (can be multiple)
    while (pos + 1 < count_ && ranges_[pos].overlaps(ranges_[pos + 1])) {
        ranges_[pos].merge(ranges_[pos + 1]);
        // shift down
        for (uint32_t i = pos + 2; i < count_; ++i) {
            ranges_[i - 1] = ranges_[i];
        }
        --count_;
    }
}

void DirtyList::append(uint32_t offset, uint32_t size) {
	if (size == 0) return;

	if (count_ == 0) {
		ranges_.push_back({offset, size});
		count_ = 1;
		return;
	}

	const Range newRange{offset, size};
	if (auto &lastRange = ranges_[count_ - 1]; lastRange.overlaps(newRange)) {
		lastRange.merge(newRange);
	} else {
		if (count_ == ranges_.size()) {
			ranges_.resize(count_ + 16);  // grow by chunk
		}
		ranges_[count_++] = newRange;
	}
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
