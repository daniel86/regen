#include <stack>
#include <chrono>
#include <limits>
#include <unordered_set>

#include "quad-tree.h"
#include "regen/math/simd.h"

#define QUAD_TREE_DEBUG_TIME
//#define QUAD_TREE_DISABLE_SIMD

#ifdef QUAD_TREE_DEBUG_TIME
#include "regen/gl-types/queries/elapsed-time.h"
#endif

using namespace regen;

namespace regen {
	static constexpr bool QUAD_TREE_SQUARED = true;
	static constexpr bool QUAD_TREE_EVER_GROWING = true;
	static constexpr bool QUAD_TREE_3D_BATCHING = true;
	static constexpr bool QUAD_TREE_MASK_EARLY_EXIT = true;
	static constexpr bool QUAD_TREE_DEFERRED_BATCH_STORE = false;
	static constexpr bool QUAD_TREE_DEBUG_TESTS = false;
}

namespace regen {
	/**
	 * Data structure for quad tree traversal.
	 */
	struct QuadTreeTraversal {
		QuadTree *tree;
		const BoundingShape *shape;
		const OrthogonalProjection *projection;
		Vec2f basePoint;

		uint32_t numQueuedItems_ = 0;
		uint32_t numSucceedingItems_ = 0;
		uint32_t currIdx_ = 0;
		uint32_t nextIdx_ = 1;
		// only used for debug purpose
		uint32_t num2DTests_ = 0;
		uint32_t num3DTests_ = 0;

		uint32_t traversalMask;
		ref_ptr<BatchedIntersectionTest> batchTest3D;
		HitBuffer hits;

		#ifndef QUAD_TREE_DISABLE_SIMD
		// batch processing result masks
		uint32_t batchCounter_ = 0;
		AlignedArray<uint8_t> batchResults_;
		// batch processing bounds
		BatchOf_float batchBoundsMinX;
		BatchOf_float batchBoundsMinY;
		BatchOf_float batchBoundsMaxX;
		BatchOf_float batchBoundsMaxY;
		#endif
		// Stores bounds of nodes that are queued for testing.
		AlignedArray<float> queuedMinX_;
		AlignedArray<float> queuedMinY_;
		AlignedArray<float> queuedMaxX_;
		AlignedArray<float> queuedMaxY_;
		AlignedArray<uint32_t> successorIdx_;
		AlignedArray<uint32_t> queuedNodes_[2];
	};

	struct QuadTree::Private {
		Private() {}

		static inline void addNodeToQueue(QuadTreeTraversal &td,
					const AlignedArray<uint32_t> &nodeArray,
					Node *node, uint32_t queueIdx) {
			const Vec2f &min = node->bounds.min;
			const Vec2f &max = node->bounds.max;
			nodeArray[queueIdx] = node->nodeIdx;
			td.queuedMinX_[queueIdx] = min.x;
			td.queuedMinY_[queueIdx] = min.y;
			td.queuedMaxX_[queueIdx] = max.x;
			td.queuedMaxY_[queueIdx] = max.y;
		}

		template <TestMode_3D TestMode3D>
		static void updateIntersections(
			QuadTreeTraversal &td, const OrthogonalProjection &projection);

		template <TestMode_3D TestMode3D> static void processLeafNode(QuadTreeTraversal &td, Node *leaf);
		template <TestMode_3D TestMode3D> static void processSuccessors(QuadTreeTraversal &td);
		static void testNodesAndSphere(QuadTreeTraversal &td);
		template<TestMode_3D TestMode3D>
		static void intersectionLoop_sphere(QuadTreeTraversal &td);

		template<uint32_t NumAxes> static void testNodesAndAxes(QuadTreeTraversal &td);
		template<uint32_t NumAxes, TestMode_3D TestMode3D>
		static void intersectionLoop(QuadTreeTraversal &td);
	};
}

QuadTree::QuadTree()
		: newBounds_(Bounds<Vec2f>::create(0, 0)),
		  priv_(new Private()) {
}

QuadTree::~QuadTree() {
	shapeToItem_.clear();
	for (auto item: newItems_) {
		delete item;
	}
	newItems_.clear();
	while (!itemPool_.empty()) {
		delete itemPool_.top();
		itemPool_.pop();
	}
	root_ = nullptr;
	for (uint32_t nodeIdx=0; nodeIdx < nodes_.size(); nodeIdx++) {
		delete nodes_[nodeIdx];
	}
	nodes_.clear();
	delete priv_;
}

unsigned int QuadTree::numShapes() const {
	if (!root_) {
		return 0;
	}
	std::stack<const Node *> stack;
	std::unordered_set<uint32_t> shapes;
	stack.push(root_);
	while (!stack.empty()) {
		auto *node = stack.top();
		stack.pop();
		for (uint32_t itemIdx: node->shapes) {
			shapes.insert(itemIdx);
		}
		if (!node->isLeaf()) {
			for (int i = 0; i < 4; i++) {
				stack.push(nodes_[node->childrenIdx[i]]);
			}
		}
	}
	return shapes.size();
}

QuadTree::Node *QuadTree::createNode(const Vec2f &min, const Vec2f &max) {
	if (nodePool_.empty()) {
		Node *node = new Node(min, max);
		node->nodeIdx = nodes_.size();
		nodes_.push_back(node);
		return node;
	} else {
		auto *node = nodePool_.top();
		nodePool_.pop();
		node->bounds.min = min;
		node->bounds.max = max;
		node->center = (min + max) * 0.5f;
		return node;
	}
}

void QuadTree::freeNode(Node *node) { // NOLINT(misc-no-recursion)
	if (!node->isLeaf()) {
		for (int i = 0; i < 4; i++) {
			freeNode(nodes_[node->childrenIdx[i]]);
			node->childrenIdx[i] = -1;
		}
	}
	node->shapes.clear();
	node->parentIdx = -1;
	nodePool_.push(node);
}

