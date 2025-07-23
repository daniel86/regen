#include "free-list.h"

using namespace regen;

uint32_t FreeList::NUM_RESERVED_NODES = 64; // default reserved nodes count

FreeList::FreeList(uint32_t fullSize)
		: fullSize_(fullSize), activeCount_(1) {
	nodes_.reserve(NUM_RESERVED_NODES); // reserve some space to avoid reallocations
	nodes_.push_back({0, fullSize});
}

void FreeList::clear(uint32_t fullSize) {
	if (nodes_.empty()) {
		nodes_.emplace_back();
	}
	nodes_[0] = {0u, fullSize};
	activeCount_ = 1;
}

float FreeList::getFragmentationScore() const {
	if (activeCount_ <= 1) return 0.0f;

	float avg = 0.0f;
	for (size_t i = 0; i < activeCount_; ++i) {
		avg += float(nodes_[i].size);
	}
	avg /= static_cast<float>(activeCount_);

	float score = 0.0f;
	for (size_t i = 0; i < activeCount_; ++i) {
		auto s = static_cast<float>(nodes_[i].size);
		if (s < avg * 0.25f) score += 1.0f; // count as "tiny"
	}
	return score / static_cast<float>(activeCount_); // % tiny nodes
}

std::pair<bool, uint32_t> FreeList::reserve(uint32_t size) {
	for (size_t i = 0; i < activeCount_; ++i) {
		Node &node = nodes_[i];
		if (node.size < size) continue;

		uint32_t offset = node.offset;
		node.offset += size;
		node.size -= size;
		if (node.size == 0) {
			// The node is now empty, remove it.
			// We do this here by shifting all nodes after it one position to the left, and
			// reducing the active count.
			for (size_t j = i + 1; j < activeCount_; ++j) {
				nodes_[j - 1] = nodes_[j];
			}
			--activeCount_;
		}
		return {true, offset};
	}

	return {false, 0}; // No fitting block found
}

void FreeList::release(uint32_t size, uint32_t offset) {
	// Find insert position via linear search.
	// Note: this is O(n) in the worst case. We could in fact use binary search here with log(n) complexity,
	//       as nodes are sorted by offset, but for small lists linear search will be ok.
	size_t i = 0;
	while (i < activeCount_ && nodes_[i].offset < offset) {
		++i;
	}
	// The node with index *i* is the first node >= offset.

	// Try merge with left node, i.e. the first node < offset.
	if (i > 0) {
		uint32_t leftEnd = nodes_[i - 1].offset + nodes_[i - 1].size;

		if (leftEnd > offset) {
			// The offset is inside the previous node, which is not allowed.
			REGEN_ERROR("Orphaning range at " << offset << " with size " << size
											  << " overlaps with existing range at " << nodes_[i - 1].offset
											  << " with size " << nodes_[i - 1].size);
			size -= leftEnd - offset; // Adjust size
		}

		if (leftEnd == offset) {
			// The offset is exactly at the end of the previous node, just how it should be!
			nodes_[i - 1].size += size;

			// Maybe also merge with right
			if (i < activeCount_ && offset + size == nodes_[i].offset) {
				nodes_[i - 1].size += nodes_[i].size;
				// Remove right node by shifting all nodes after it one position to the left.
				for (size_t j = i + 1; j < activeCount_; ++j) {
					nodes_[j - 1] = nodes_[j];
				}
				--activeCount_;
			}
			return;
		}
	}

	// Try merge with right only
	if (i < activeCount_ && offset + size == nodes_[i].offset) {
		nodes_[i].offset = offset;
		nodes_[i].size += size;
		return;
	}

	// No merge possible, insert new node at position i
	if (activeCount_ >= nodes_.capacity()) {
		// If we are at capacity, we need to extend the vector.
		nodes_.reserve(nodes_.capacity() + NUM_RESERVED_NODES); // Reserve some more space
	}
	if (activeCount_ < nodes_.size()) {
		// Reuse slot at the end
	} else {
		nodes_.emplace_back(); // Extend vector
	}
	// Shift all nodes after i one position to the right
	for (size_t j = activeCount_; j > i; --j) {
		nodes_[j] = nodes_[j - 1];
	}
	nodes_[i] = {offset, size};
	++activeCount_;
}
