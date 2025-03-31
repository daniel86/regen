#include "memory-allocator.h"
#include <stack>

#define EXACT_BUDDY_ALLOCATION

using namespace regen;

BuddyAllocator::BuddyNode::BuddyNode(
		unsigned int _address, unsigned int _size,
		BuddyNode *_parent)
		: state(FREE),
		  address(_address), size(_size), maxSpace(_size),
		  leftChild(nullptr), rightChild(nullptr), parent(_parent) {
}

BuddyAllocator::BuddyAllocator(unsigned int size) {
	buddyTree_ = createNode(0u, size, nullptr);
}

BuddyAllocator::~BuddyAllocator() {
	if (buddyTree_) {
		clear(buddyTree_);
		buddyTree_ = nullptr;
	}
}

BuddyAllocator::State BuddyAllocator::allocatorState() const { return buddyTree_->state; }

unsigned int BuddyAllocator::size() const { return buddyTree_->size; }

unsigned int BuddyAllocator::maxSpace() const { return buddyTree_->maxSpace; }

void BuddyAllocator::computeMaxSpace(BuddyNode *n) {
	if (n->leftChild != nullptr) {
		n->maxSpace = std::max(n->leftChild->maxSpace, n->rightChild->maxSpace);
	}
	// must recompute maxSpace for given node and all parents
	for (BuddyNode *t = n->parent; t != nullptr; t = t->parent) {
		t->maxSpace = std::max(t->leftChild->maxSpace, t->rightChild->maxSpace);
	}
}

unsigned int BuddyAllocator::createPartition(BuddyNode *n, unsigned int size) {
	if (size == n->size) {
		// the buddy node fits perfectly with the requested size.
		// Just mark the node as full and return the address.
		n->state = FULL;
		n->maxSpace = 0;
		return n->address;
	}
#ifndef EXACT_BUDDY_ALLOCATION
	else if(size*2 > n->size) {
	  // half of the node size is not enough for the requested memory.
#endif
	// Create a node that fits exactly with the requested size
	// and the rest of the node space remains free for allocation.
	// No intern fragmentation occurs.
	unsigned int size0 = size;
	unsigned int size1 = n->size - size;
	n->state = PARTIAL;
	n->maxSpace = size1;
	n->leftChild = createNode(n->address, size0, n);
	n->leftChild->state = FULL;
	n->leftChild->maxSpace = 0;
	n->rightChild = createNode(n->address + size0, size1, n);
	return n->address;
#ifndef EXACT_BUDDY_ALLOCATION
	}
	else {
	  // half of the node size is enough for the requested memory.
	  // Split the node with half size and continue for left child.
	  unsigned int size0 = n->size/2;
	  unsigned int size1 = n->size-size0;
	  n->state = PARTIAL;
	  n->maxSpace = size1;
	  n->leftChild = new BuddyNode(n->address, size0, n);
	  n->rightChild = new BuddyNode(n->address+size0, size1, n);
	  return createPartition(n->leftChild, size);
	}
#endif
}

bool BuddyAllocator::alloc(unsigned int size, unsigned int *x) {
	if (buddyTree_->maxSpace < size) return false;

	BuddyNode *n = buddyTree_;
	while (true) {
		switch (n->state) {
			case FREE:
				// free node with enough space. Create partitions inside the
				// node to avoid internal fragmentation.
				*x = createPartition(n, size);
				computeMaxSpace(n);
				return true;
			case PARTIAL: {
				// walk down the tree until we reach a FREE node.
				int d1 = n->leftChild->maxSpace - size;
				int d2 = n->rightChild->maxSpace - size;
				if (d2 < 0 || (d1 >= 0 && d1 < d2)) { n = n->leftChild; }
				else { n = n->rightChild; }
				break;
			}
			default:
				REGEN_ERROR("Invalid state " << n->state << ".");
				REGEN_ASSERT(0);//should not be reached
				return false;
		}
	}
	return false;
}

void BuddyAllocator::clear() {
	auto *clearedTree = createNode(0u, buddyTree_->size, nullptr);
	clear(buddyTree_);
	buddyTree_ = clearedTree;
}

void BuddyAllocator::clear(BuddyNode *n) {
	std::stack<BuddyNode *> stack;
	stack.push(n);
	while (!stack.empty()) {
		BuddyNode *node = stack.top();
		stack.pop();
		if (node->leftChild) stack.push(node->leftChild);
		if (node->rightChild) stack.push(node->rightChild);
		delete node;
	}
}

void BuddyAllocator::free(unsigned int address) {
	// find the node that allocates given address
	BuddyNode *t = buddyTree_;
	while (t->rightChild) {
		t = (address >= t->rightChild->address) ? t->rightChild : t->leftChild;
	}

	// mark as free space
	t->state = FREE;
	t->maxSpace = t->size;
	if (t->leftChild) {
		deleteNode(t->leftChild);
		t->leftChild = nullptr;
	}
	if (t->rightChild) {
		deleteNode(t->rightChild);
		t->rightChild = nullptr;
	}
	// join parents with both childs becoming FREE with this call
	while (t->parent) {
		if (t->parent->leftChild->state == FREE &&
			t->parent->rightChild->state == FREE) {
			t = t->parent;
			deleteNode(t->leftChild);
			deleteNode(t->rightChild);
			t->leftChild = nullptr;
			t->rightChild = nullptr;
			// mark as free space
			t->state = FREE;
			t->maxSpace = t->size;
		} else {
			computeMaxSpace(t->parent);
			break;
		}
	}
}

BuddyAllocator::BuddyNode* BuddyAllocator::createNode(
				unsigned int address,
				unsigned int size,
				BuddyNode *parent) {
	if (nodePool_.empty()) {
		return new BuddyNode(address, size, parent);
	} else {
		BuddyNode *n = nodePool_.top();
		nodePool_.pop();
		n->address = address;
		n->size = size;
		n->maxSpace = size;
		n->parent = parent;
		return n;
	}
}

void BuddyAllocator::deleteNode(BuddyNode *n) {
	n->leftChild = nullptr;
	n->rightChild = nullptr;
	n->parent = nullptr;
	n->state = FREE;
	nodePool_.push(n);
}