QuadTree::Item *QuadTree::createItem(const ref_ptr<BoundingShape> &shape) {
	if (itemPool_.empty()) {
		return new Item(shape);
	} else {
		auto *item = itemPool_.top();
		itemPool_.pop();
		item->shape = shape;
		shape->updateOrthogonalProjection();
		return item;
	}
}

void QuadTree::freeItem(Item *item) {
	item->shape = {};
	item->node = nullptr;
	itemPool_.push(item);
}

void QuadTree::insert(const ref_ptr<BoundingShape> &shape) {
	newItems_.push_back(createItem(shape));
	addToIndex(shape);
}

bool QuadTree::reinsert(uint32_t shapeIdx, bool allowSubdivision) { // NOLINT(misc-no-recursion)
	auto &shape = itemBoundingShapes_[shapeIdx];
	auto nodeIdx = itemNodeIdx_[shapeIdx];
	auto idxInNode = itemIdxInNode_[shapeIdx];
	const OrthogonalProjection &projection = shape->orthoProjection();
	Node *m;
	if (nodeIdx != -1) {
		m = nodes_[nodeIdx];
		removeFromNode(m, idxInNode);
		itemNodeIdx_[shapeIdx] = -1;
	} else {
		m = root_;
	}

	if (m->contains(projection)) {
		// the node fully contains the shape, try to insert it here
		if (insert1(m, shapeIdx, allowSubdivision)) {
			if (itemNodeIdx_[shapeIdx] != static_cast<int>(m->nodeIdx)) collapse(m);
			return true;
		} else {
			itemNodeIdx_[shapeIdx] = m->nodeIdx;
			return false;
		}
	}
	if (m->parentIdx == -1) {
		REGEN_WARN("Shape '" << shape->name() << "." << shape->instanceID() << "' is out of bounds!");
		// the shape is out of bounds, reinsert at root
		itemNodeIdx_[shapeIdx] = m->nodeIdx;
		return false;
	}
	Node *n = nodes_[m->parentIdx];
	while (n->parentIdx != -1 && !n->contains(projection)) {
		n = nodes_[n->parentIdx];
	}
	itemNodeIdx_[shapeIdx] = n->nodeIdx;
	itemIdxInNode_[shapeIdx] = n->shapes.size();
	n->shapes.push_back(shapeIdx);
	if (n->nodeIdx != m->nodeIdx) collapse(m);
	return true;
}

bool QuadTree::insert(Node *node, uint32_t shapeIdx, bool allowSubdivision) { // NOLINT(misc-no-recursion)
	if (!node->intersects(itemBoundingShapes_[shapeIdx]->orthoProjection())) {
		return false;
	} else {
		return insert1(node, shapeIdx, allowSubdivision);
	}
}

bool QuadTree::insert1(Node *node, uint32_t newShapeIdx, bool allowSubdivision) { // NOLINT(misc-no-recursion)
	if (node->isLeaf()) {
		// the node does not have child nodes (yet).
		// the shape can be added to the node in three cases:
		// 1. the node has still not exceeded the maximum number of shapes per node
		// 2. the node has reached the minimum size and cannot be subdivided further
		// 3. the node was just created by subdividing a parent node
		if (!allowSubdivision ||
				node->shapes.size() < subdivisionThreshold_ ||
				node->bounds.size() < minNodeSize_) {
			itemIdxInNode_[newShapeIdx] = node->shapes.size();
			node->shapes.push_back(newShapeIdx);
			itemNodeIdx_[newShapeIdx] = node->nodeIdx;
			return true;
		} else {
			// split the node into four children
			subdivide(node);
			// Keep a copy of the existing shapes in shapesTmp for iteration below.
			node->shapesTmp = std::move(node->shapes);
			node->shapes.clear();
			// reinsert the existing shapes into the new children nodes
			for (uint32_t itemIdx: node->shapesTmp) {
				if (insert(node, itemIdx, true)) {
					continue;
				}
				// failed to insert, try root node instead
				if (insert(root_, itemIdx, true)) {
					continue;
				}
				// still failed, this should never happen
				REGEN_WARN("Shape '" << itemBoundingShapes_[itemIdx]->name() << "." <<
					itemBoundingShapes_[itemIdx]->instanceID() << "' is out of bounds!");
			}
			// also insert the new shape
			return insert(node, newShapeIdx, true);
		}
	} else {
		// the node has child nodes, must insert into (at least) one of them.
		// note: a shape can be inserted into multiple nodes, so we allways need to check all children
		//       (at least on the next level)
		for (auto &childIdx: node->childrenIdx) {
			auto *child = nodes_[childIdx];
			auto &shape = itemBoundingShapes_[newShapeIdx];
			if (child->contains(shape->orthoProjection())) {
				// the child node fully contains the shape, insert it
				if (insert(child, newShapeIdx, allowSubdivision)) return true;
			}
		}
		itemNodeIdx_[newShapeIdx] = node->nodeIdx;
		itemIdxInNode_[newShapeIdx] = node->shapes.size();
		node->shapes.push_back(newShapeIdx);
		return true;
	}
}

void QuadTree::remove(const ref_ptr<BoundingShape> &shape) {
	auto it = shapeToItem_.find(shape.get());
	if (it == shapeToItem_.end()) {
		REGEN_WARN("Shape not found in quad tree.");
		return;
	}
	uint32_t itemIdx = it->second;
	auto *node = nodes_[itemNodeIdx_[itemIdx]];
	removeFromNode(node, itemIdxInNode_[itemIdx]);
	collapse(node);
	itemNodeIdx_[itemIdx] = -1;
}

void QuadTree::removeFromNode(Node *node, uint32_t idxInNode) {
	// Here we remove item at index `idxInNode` from node->shapes array.
	// We do this by moving the last item of the array into the position of the removed item,
	// and then popping the last item.
	uint32_t lastIdx = node->shapes.size() - 1;
	if (idxInNode != lastIdx) {
		// move the last item into the position of the removed item
		uint32_t movedItemIdx = node->shapes[lastIdx];
		node->shapes[idxInNode] = movedItemIdx;
		itemIdxInNode_[movedItemIdx] = idxInNode;
	}
	node->shapes.pop_back();
}

QuadTree::Node* QuadTree::collapse(Node *node) { // NOLINT(misc-no-recursion)
	auto *parent = (node->parentIdx == -1) ? nullptr : nodes_[node->parentIdx];
	if (!parent) return node;
	auto c0 = nodes_[parent->childrenIdx[0]];
	auto c1 = nodes_[parent->childrenIdx[1]];
	auto c2 = nodes_[parent->childrenIdx[2]];
	auto c3 = nodes_[parent->childrenIdx[3]];
	// parent can only be collapsed if all its children are leaf nodes without shapes
	if (!c0->isCollapsable() || !c1->isCollapsable() ||
		!c2->isCollapsable() || !c3->isCollapsable()) return node;

	// free all children nodes
	freeNode(c0);
	freeNode(c1);
	freeNode(c2);
	freeNode(c3);
	for (int i = 0; i < 4; i++) {
		parent->childrenIdx[i] = -1;
	}
	// four nodes are removed, parent turns into a leaf node
	numNodes_ -= 4;
	numLeaves_ -= 3;
	// continue collapsing the parent node
	return collapse(parent);
}

void QuadTree::subdivide(Node *node) {
	// Subdivide the node into four children.
	auto center = (node->bounds.min + node->bounds.max) * 0.5f;
	Node *child;
	// bottom-left
	child = createNode(node->bounds.min, center);
	child->parentIdx = node->nodeIdx;
	node->childrenIdx[0] = child->nodeIdx;
	// bottom-right
	child = createNode(Vec2f(center.x, node->bounds.min.y), Vec2f(node->bounds.max.x, center.y));
	child->parentIdx = node->nodeIdx;
	node->childrenIdx[1] = child->nodeIdx;
	// top-right
	child = createNode(center, node->bounds.max);
	child->parentIdx = node->nodeIdx;
	node->childrenIdx[2] = child->nodeIdx;
	// top-left
	child = createNode(Vec2f(node->bounds.min.x, center.y), Vec2f(center.x, node->bounds.max.y));
	child->parentIdx = node->nodeIdx;
	node->childrenIdx[3] = child->nodeIdx;
	// four new nodes created, parent turns into an internal node
	numNodes_ += 4;
	numLeaves_ += 3;
}

inline ref_ptr<BoundingShape> initShape(const ref_ptr<BoundingShape> &shape) {
	shape->updateTransform(false);
	return shape;
}

QuadTree::Item::Item(const ref_ptr<BoundingShape> &shape) :
		shape(initShape(shape)) {
	shape->updateOrthogonalProjection();
}

QuadTree::Node::Node(const Vec2f &min, const Vec2f &max)
		: bounds(Bounds<Vec2f>::create(min, max)),
		  center((min + max) * 0.5f)
{
	for (int i = 0; i < 4; i++) {
		childrenIdx[i] = -1;
	}
}

QuadTree::Node::~Node() = default;

bool QuadTree::Node::isLeaf() const {
	return childrenIdx[0] == -1;
}

bool QuadTree::Node::isCollapsable() const {
	return childrenIdx[0] == -1 && shapes.empty();
}

bool QuadTree::Node::contains(const OrthogonalProjection &projection) const {
	return bounds.contains(projection.bounds);
}

bool QuadTree::Node::intersects(const OrthogonalProjection &projection) const {
	if (projection.type == OrthogonalProjection::Type::CIRCLE) {
		const float radiusSqr = projection.points[1].x; // = radius * radius
		const auto &p = projection.points[0];
		// Calculate the squared distance from the circle's center to the AABB
		float sqDist = 0.0f;
		if (p.x < bounds.min.x) {
			sqDist += (bounds.min.x - p.x) * (bounds.min.x - p.x);
		} else if (p.x > bounds.max.x) {
			sqDist += (p.x - bounds.max.x) * (p.x - bounds.max.x);
		}
		if (p.y < bounds.min.y) {
			sqDist += (bounds.min.y - p.y) * (bounds.min.y - p.y);
		} else if (p.y > bounds.max.y) {
			sqDist += (p.y - bounds.max.y) * (p.y - bounds.max.y);
		}
		return sqDist < radiusSqr;
	} else {
		// Check for separation along the axes of the shape and the axis-aligned quad
		const uint32_t numAxes = projection.axes.size();
		bool isOutside = false;
		for (uint32_t i = 0; i < numAxes && !isOutside; i++) {
			auto &axis = projection.axes[i];
			const float minXX = bounds.min.x * axis.dir.x;
			const float maxXX = bounds.max.x * axis.dir.x;
			const float minYY = bounds.min.y * axis.dir.y;
			const float maxYY = bounds.max.y * axis.dir.y;

			const float p0 = minXX + minYY;
			const float p1 = maxXX + minYY;
			const float p2 = minXX + maxYY;
			const float p3 = maxXX + maxYY;

			const float projMin = std::min(std::min(p0, p1), std::min(p2, p3));
			const float projMax = std::max(std::max(p0, p1), std::max(p2, p3));

			isOutside = (projMax < axis.min || axis.max < projMin);
		}
		return true;
	}
	return false;
}

bool QuadTree::hasIntersection(const BoundingShape &shape, uint32_t traversalBit) {
	auto &[_, count] = foreachIntersection(shape, traversalBit);
	return count > 0;
}

int QuadTree::numIntersections(const BoundingShape &shape, uint32_t traversalBit) {
	auto &[_, count] = foreachIntersection(shape, traversalBit);
	return count;
}

template<uint32_t NumAxes> __attribute__((noinline))
void QuadTree::Private::testNodesAndAxes(QuadTreeTraversal &td) {
	using namespace regen::simd;
	// Process numQueuedItems_ nodes from the queue, performing an intersection test with the shape's projection;
	// and also writing nodeIdx to successor array if the test succeeds.
	const auto &axes = td.projection->axes;
	int32_t nodeIdx = 0;

#ifndef QUAD_TREE_DISABLE_SIMD
	if constexpr (QUAD_TREE_DEFERRED_BATCH_STORE) {
		td.batchCounter_ = 0;
	}

	for (; nodeIdx + RegisterWidth <= static_cast<int32_t>(td.numQueuedItems_); nodeIdx += RegisterWidth) {
		// Load the bounds of the node at nodeIdx into the SIMD registers
		td.batchBoundsMinX = BatchOf_float::loadAligned(td.queuedMinX_.data() + nodeIdx);
		td.batchBoundsMinY = BatchOf_float::loadAligned(td.queuedMinY_.data() + nodeIdx);
		td.batchBoundsMaxX = BatchOf_float::loadAligned(td.queuedMaxX_.data() + nodeIdx);
		td.batchBoundsMaxY = BatchOf_float::loadAligned(td.queuedMaxY_.data() + nodeIdx);

		uint8_t mask = RegisterMask;

		for (uint32_t axisIdx = 0; axisIdx < NumAxes; ++axisIdx) {
			// d_i = corners[i].dot(axis.dir);
			const BatchOf_Vec2f axisDir =
				BatchOf_Vec2f::fromScalar(td.projection->axes[axisIdx].dir);
			BatchOf_float d0 =
				(td.batchBoundsMinX * axisDir.x) +
				(td.batchBoundsMinY * axisDir.y);
			BatchOf_float d1 =
				(td.batchBoundsMaxX * axisDir.x) +
				(td.batchBoundsMinY * axisDir.y);
			BatchOf_float d2 =
				(td.batchBoundsMinX * axisDir.x) +
				(td.batchBoundsMaxY * axisDir.y);
			BatchOf_float d3 =
				(td.batchBoundsMaxX * axisDir.x) +
				(td.batchBoundsMaxY * axisDir.y);

			// mm = min{d0, d1, d2, d3}
			Register min = min_ps(min_ps(d0.c, d1.c), min_ps(d2.c, d3.c));
			// mm = max{d0, d1, d2, d3}
			Register max = max_ps(max_ps(d0.c, d1.c), max_ps(d2.c, d3.c));
			// Compute `(max_n < axis.min) || (axis.max < min_n)`
			Register sep = cmp_or(
				cmp_lt(set1_ps(td.projection->axes[axisIdx].max), min),
				cmp_lt(max, set1_ps(td.projection->axes[axisIdx].min)));

			// Convert mask to bits
			int sepMask = movemask_ps(sep); // 1 = separated
			mask &= ~sepMask; // Clear bits in intersectMask where sepMask is 1
			if constexpr (QUAD_TREE_MASK_EARLY_EXIT) {
				if (mask == 0) return;
			}
		}
		if constexpr (QUAD_TREE_DEFERRED_BATCH_STORE) {
			td.batchResults_[td.batchCounter_++] = mask;
		} else {
			// add nodeIdx to successorIdx_ for each bit set in intersectMask
			while (mask) {
				int bitIndex = simd::nextBitIndex<uint8_t>(mask);
				td.successorIdx_[td.numSucceedingItems_++] = nodeIdx + bitIndex;
			}
		}
	}

	if constexpr (QUAD_TREE_DEFERRED_BATCH_STORE) {
		for (int32_t batchIdx = 0; batchIdx < td.batchCounter_; ++batchIdx) {
			uint8_t mask = td.batchResults_[batchIdx];
			int32_t startIdx = batchIdx * RegisterWidth;
			while (mask) {
				int bitIndex = simd::nextBitIndex<uint8_t>(mask);
				td.successorIdx_[td.numSucceedingItems_++] = startIdx + bitIndex;
			}
		}
		td.batchCounter_ = 0;
	}
#endif

	// Process remaining nodes scalar way
	for (; nodeIdx < static_cast<int32_t>(td.numQueuedItems_); nodeIdx++) {
		const float minX = td.queuedMinX_[nodeIdx];
		const float minY = td.queuedMinY_[nodeIdx];
		const float maxX = td.queuedMaxX_[nodeIdx];
		const float maxY = td.queuedMaxY_[nodeIdx];

		bool hasIntersection = true;
		for (uint32_t axisIdx = 0; axisIdx < NumAxes; ++axisIdx) {
			auto &axis = axes[axisIdx];
			const float minXX = minX * axis.dir.x;
			const float maxXX = maxX * axis.dir.x;
			const float minYY = minY * axis.dir.y;
			const float maxYY = maxY * axis.dir.y;

			const float p0 = minXX + minYY;
			const float p1 = maxXX + minYY;
			const float p2 = minXX + maxYY;
			const float p3 = maxXX + maxYY;

			const float projMin = std::min(std::min(p0, p1), std::min(p2, p3));
			const float projMax = std::max(std::max(p0, p1), std::max(p2, p3));

			if (projMax < axis.min || axis.max < projMin) {
				hasIntersection = false; break;
			}
		}

		// write to successor array
		td.successorIdx_[td.numSucceedingItems_] = nodeIdx;
		// but only increment the count if the projection intersects with the node
		td.numSucceedingItems_ += static_cast<int32_t>(hasIntersection);
	}
	if constexpr(QUAD_TREE_DEBUG_TESTS) {
		td.num2DTests_ += td.numQueuedItems_;
	}
	td.numQueuedItems_ = 0;
}

__attribute__((noinline))
void QuadTree::Private::testNodesAndSphere(QuadTreeTraversal &td) {
	// Process numQueuedItems_ nodes from the queue, performing an intersection test with the shape's projection;
	// and also writing nodeIdx to successor array if the test succeeds.
	const auto &radiusSqr = td.projection->points[1].x; // = radius * radius
	const auto &center = td.projection->points[0];
	int32_t nodeIdx = 0;

#ifndef QUAD_TREE_DISABLE_SIMD
	using namespace regen::simd;
	if constexpr (QUAD_TREE_DEFERRED_BATCH_STORE) {
		td.batchCounter_ = 0;
	}

	BatchOf_float r_centerX = BatchOf_float::fromScalar(center.x);
	BatchOf_float r_centerY = BatchOf_float::fromScalar(center.y);
	BatchOf_float r_radius = BatchOf_float::fromScalar(radiusSqr);

	for (; nodeIdx + RegisterWidth <= static_cast<int32_t>(td.numQueuedItems_); nodeIdx += RegisterWidth) {
		// Load the bounds of the node at nodeIdx into the SIMD registers
		td.batchBoundsMinX = BatchOf_float::loadAligned(td.queuedMinX_.data() + nodeIdx);
		td.batchBoundsMinY = BatchOf_float::loadAligned(td.queuedMinY_.data() + nodeIdx);
		td.batchBoundsMaxX = BatchOf_float::loadAligned(td.queuedMaxX_.data() + nodeIdx);
		td.batchBoundsMaxY = BatchOf_float::loadAligned(td.queuedMaxY_.data() + nodeIdx);

		uint8_t mask = RegisterMask;
		// Compute distance along X
		Register distL = sub_ps(td.batchBoundsMinX.c, r_centerX.c);
		Register distR = sub_ps(r_centerX.c, td.batchBoundsMaxX.c);
		Register distX = cmp_or(
			cmp_and(cmp_lt(r_centerX.c, td.batchBoundsMinX.c), distL),
			cmp_and(cmp_gt(r_centerX.c, td.batchBoundsMaxX.c), distR));

		// Compute distance along Y
		distL = sub_ps(td.batchBoundsMinY.c, r_centerY.c);
		distR = sub_ps(r_centerY.c, td.batchBoundsMaxY.c);
		Register distY = cmp_or(
			cmp_and(cmp_lt(r_centerY.c, td.batchBoundsMinY.c), distL),
			cmp_and(cmp_gt(r_centerY.c, td.batchBoundsMaxY.c), distR));

		// Compute total squared distance
		Register sqDist = add_ps(mul_ps(distX, distX), mul_ps(distY, distY));
		// Finally, compare against radius², and push nodes that intersect
		Register sep = cmp_lt(sqDist, r_radius.c);

		// Convert mask to bits
		int sepMask = movemask_ps(sep); // 1 = separated
		mask &= ~sepMask; // Clear bits in intersectMask where sepMask is 1
		if constexpr (QUAD_TREE_DEFERRED_BATCH_STORE) {
			td.batchResults_[td.batchCounter_++] = mask;
		} else {
			// add nodeIdx to successorIdx_ for each bit set in intersectMask
			while (mask) {
				int bitIndex = simd::nextBitIndex<uint8_t>(mask);
				td.successorIdx_[td.numSucceedingItems_++] = nodeIdx + bitIndex;
			}
		}
	}

	if constexpr (QUAD_TREE_DEFERRED_BATCH_STORE) {
		for (int32_t batchIdx = 0; batchIdx < td.batchCounter_; ++batchIdx) {
			uint8_t mask = td.batchResults_[batchIdx];
			int32_t startIdx = batchIdx * RegisterWidth;
			while (mask) {
				int bitIndex = simd::nextBitIndex<uint8_t>(mask);
				td.successorIdx_[td.numSucceedingItems_++] = startIdx + bitIndex;
			}
		}
		td.batchCounter_ = 0;
	}
#endif

	// Process remaining nodes scalar way
	for (; nodeIdx < static_cast<int32_t>(td.numQueuedItems_); nodeIdx++) {
		float minX = td.queuedMinX_[nodeIdx];
		float maxX = td.queuedMaxX_[nodeIdx];
		float minY = td.queuedMinY_[nodeIdx];
		float maxY = td.queuedMaxY_[nodeIdx];

		// Calculate the squared distance from the circle's center to the AABB
		float sqDist = 0.0f;
		if (center.x < minX) {
			sqDist += (minX - center.x) * (minX - center.x);
		} else if (center.x > maxX) {
			sqDist += (center.x - maxX) * (center.x - maxX);
		}
		if (center.y < minY) {
			sqDist += (minY - center.y) * (minY - center.y);
		} else if (center.y > maxY) {
			sqDist += (center.y - maxY) * (center.y - maxY);
		}
		bool hasIntersection = sqDist < radiusSqr;

		// write to successor array
		td.successorIdx_[td.numSucceedingItems_] = nodeIdx;
		// but only increment the count if the projection intersects with the node
		td.numSucceedingItems_ += static_cast<int32_t>(hasIntersection);
	}
	td.numQueuedItems_ = 0;
}

template<uint32_t NumAxes, QuadTree::TestMode_3D TestMode3D>
void QuadTree::Private::intersectionLoop(QuadTreeTraversal &td) {
	// Process the queued nodes, performing an intersection test with the shape's projection;
	// and also writing nodeIdx to successor array if the test succeeds.
	while (td.numQueuedItems_ > 0) {
		td.currIdx_ = td.nextIdx_;
		td.nextIdx_ = 1 - td.nextIdx_;
		testNodesAndAxes<NumAxes>(td);
		processSuccessors<TestMode3D>(td);
	}
}

template<QuadTree::TestMode_3D TestMode3D>
void QuadTree::Private::intersectionLoop_sphere(QuadTreeTraversal &td) {
	while (td.numQueuedItems_ > 0) {
		td.currIdx_ = td.nextIdx_;
		td.nextIdx_ = 1 - td.nextIdx_;
		testNodesAndSphere(td);
		processSuccessors<TestMode3D>(td);
	}
}

template <QuadTree::TestMode_3D TestMode3D>
void QuadTree::Private::processSuccessors(QuadTreeTraversal &td) {
	// this function implements a scalar loop over all nodes that passed the intersection test
	// in the current iteration:
	// (1) if inner node, then add children to the queue for the next iteration.
	// (2) if leaf node, then process the shapes in the node.
	const auto &treeNodes = td.tree->nodes_;
	const auto &currArray = td.queuedNodes_[td.currIdx_];
	const auto &nextArray = td.queuedNodes_[td.nextIdx_];

	for (uint32_t i = 0; i < td.numSucceedingItems_; i++) {
		auto successorIdx = td.successorIdx_[i];
		auto successor = treeNodes[currArray[successorIdx]];

		if (successor->shapes.size() > 0) {
			processLeafNode<TestMode3D>(td, successor);
		}
		if (!successor->isLeaf()) {
			// add child nodes to the queue for the next iteration
			addNodeToQueue(td, nextArray, treeNodes[successor->childrenIdx[0]], td.numQueuedItems_++);
			addNodeToQueue(td, nextArray, treeNodes[successor->childrenIdx[1]], td.numQueuedItems_++);
			addNodeToQueue(td, nextArray, treeNodes[successor->childrenIdx[2]], td.numQueuedItems_++);
			addNodeToQueue(td, nextArray, treeNodes[successor->childrenIdx[3]], td.numQueuedItems_++);
		}
	}
	td.numSucceedingItems_ = 0;
}

template <QuadTree::TestMode_3D TestMode3D> __attribute__((noinline))
void QuadTree::Private::processLeafNode(QuadTreeTraversal &td, Node *leaf) {
	// 3D intersection test with the shapes in the node.
	// However, we do not always perform the intersection test, depending
	// on the test mode configured for the quad tree.
	// Also we submit each shape to a batched intersection tester,
	// which will perform the actual intersection tests later in a batch
	// (latest when endFrame is called on the quad tree).

	const uint32_t traversalMask = td.traversalMask;
	auto &itemShapes = td.tree->itemBoundingShapes_;

	if constexpr (TestMode3D == QUAD_TREE_3D_TEST_NONE) {
		// no intersection test, just record the hit
		for (uint32_t itemIdx: leaf->shapes) {
			const BoundingShape &shape = *itemShapes[itemIdx].get();
			if ((shape.traversalMask() & traversalMask) != 0) {
				td.hits.push(itemIdx);
			}
		}
	}
	if constexpr (TestMode3D == QUAD_TREE_3D_TEST_ALL) {
		// test all shapes, even if they are not close to the shape's projection origin
		for (uint32_t itemIdx: leaf->shapes) {
			const BoundingShape &shape = *itemShapes[itemIdx].get();
			if ((shape.traversalMask() & traversalMask) == 0) continue;
			if constexpr (QUAD_TREE_3D_BATCHING) {
				td.batchTest3D->push(shape, itemIdx);
			} else {
				if (td.shape->hasIntersectionWith(shape)) {
					td.hits.push(itemIdx);
				}
			}
			if constexpr(QUAD_TREE_DEBUG_TESTS) {
				td.num3DTests_ += 1;
			}
		}
	}
	if constexpr (TestMode3D == QUAD_TREE_3D_TEST_CLOSEST) {
		// heuristic: only test shapes that are close to the shape's projection origin (e.g. camera position)
		// This is a good approach because:
		//     (1) shapes that are close use higher level of detail -> more expensive to draw false positives
		//     (2) most false positives are close to camera position in case camera is above/below the ground level
		float distSq = (td.basePoint - leaf->center).lengthSquared();
		if (distSq > td.tree->closeDistanceSquared_) {
			for (uint32_t itemIdx: leaf->shapes) {
				const BoundingShape &shape = *itemShapes[itemIdx].get();
				if ((shape.traversalMask() & traversalMask) != 0) {
					td.hits.push(itemIdx);
				}
			}
		} else {
			for (uint32_t itemIdx: leaf->shapes) {
				const BoundingShape &shape = *itemShapes[itemIdx].get();
				if ((shape.traversalMask() & traversalMask) == 0) continue;
				if constexpr (QUAD_TREE_3D_BATCHING) {
					td.batchTest3D->push(shape, itemIdx);
				} else {
					if (td.shape->hasIntersectionWith(shape)) {
						td.hits.push(itemIdx);
					}
				}
				if constexpr(QUAD_TREE_DEBUG_TESTS) {
					td.num3DTests_ += 1;
				}
			}
		}
	}
}

template <QuadTree::TestMode_3D TestMode3D>
void QuadTree::Private::updateIntersections(QuadTreeTraversal &td, const OrthogonalProjection &projection) {
	switch (projection.type) {
		case OrthogonalProjection::Type::CIRCLE: [[unlikely]]
			intersectionLoop_sphere<TestMode3D>(td);
			break;
		case OrthogonalProjection::Type::RECTANGLE:
			intersectionLoop<4,TestMode3D>(td);
			break;
		case OrthogonalProjection::Type::CONVEX_HULL: [[likely]]
			switch (projection.axes.size()) {
			case 4: [[unlikely]] intersectionLoop<4,TestMode3D>(td); break;
			case 5: [[unlikely]] intersectionLoop<5,TestMode3D>(td); break;
			case 6: intersectionLoop<6,TestMode3D>(td); break;
			case 7: [[likely]] intersectionLoop<7,TestMode3D>(td); break;
			case 8: [[likely]] intersectionLoop<8,TestMode3D>(td); break;
			case 9: [[likely]] intersectionLoop<9,TestMode3D>(td); break;
			case 10: intersectionLoop<10,TestMode3D>(td); break;
			default:
					REGEN_ERROR("unsupported number of axes for intersection test: " << projection.axes.size());
					break;
			}
			break;
	}
}

HitBuffer& QuadTree::foreachIntersection(const BoundingShape &shape, uint32_t mask) {
	// use thread-local traversal data to avoid reallocations,
	// and synchronization between threads.
	thread_local QuadTreeTraversal td;
	if (td.tree != this) {
		// make sure the traversal data belongs to this quad tree
		// (in case of multiple quad trees in the application).
		// This also handles the very first initialization of td.
		td.tree = this;
		if constexpr (QUAD_TREE_3D_BATCHING) {
			if (!td.batchTest3D) {
				td.batchTest3D = ref_ptr<BatchedIntersectionTest>::alloc();
				td.batchTest3D->setHitBuffer(&td.hits);
			}
			td.batchTest3D->setIndexedShapes(&itemBoundingShapes_);
		}
	}
	td.hits.reset();

	if (!root_) return td.hits;
	if (root_->isLeaf() && root_->shapes.empty()) return td.hits;

	auto &origin = shape.tfOrigin();
	// project the shape onto the xz-plane for faster intersection tests
	// with the quad tree nodes.
	const OrthogonalProjection &projection = shape.orthoProjection();

	if constexpr (QUAD_TREE_3D_BATCHING) {
		td.batchTest3D->setBatchCapacity(batchSize3D_);
	}
	td.numQueuedItems_ = 0;
	td.numSucceedingItems_ = 0;
	td.shape = &shape;
	td.projection = &projection;
	td.basePoint = Vec2f(origin.x, origin.z);
	td.traversalMask = mask;

	if (nextBufferSize_ > td.queuedMinX_.size()) {
		td.queuedNodes_[0].resize(nextBufferSize_);
		td.queuedNodes_[1].resize(nextBufferSize_);
		td.queuedMinX_.resize(nextBufferSize_);
		td.queuedMinY_.resize(nextBufferSize_);
		td.queuedMaxX_.resize(nextBufferSize_);
		td.queuedMaxY_.resize(nextBufferSize_);
		td.successorIdx_.resize(nextBufferSize_);
		if constexpr (QUAD_TREE_DEFERRED_BATCH_STORE) {
			td.batchResults_.resize(nextBufferSize_ / simd::RegisterWidth + 1);
		}
	}
	if constexpr(QUAD_TREE_DEBUG_TESTS) {
		td.num2DTests_ = 0;
		td.num3DTests_ = 0;
	}

	if (td.hits.data.capacity() < itemNodeIdx_.size()) {
		td.hits.resize(itemNodeIdx_.size());
	}

	if constexpr (QUAD_TREE_3D_BATCHING) {
		td.batchTest3D->beginFrame(shape);
	}
	priv_->addNodeToQueue(td, td.queuedNodes_[td.nextIdx_], root_, td.numQueuedItems_++);

	switch (testMode3D_) {
		case QUAD_TREE_3D_TEST_NONE: [[unlikely]]
			priv_->updateIntersections<QUAD_TREE_3D_TEST_NONE>(td, projection);
			break;
		case QUAD_TREE_3D_TEST_ALL:
			priv_->updateIntersections<QUAD_TREE_3D_TEST_ALL>(td, projection);
			break;
		case QUAD_TREE_3D_TEST_CLOSEST: [[likely]]
			priv_->updateIntersections<QUAD_TREE_3D_TEST_CLOSEST>(td, projection);
			break;
	}
	if constexpr (QUAD_TREE_3D_BATCHING) {
		td.batchTest3D->endFrame();
	}

	if constexpr(QUAD_TREE_DEBUG_TESTS) {
		static uint32_t numFrames = 0;
		static std::vector<uint32_t> numTests_2d;
		static std::vector<uint32_t> numTests_3d;
		numFrames++;
		numTests_2d.push_back(td.num2DTests_);
		numTests_3d.push_back(td.num3DTests_);
		if (numFrames > 1000) {
			uint32_t avg2DTests = 0;
			uint32_t avg3DTests = 0;
			for (size_t i = 0; i < numFrames; i++) {
				avg2DTests += numTests_2d[i];
				avg3DTests += numTests_3d[i];
			}
			avg2DTests /= numFrames;
			avg3DTests /= numFrames;
			REGEN_INFO("QuadTree:"
							   << " #nodes: " << numNodes_
							   << " #leaves: " << numLeaves_
							   << " #avg-2d: " << avg2DTests
							   << " #avg-3d: " << avg3DTests);
			numFrames = 0;
			numTests_2d.clear();
			numTests_3d.clear();
		}
	}

	return td.hits;
}

void QuadTree::update(float dt) {
	static constexpr float maxFloat = std::numeric_limits<float>::lowest();
	static constexpr float minFloat = std::numeric_limits<float>::max();

	if (itemNodeIdx_.empty() && newItems_.empty()) {
		// nothing to do
		return;
	}
	bool hasChanged;
#ifdef QUAD_TREE_DEBUG_TIME
	thread_local ElapsedTimeDebugger elapsedTime("Quad-Tree Update",
		1000, ElapsedTimeDebugger::CPU_ONLY);
	elapsedTime.beginFrame();
	elapsedTime.push("starting");
#endif

	changedItems_.clear();
	newBounds_.min.x = minFloat;
	newBounds_.min.x = minFloat;
	newBounds_.max.x = maxFloat;
	newBounds_.max.y = maxFloat;
	if constexpr(QUAD_TREE_EVER_GROWING) {
		if (root_ != nullptr) {
			// never shrink the root node
			newBounds_.extend(root_->bounds);
		}
	}

	// go through all items and update their geometry and transform, and the new bounds
	for (uint32_t itemIdx = 0; itemIdx < itemNodeIdx_.size(); itemIdx++) {
		auto &shape = itemBoundingShapes_[itemIdx];
		auto nodeIdx = itemNodeIdx_[itemIdx];
		auto idxInNode = itemIdxInNode_[itemIdx];
		OrthogonalProjection &projection = shape->orthoProjection();
		// NOTE: items with traversalMask == 0 are currently disabled,
		//       so we can skip updating them, and remove them from the tree.
		if (shape->traversalMask() == 0) {
			if (nodeIdx != -1) {
				auto &node = nodes_[nodeIdx];
				removeFromNode(node, idxInNode);
				collapse(node);
				itemNodeIdx_[itemIdx] = -1;
			}
			continue;
		}
		hasChanged = shape->updateGeometry();
		hasChanged = shape->updateTransform(hasChanged) || hasChanged;
		if (hasChanged) {
			projection.update(*shape.get());
			changedItems_.push_back(itemIdx);
		}
		newBounds_.extend(projection.bounds);
	}

	// do the same for any additional items added to the tree
	for (const auto &item: newItems_) {
		OrthogonalProjection &projection = item->shape->orthoProjection();
		hasChanged = item->shape->updateGeometry();
		hasChanged = item->shape->updateTransform(hasChanged) || hasChanged;
		if (hasChanged) {
			projection.update(*item->shape.get());
		}
		newBounds_.extend(projection.bounds);
	}
	if constexpr(QUAD_TREE_SQUARED) {
		// make the bounds square
		newBounds_.min.x = std::min(newBounds_.min.x, newBounds_.min.y);
		newBounds_.min.y = newBounds_.min.x;
		newBounds_.max.x = std::max(newBounds_.max.x, newBounds_.max.y);
		newBounds_.max.y = newBounds_.max.x;
	}
#ifdef QUAD_TREE_DEBUG_TIME
	elapsedTime.push("bounds-update");
#endif

	if (root_ == nullptr || newBounds_ != root_->bounds) {
		// if bounds have changed, re-initialize the tree
		// free the root node and start all over
		if (root_) freeNode(root_);
		root_ = createNode(newBounds_.min, newBounds_.max);
		numNodes_ = 1;
		numLeaves_ = 1;

		for (uint32_t itemIdx = 0; itemIdx < itemNodeIdx_.size(); itemIdx++) {
			auto &shape = itemBoundingShapes_[itemIdx];
			if (shape->traversalMask() == 0) continue;
			itemNodeIdx_[itemIdx] = -1; // reset node index
			insert1(root_, itemIdx, true);
		}
	} else {
		// else remove/insert the changed items
		for (uint32_t itemIdx: changedItems_) {
			reinsert(itemIdx, true);
		}
	}
#ifdef QUAD_TREE_DEBUG_TIME
	elapsedTime.push("re-insertions");
#endif

	// finally insert the new items
	for (auto item: newItems_) {
		uint32_t newItemIdx = static_cast<uint32_t>(itemNodeIdx_.size());
		itemIdxInNode_.push_back(0);
		itemNodeIdx_.push_back(-1);
		freeItem(item);

		if (insert1(root_, newItemIdx, true)) {
			shapeToItem_[item->shape.get()] = newItemIdx;
		} else {
			REGEN_WARN("Failed to insert shape into quad tree. This should not happen!");
		}
	}
	newItems_.clear();
#ifdef QUAD_TREE_DEBUG_TIME
	elapsedTime.push("new-insertions");
#endif

	// update buffer sizes for intersection tests.
	// at max we will have num-leaves nodes in the queue.
	// here we will resize the arrays to the next power of two to have sufficient space.
	nextBufferSize_ = (numLeaves_ * 2);

	// make the visibility computations
	updateVisibility(traversalBit_);
#ifdef QUAD_TREE_DEBUG_TIME
	elapsedTime.push("updated-visibility");
	elapsedTime.endFrame();
#endif
}

inline Vec3f toVec3(const Vec2f &v, float y) {
	return {v.x, y, v.y};
}

void QuadTree::debugDraw(DebugInterface &debug) const {
	SpatialIndex::debugDraw(debug);
	// draw lines around the quad tree nodes
	if (!root_) return;
	static const float drawHeight = 5.5f;
	Vec3f lineColor(1, 0, 0);

	// draw the bounds of nodes
	std::stack<const Node *> stack;
	stack.push(root_);
	while (!stack.empty()) {
		const Node *node = stack.top();
		stack.pop();
		debug.drawLine(
				Vec3f(node->bounds.min.x, drawHeight, node->bounds.min.y),
				Vec3f(node->bounds.max.x, drawHeight, node->bounds.min.y),
				lineColor);
		debug.drawLine(
				Vec3f(node->bounds.max.x, drawHeight, node->bounds.min.y),
				Vec3f(node->bounds.max.x, drawHeight, node->bounds.max.y),
				lineColor);
		debug.drawLine(
				Vec3f(node->bounds.max.x, drawHeight, node->bounds.max.y),
				Vec3f(node->bounds.min.x, drawHeight, node->bounds.max.y),
				lineColor);
		debug.drawLine(
				Vec3f(node->bounds.min.x, drawHeight, node->bounds.max.y),
				Vec3f(node->bounds.min.x, drawHeight, node->bounds.min.y),
				lineColor);
		if (!node->isLeaf()) {
			for (int i = 0; i < 4; i++) {
				stack.push(nodes_[node->childrenIdx[i]]);
			}
		}
	}

	// draw 2d projections of the shapes
	lineColor = Vec3f(0, 1, 0);
	const float h = 5.1f;
	for (uint32_t itemIdx = 0; itemIdx < itemBoundingShapes_.size(); itemIdx++) {
		auto &shape = itemBoundingShapes_[itemIdx];
		if (shape->traversalMask() == 0) continue;
		auto &projection = shape->orthoProjection();
		auto &points = projection.points;
		switch (projection.type) {
			case OrthogonalProjection::Type::CIRCLE: {
				auto radius = std::sqrt(points[1].x);
				debug.drawCircle(toVec3(points[0], h), radius, lineColor);
				break;
			}
			default:
				for (size_t i = 0; i < points.size(); i++) {
					debug.drawLine(
							toVec3(points[i], h),
							toVec3(points[(i + 1) % points.size()], h),
							lineColor);
				}
				break;
		}
	}
}
