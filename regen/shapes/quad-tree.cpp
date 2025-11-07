#include <stack>
#include <chrono>
#include <limits>
#include <unordered_set>

#include "quad-tree.h"
#include "regen/math/simd.h"

//#define QUAD_TREE_DEBUG_TESTS
//#define QUAD_TREE_DEBUG_TIME
#define QUAD_TREE_EVER_GROWING
#define QUAD_TREE_SQUARED
//#define QUAD_TREE_DISABLE_SIMD
#define QUAD_TREE_MASK_EARLY_EXIT
//#define QUAD_TREE_DEFERRED_BATCH_STORE

#ifdef QUAD_TREE_DISABLE_SIMD
	#ifdef QUAD_TREE_DEFERRED_BATCH_STORE
	#undef QUAD_TREE_DEFERRED_BATCH_STORE
	#endif // QUAD_TREE_DEFERRED_BATCH_STORE
#endif


#ifdef QUAD_TREE_DEBUG_TIME
#include "regen/gl-types/queries/elapsed-time.h"
#endif

// NOTE: this piece of code is performance critical! For many execution paths:
// - avoid the use of std::set, std::unordered_set, std::map, std::unordered_map, etc. here
// 		- also iteration over these containers is expensive!
// - avoid lambda functions
// - avoid alloc/free

using namespace regen;

namespace regen {
	struct QuadTreeTraversal {
		QuadTree *tree;
		const BoundingShape *shape;
		const OrthogonalProjection *projection;
		Vec2f basePoint;
		std::array<Vec2f, 4> corners;
		std::array<float, 4> projections;
		void *userData;
		uint32_t traversalBit = 0;

		void (*callback)(const BoundingShape &, void *);

#ifndef QUAD_TREE_DISABLE_SIMD
		BatchOf_float batchBoundsMinX; // NOLINT(cppcoreguidelines-pro-type-member-init)
		BatchOf_float batchBoundsMinY; // NOLINT(cppcoreguidelines-pro-type-member-init)
		BatchOf_float batchBoundsMaxX; // NOLINT(cppcoreguidelines-pro-type-member-init)
		BatchOf_float batchBoundsMaxY; // NOLINT(cppcoreguidelines-pro-type-member-init)
#endif
	};

	struct QuadTree::Private {
		// Stores bounds of nodes that are queued for testing.
		AlignedArray<float> queuedMinX_;
		AlignedArray<float> queuedMinY_;
		AlignedArray<float> queuedMaxX_;
		AlignedArray<float> queuedMaxY_;
		AlignedArray<uint32_t> successorIdx_;
		AlignedArray<Node *> queuedNodes_[2];
#ifndef QUAD_TREE_DISABLE_SIMD
#ifdef QUAD_TREE_DEFERRED_BATCH_STORE
		AlignedArray<uint8_t> batchResults_;
#endif
#endif
		uint32_t numQueuedItems_ = 0;
		uint32_t numSucceedingItems_ = 0;
		uint32_t currIdx_ = 0;
		uint32_t nextIdx_ = 1;
#ifdef QUAD_TREE_DEBUG_TESTS
		uint32_t num2DTests_ = 0;
		uint32_t num3DTests_ = 0;
#endif

		Private() {}

		void addNodeToQueue(Node *node);
		void processLeafNode(QuadTreeTraversal &td, Node *leaf);
		void processSuccessors(QuadTreeTraversal &td);
		void processQueuedNodes_scalar(QuadTreeTraversal &td, int32_t nodeIdx);
		void processQueuedNodes_sphere(QuadTreeTraversal &td);
		void intersectionLoop_sphere(QuadTreeTraversal &td);
		template<uint32_t NumAxes> void processQueuedNodes(QuadTreeTraversal &td);
		template<uint32_t NumAxes> void intersectionLoop(QuadTreeTraversal &td);
#ifndef QUAD_TREE_DISABLE_SIMD
		template<uint32_t NumAxes> void processAxes_SIMD(QuadTreeTraversal &td, uint8_t &mask);
		void processAxis_SIMD(QuadTreeTraversal &td, uint8_t &mask, int32_t axisIdx);
		void processSphere_SIMD(QuadTreeTraversal &td, uint8_t &mask);
#endif
	};
}

QuadTree::QuadTree()
		: SpatialIndex(),
		  priv_(new Private()),
		  root_(nullptr),
		  newBounds_(0, 0) {
}

QuadTree::~QuadTree() {
	for (auto item: items_) {
		delete item;
	}
	items_.clear();
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
		item->projection.update(*shape.get());
		return item;
	}
}

void QuadTree::freeItem(Item *item) {
	item->shape = {};
	item->projection.points.clear();
	item->projection.axes.clear();
	item->node = nullptr;
	itemPool_.push(item);
}

void QuadTree::insert(const ref_ptr<BoundingShape> &shape) {
	newItems_.push_back(createItem(shape));
	addToIndex(shape);
}

bool QuadTree::reinsert(uint32_t shapeIdx, bool allowSubdivision) { // NOLINT(misc-no-recursion)
	auto &shape = items_[shapeIdx];
	Node *m;
	if (shape->node) {
		m = shape->node;
		removeFromNode(shape->node, shape->idxInNode);
		shape->node = nullptr;
	} else {
		m = root_;
	}

	if (m->contains(shape->projection)) {
		// the node fully contains the shape, try to insert it here
		if (insert1(m, shapeIdx, allowSubdivision)) {
			if (shape->node != m) collapse(m);
			return true;
		} else {
			shape->node = m;
			return false;
		}
	}
	if (m->parentIdx == -1) {
		REGEN_WARN("Shape '" << shape->shape->name() << "." <<
			shape->shape->instanceID() << "' is out of bounds!");
		// the shape is out of bounds, reinsert at root
		shape->node = m;
		return false;
	}
	Node *n = nodes_[m->parentIdx];
	while (n->parentIdx != -1 && !n->contains(shape->projection)) {
		n = nodes_[n->parentIdx];
	}
	shape->node = n;
	shape->idxInNode = n->shapes.size();
	n->shapes.push_back(shapeIdx);
	if (shape->node != m) collapse(m);
	return true;
}

bool QuadTree::insert(Node *node, uint32_t shapeIdx, bool allowSubdivision) { // NOLINT(misc-no-recursion)
	if (!node->intersects(items_[shapeIdx]->projection)) {
		return false;
	} else {
		return insert1(node, shapeIdx, allowSubdivision);
	}
}

bool QuadTree::insert1(Node *node, uint32_t newShapeIdx, bool allowSubdivision) { // NOLINT(misc-no-recursion)
	Item *newShape = items_[newShapeIdx];
	if (node->isLeaf()) {
		// the node does not have child nodes (yet).
		// the shape can be added to the node in three cases:
		// 1. the node has still not exceeded the maximum number of shapes per node
		// 2. the node has reached the minimum size and cannot be subdivided further
		// 3. the node was just created by subdividing a parent node
		if (!allowSubdivision ||
				node->shapes.size() < subdivisionThreshold_ ||
				node->bounds.size() < minNodeSize_) {
			newShape->idxInNode = node->shapes.size();
			node->shapes.push_back(newShapeIdx);
			newShape->node = node;
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
				REGEN_WARN("Shape '" << items_[itemIdx]->shape->name() << "." <<
					items_[itemIdx]->shape->instanceID() << "' is out of bounds!");
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
			if (child->contains(newShape->projection)) {
				// the child node fully contains the shape, insert it
				if (insert(child, newShapeIdx, allowSubdivision)) return true;
			}
		}
		newShape->node = node;
		newShape->idxInNode = node->shapes.size();
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
	auto *item = items_[itemIdx];
	removeFromNode(item->node, item->idxInNode);
	collapse(item->node);
	item->node = nullptr;
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
		items_[movedItemIdx]->idxInNode = idxInNode;
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
		shape(initShape(shape)),
		projection(*shape.get()) {
}

QuadTree::Node::Node(const Vec2f &min, const Vec2f &max) : bounds(min, max), parentIdx(-1) {
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

static inline std::pair<float, float> project(const Bounds<Vec2f> &b, const Vec2f &axis) {
	std::array<Vec2f, 4> corners = {
			Vec2f(b.min.x, b.min.y),
			Vec2f(b.max.x, b.min.y),
			Vec2f(b.min.x, b.max.y),
			Vec2f(b.max.x, b.max.y)
	};
	std::array<float, 4> projections;
	for (int i = 0; i < 4; ++i) projections[i] = corners[i].dot(axis);
	auto [minIt, maxIt] = std::minmax_element(projections.begin(), projections.end());
	return {*minIt, *maxIt};
}

bool QuadTree::Node::intersects(const OrthogonalProjection &projection) const {
	if (projection.type == OrthogonalProjection::Type::CIRCLE) {
		const auto &radiusSqr = projection.points[1].x; // = radius * radius
		const auto &center = projection.points[0];
		// Calculate the squared distance from the circle's center to the AABB
		float sqDist = 0.0f;
		if (center.x < bounds.min.x) {
			sqDist += (bounds.min.x - center.x) * (bounds.min.x - center.x);
		} else if (center.x > bounds.max.x) {
			sqDist += (center.x - bounds.max.x) * (center.x - bounds.max.x);
		}
		if (center.y < bounds.min.y) {
			sqDist += (bounds.min.y - center.y) * (bounds.min.y - center.y);
		} else if (center.y > bounds.max.y) {
			sqDist += (center.y - bounds.max.y) * (center.y - bounds.max.y);
		}
		return sqDist < radiusSqr;
	} else {
		// Check for separation along the axes of the shape and the axis-aligned quad
		for (const auto &axis: projection.axes) {
			auto [minA, maxA] = project(bounds, axis.dir);
			if (maxA < axis.min || axis.max < minA) {
				return false;
			}
		}
		return true;
	}
	return false;
}

static void countIntersections(const BoundingShape &, void *userData) {
	int &counter = *static_cast<int *>(userData);
	counter++;
}

bool QuadTree::hasIntersection(const BoundingShape &shape, uint32_t traversalBit) {
	int count = 0;
	foreachIntersection(shape, countIntersections, &count, traversalBit);
	return count > 0;
}

int QuadTree::numIntersections(const BoundingShape &shape, uint32_t traversalBit) {
	int count = 0;
	foreachIntersection(shape, countIntersections, &count, traversalBit);
	return count;
}

void QuadTree::Private::addNodeToQueue(Node *node) {
	queuedNodes_[nextIdx_][numQueuedItems_] = node;
	queuedMinX_[numQueuedItems_] = node->bounds.min.x;
	queuedMinY_[numQueuedItems_] = node->bounds.min.y;
	queuedMaxX_[numQueuedItems_] = node->bounds.max.x;
	queuedMaxY_[numQueuedItems_] = node->bounds.max.y;
	numQueuedItems_++;
}

#ifndef QUAD_TREE_DISABLE_SIMD
void QuadTree::Private::processSphere_SIMD(
		QuadTreeTraversal &td,
		uint8_t &intersectMask) {
	using namespace regen::simd;

	Register distX, distY;
	{
		const auto &center = td.projection->points[0];
		// Compute distance along X
		Register r_center = set1_ps(center.x);
		Register distL = sub_ps(td.batchBoundsMinX.c, r_center);
		Register distR = sub_ps(r_center, td.batchBoundsMaxX.c);
		Register maskL = cmp_lt(r_center, td.batchBoundsMinX.c);
		Register maskR = cmp_gt(r_center, td.batchBoundsMaxX.c);
		distX = cmp_or(cmp_and(maskL, distL), cmp_and(maskR, distR));

		// Compute distance along Y
		r_center = set1_ps(center.y);
		distL = sub_ps(td.batchBoundsMinY.c, r_center);
		distR = sub_ps(r_center, td.batchBoundsMaxY.c);
		maskL = cmp_lt(r_center, td.batchBoundsMinY.c);
		maskR = cmp_gt(r_center, td.batchBoundsMaxY.c);
		distY = cmp_or(cmp_and(maskL, distL), cmp_and(maskR, distR));
	}

	// Compute total squared distance
	Register sqDist = add_ps(
		mul_ps(distX, distX),
		mul_ps(distY, distY));

	// Load radiusSqr into SIMD register
	const auto &radiusSqr = td.projection->points[1].x; // = radius * radius
	// Finally, compare against radius², and push nodes that intersect
	Register sep = cmp_lt(sqDist, set1_ps(radiusSqr));

	// Convert mask to bits
	uint8_t sepMask = movemask_ps(sep); // 1 = separated
	intersectMask &= ~sepMask; // Clear bits in intersectMask where sepMask is 1
}

void QuadTree::Private::processAxis_SIMD(
		QuadTreeTraversal &td,
		uint8_t &intersectMask,
		int32_t axisIdx) {
	using namespace regen::simd;
	Register min, max;
	{
		// d_i = corners[i].dot(axis.dir);
		BatchOf_Vec2f axisDir(td.projection->axes[axisIdx].dir);
		auto d0 = add_ps(
				mul_ps(td.batchBoundsMinX.c, axisDir.x),
				mul_ps(td.batchBoundsMinY.c, axisDir.y));
		auto d1 = add_ps(
				mul_ps(td.batchBoundsMaxX.c, axisDir.x),
				mul_ps(td.batchBoundsMinY.c, axisDir.y));
		auto d2 = add_ps(
				mul_ps(td.batchBoundsMinX.c, axisDir.x),
				mul_ps(td.batchBoundsMaxY.c, axisDir.y));
		auto d3 = add_ps(
				mul_ps(td.batchBoundsMaxX.c, axisDir.x),
				mul_ps(td.batchBoundsMaxY.c, axisDir.y));

		// min = min{d0, d1, d2, d3}
		min = min_ps(min_ps(d0, d1), min_ps(d2, d3));
		// max = max{d0, d1, d2, d3}
		max = max_ps(max_ps(d0, d1), max_ps(d2, d3));
	}

	// Compute `(max_n < axis.min) || (axis.max < min_n)`
	BatchOf_float axisMin(td.projection->axes[axisIdx].min);
	BatchOf_float axisMax(td.projection->axes[axisIdx].max);
	auto sep = cmp_or(
			cmp_lt(max, axisMin.c),
			cmp_lt(axisMax.c, min));

	// Convert mask to bits
	uint8_t sepMask = movemask_ps(sep); // 1 = separated
	intersectMask &= ~sepMask; // Clear bits in intersectMask where sepMask is 1
}

template<uint32_t NumAxes>
void QuadTree::Private::processAxes_SIMD(QuadTreeTraversal &td, uint8_t &mask) {
	constexpr uint32_t N = NumAxes;
	for (uint32_t i = 0; i < N; ++i) {
		processAxis_SIMD(td, mask, i);
#ifdef QUAD_TREE_MASK_EARLY_EXIT
		if (mask == 0) return;
#endif
	}
}
#endif

template<uint32_t NumAxes>
void QuadTree::Private::processQueuedNodes(QuadTreeTraversal &td) {
	// Process numQueuedItems_ nodes from the queue, performing an intersection test with the shape's projection;
	// and also writing nodeIdx to successor array if the test succeeds.
	int32_t nodeIdx = 0;
#ifndef QUAD_TREE_DISABLE_SIMD
	if (numQueuedItems_ >= 64) {
	#ifdef QUAD_TREE_DEFERRED_BATCH_STORE
		int32_t batchCount = 0;
	#endif

		for (; nodeIdx + regen::simd::RegisterWidth <= static_cast<int32_t>(numQueuedItems_);
			   nodeIdx += regen::simd::RegisterWidth) {
			// Load the bounds of the node at nodeIdx into the SIMD registers
			td.batchBoundsMinX.load_aligned(queuedMinX_.data() + nodeIdx);
			td.batchBoundsMinY.load_aligned(queuedMinY_.data() + nodeIdx);
			td.batchBoundsMaxX.load_aligned(queuedMaxX_.data() + nodeIdx);
			td.batchBoundsMaxY.load_aligned(queuedMaxY_.data() + nodeIdx);

			uint8_t mask = regen::simd::RegisterMask;
			processAxes_SIMD<NumAxes>(td, mask);
#ifdef QUAD_TREE_DEFERRED_BATCH_STORE
			batchResults_[batchCount++] = mask;
#else
			// add nodeIdx to successorIdx_ for each bit set in intersectMask
			while (mask) {
				int bitIndex = __builtin_ctz(mask);
				mask &= (mask - 1);
				successorIdx_[numSucceedingItems_++] = nodeIdx + bitIndex;
			}
#endif
		}

#ifdef QUAD_TREE_DEFERRED_BATCH_STORE
		for (int32_t batchIdx = 0; batchIdx < batchCount; ++batchIdx) {
			uint8_t mask = batchResults_[batchIdx];
			int32_t startIdx = batchIdx * regen::simd::RegisterWidth;
			while (mask) {
				int bitIndex = __builtin_ctz(mask);
				mask &= (mask - 1);
				successorIdx_[numSucceedingItems_++] = startIdx + bitIndex;
			}
		}
#endif
	}
#endif
	processQueuedNodes_scalar(td, nodeIdx);
#ifdef QUAD_TREE_DEBUG_TESTS
	num2DTests_ += numQueuedItems_;
#endif
	numQueuedItems_ = 0;
}

void QuadTree::Private::processQueuedNodes_sphere(QuadTreeTraversal &td) {
	// Process numQueuedItems_ nodes from the queue, performing an intersection test with the shape's projection;
	// and also writing nodeIdx to successor array if the test succeeds.
	int32_t nodeIdx = 0;
	const auto &radiusSqr = td.projection->points[1].x; // = radius * radius
	const auto &center = td.projection->points[0];

#ifndef QUAD_TREE_DISABLE_SIMD
	if (numQueuedItems_ >= 64) {
	#ifdef QUAD_TREE_DEFERRED_BATCH_STORE
		int32_t batchCount = 0;
	#endif

		for (; nodeIdx + regen::simd::RegisterWidth <= static_cast<int32_t>(numQueuedItems_);
			   nodeIdx += regen::simd::RegisterWidth) {
			// Load the bounds of the node at nodeIdx into the SIMD registers
			td.batchBoundsMinX.load_aligned(queuedMinX_.data() + nodeIdx);
			td.batchBoundsMinY.load_aligned(queuedMinY_.data() + nodeIdx);
			td.batchBoundsMaxX.load_aligned(queuedMaxX_.data() + nodeIdx);
			td.batchBoundsMaxY.load_aligned(queuedMaxY_.data() + nodeIdx);

			uint8_t mask = regen::simd::RegisterMask;
			processSphere_SIMD(td, mask);
#ifdef QUAD_TREE_DEFERRED_BATCH_STORE
			batchResults_[batchCount++] = mask;
#else
			// add nodeIdx to successorIdx_ for each bit set in intersectMask
			while (mask) {
				int bitIndex = __builtin_ctz(mask);
				mask &= (mask - 1);
				successorIdx_[numSucceedingItems_++] = nodeIdx + bitIndex;
			}
#endif
		}

#ifdef QUAD_TREE_DEFERRED_BATCH_STORE
		for (int32_t batchIdx = 0; batchIdx < batchCount; ++batchIdx) {
			uint8_t mask = batchResults_[batchIdx];
			int32_t startIdx = batchIdx * regen::simd::RegisterWidth;
			while (mask) {
				int bitIndex = __builtin_ctz(mask);
				mask &= (mask - 1);
				successorIdx_[numSucceedingItems_++] = startIdx + bitIndex;
			}
		}
#endif
	}
#endif

	for (; nodeIdx < static_cast<int32_t>(numQueuedItems_); nodeIdx++) {
		float minX = queuedMinX_[nodeIdx];
		float maxX = queuedMaxX_[nodeIdx];
		float minY = queuedMinY_[nodeIdx];
		float maxY = queuedMaxY_[nodeIdx];

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
		successorIdx_[numSucceedingItems_] = nodeIdx;
		// but only increment the count if the projection intersects with the node
		numSucceedingItems_ += int32_t(hasIntersection);
	}
	numQueuedItems_ = 0;
}

template<uint32_t NumAxes>
void QuadTree::Private::intersectionLoop(QuadTreeTraversal &td) {
	// Process the queued nodes, performing an intersection test with the shape's projection;
	// and also writing nodeIdx to successor array if the test succeeds.
	while (numQueuedItems_ > 0) {
		currIdx_ = nextIdx_;
		nextIdx_ = 1 - nextIdx_;
		processQueuedNodes<NumAxes>(td);
		processSuccessors(td);
	}
}

void QuadTree::Private::intersectionLoop_sphere(QuadTreeTraversal &td) {
	while (numQueuedItems_ > 0) {
		currIdx_ = nextIdx_;
		nextIdx_ = 1 - nextIdx_;
		processQueuedNodes_sphere(td);
		processSuccessors(td);
	}
}

void QuadTree::Private::processQueuedNodes_scalar(QuadTreeTraversal &td, int32_t nodeIdx) {
	for (; nodeIdx < static_cast<int32_t>(numQueuedItems_); nodeIdx++) {
		float minX = queuedMinX_[nodeIdx];
		float minY = queuedMinY_[nodeIdx];
		float maxX = queuedMaxX_[nodeIdx];
		float maxY = queuedMaxY_[nodeIdx];
		td.corners[0].x = minX;
		td.corners[0].y = minY;
		td.corners[1].x = maxX;
		td.corners[1].y = minY;
		td.corners[2].x = minX;
		td.corners[2].y = maxY;
		td.corners[3].x = maxX;
		td.corners[3].y = maxY;

		bool hasIntersection = true;
		for (const auto &axis: td.projection->axes) {
			td.projections[0] = td.corners[0].dot(axis.dir);
			td.projections[1] = td.corners[1].dot(axis.dir);
			td.projections[2] = td.corners[2].dot(axis.dir);
			td.projections[3] = td.corners[3].dot(axis.dir);

			auto [minA, maxA] = std::minmax_element(
					td.projections.begin(), td.projections.end());
			if (*maxA < axis.min || axis.max < *minA) {
				hasIntersection = false;
				break; // no need to check further axes
			}
		}

		// write to successor array
		successorIdx_[numSucceedingItems_] = nodeIdx;
		// but only increment the count if the projection intersects with the node
		numSucceedingItems_ += int32_t(hasIntersection);
	}
}

void QuadTree::Private::processSuccessors(QuadTreeTraversal &td) {
	// this function implements a scalar loop over all nodes that passed the intersection test
	// in the current iteration:
	// (1) if inner node, then add children to the queue for the next iteration.
	// (2) if leaf node, then process the shapes in the node.

	for (uint32_t i = 0; i < numSucceedingItems_; i++) {
		auto successorIdx = successorIdx_[i];
		auto successor = queuedNodes_[currIdx_][successorIdx];

		if (successor->shapes.size() > 0) {
			processLeafNode(td, successor);
		}
		if (!successor->isLeaf()) {
			// add child nodes to the queue for the next iteration
			addNodeToQueue(td.tree->nodes_[successor->childrenIdx[0]]);
			addNodeToQueue(td.tree->nodes_[successor->childrenIdx[1]]);
			addNodeToQueue(td.tree->nodes_[successor->childrenIdx[2]]);
			addNodeToQueue(td.tree->nodes_[successor->childrenIdx[3]]);
		}
	}
	numSucceedingItems_ = 0;
}

static bool isMasked(QuadTreeTraversal &td, const QuadTree::Item *item) {
	// only include the item if the traversal bit is set
	if (td.traversalBit != 0 &&
		(item->shape->traversalMask() & td.traversalBit) == 0) {
		return true;
	}
	return false;
}

void QuadTree::Private::processLeafNode(QuadTreeTraversal &td, Node *leaf) {
	// 3D intersection test with the shapes in the node
	// TODO: Use SIMD for processing quad tree leaf node batches.
	//       One difficulty is that different shape types must be supported,
	//       which also would use different code paths. With current set of
	//       shape types, I think we would have 7 different code paths.

	if (td.tree->testMode3D_ == QUAD_TREE_3D_TEST_NONE) {
		// no intersection test, just call the callback
		for (uint32_t itemIdx: leaf->shapes) {
			auto &quadShape = td.tree->items_[itemIdx];
			if (isMasked(td, quadShape)) continue;
			td.callback(*quadShape->shape.get(), td.userData);
		}
	} else if (td.tree->testMode3D_ == QUAD_TREE_3D_TEST_ALL) {
		// test all shapes, even if they are not close to the shape's projection origin
		for (uint32_t itemIdx: leaf->shapes) {
			auto &quadShape = td.tree->items_[itemIdx];
			if (quadShape->shape->hasIntersectionWith(*td.shape)) {
				td.callback(*quadShape->shape.get(), td.userData);
			}
#ifdef QUAD_TREE_DEBUG_TESTS
			num3DTests_ += 1;
#endif
		}
	} else if (td.tree->testMode3D_ == QUAD_TREE_3D_TEST_CLOSEST) {
		// heuristic: only test shapes that are close to the shape's projection origin (e.g. camera position)
		// This is a good approach because:
		//     (1) shapes that are close use higher level of detail -> more expensive to draw false positives
		//     (2) most false positives are close to camera position in case camera is above/below the ground level
		float distSq = (td.basePoint - leaf->bounds.center()).lengthSquared();
		if (distSq > td.tree->closeDistanceSquared_) {
			for (uint32_t itemIdx: leaf->shapes) {
				auto &quadShape = td.tree->items_[itemIdx];
				if (isMasked(td, quadShape)) continue;
				td.callback(*quadShape->shape.get(), td.userData);
			}
		} else {
			for (uint32_t itemIdx: leaf->shapes) {
				auto &quadShape = td.tree->items_[itemIdx];
				if (quadShape->shape->hasIntersectionWith(*td.shape)) {
					if (isMasked(td, quadShape)) continue;
					td.callback(*quadShape->shape.get(), td.userData);
				}
#ifdef QUAD_TREE_DEBUG_TESTS
				num3DTests_ += 1;
#endif
			}
		}
	}
}

void QuadTree::foreachIntersection(
		const BoundingShape &shape,
		void (*callback)(const BoundingShape &, void *),
		void *userData,
		uint32_t traversalBit) {
	if (!root_) return;
	if (root_->isLeaf() && root_->shapes.empty()) return;

	auto &origin = shape.tfOrigin();
	// project the shape onto the xz-plane for faster intersection tests
	// with the quad tree nodes.
	const OrthogonalProjection &projection = shape.getOrthogonalProjection();
	QuadTreeTraversal td;
	td.tree = this;
	td.shape = &shape;
	td.projection = &projection;
	td.basePoint = Vec2f(origin.x, origin.z);
	td.callback = callback;
	td.userData = userData;
	td.traversalBit = traversalBit;

#ifdef QUAD_TREE_DEBUG_TESTS
	priv_->num2DTests_ = 0;
	priv_->num3DTests_ = 0;
#endif

	priv_->numQueuedItems_ = 0;
	priv_->numSucceedingItems_ = 0;
	priv_->addNodeToQueue(root_);

	if (projection.type == OrthogonalProjection::Type::CIRCLE) {
		priv_->intersectionLoop_sphere(td);
	} else {
		// call the templated intersection loop function with the number of axes
		// as template parameter.
		switch (const size_t numAxes = projection.axes.size()) {
			case 0: // circle handled separately already
				// shouldn't reach here for polygon/circle mix, but keep safe
				REGEN_ERROR("unexpected axis count 0");
				break;
			case 1: priv_->intersectionLoop<1>(td); break;
			case 2: priv_->intersectionLoop<2>(td); break;
			case 3: priv_->intersectionLoop<3>(td); break;
			case 4: priv_->intersectionLoop<4>(td); break;
			case 5: priv_->intersectionLoop<5>(td); break;
			case 6: priv_->intersectionLoop<6>(td); break;
			case 7: priv_->intersectionLoop<7>(td); break;
			case 8: priv_->intersectionLoop<8>(td); break;
			case 9: priv_->intersectionLoop<9>(td); break;
			case 10: priv_->intersectionLoop<10>(td); break;
			default:
				REGEN_ERROR("unsupported number of axes for intersection test: " << numAxes);
				return;
		}
	}

#ifdef QUAD_TREE_DEBUG_TESTS
	static uint32_t numFrames = 0;
	static std::vector<uint32_t> numTests_2d;
	static std::vector<uint32_t> numTests_3d;
	numFrames++;
	numTests_2d.push_back(priv_->num2DTests_);
	numTests_3d.push_back(priv_->num3DTests_);
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
#endif
}

void QuadTree::update(float dt) {
	static auto maxFloat = Vec2f(std::numeric_limits<float>::lowest());
	static auto minFloat = Vec2f(std::numeric_limits<float>::max());
	if (items_.empty() && newItems_.empty()) {
		// nothing to do
		return;
	}
	bool hasChanged;
#ifdef QUAD_TREE_DEBUG_TIME
	static ElapsedTimeDebugger elapsedTime("Quad-Tree Update",
		1000, ElapsedTimeDebugger::CPU_ONLY);
	elapsedTime.beginFrame();
#endif
#ifdef QUAD_TREE_DEBUG_TIME
	elapsedTime.push("starting");
#endif

	changedItems_.clear();
	newBounds_.min = minFloat;
	newBounds_.max = maxFloat;
#ifdef QUAD_TREE_EVER_GROWING
	if (root_ != nullptr) {
		// never shrink the root node
		newBounds_.extend(root_->bounds);
	}
#endif

	// go through all items and update their geometry and transform, and the new bounds
	for (uint32_t itemIdx = 0; itemIdx < items_.size(); itemIdx++) {
		auto &item = items_[itemIdx];
		// NOTE: items with traversalMask == 0 are currently disabled,
		//       so we can skip updating them, and remove them from the tree.
		if (item->shape->traversalMask() == 0) {
			if (item->node) {
				removeFromNode(item->node, item->idxInNode);
				collapse(item->node);
				item->node = nullptr;
			}
			continue;
		}
		hasChanged = item->shape->updateGeometry();
		hasChanged = item->shape->updateTransform(hasChanged) || hasChanged;
		if (hasChanged) {
			item->projection.update(*item->shape.get());
			changedItems_.push_back(itemIdx);
		}
		newBounds_.extend(item->projection.bounds);
	}
	// do the same for any additional items added to the tree
	for (const auto &item: newItems_) {
		hasChanged = item->shape->updateGeometry();
		hasChanged = item->shape->updateTransform(hasChanged) || hasChanged;
		if (hasChanged) {
			item->projection.update(*item->shape.get());
		}
		newBounds_.extend(item->projection.bounds);
	}
#ifdef QUAD_TREE_SQUARED
	// make the bounds square
	newBounds_.min.x = std::min(newBounds_.min.x, newBounds_.min.y);
	newBounds_.min.y = newBounds_.min.x;
	newBounds_.max.x = std::max(newBounds_.max.x, newBounds_.max.y);
	newBounds_.max.y = newBounds_.max.x;
#endif
#ifdef QUAD_TREE_DEBUG_TIME
	elapsedTime.push("bounds-update");
#endif
	auto reInit = (root_ == nullptr || newBounds_ != root_->bounds);
	if (reInit) {
		// if bounds have changed, re-initialize the tree
		// free the root node and start all over
		if (root_) freeNode(root_);
		root_ = createNode(newBounds_.min, newBounds_.max);
		numNodes_ = 1;
		numLeaves_ = 1;

		for (uint32_t itemIdx = 0; itemIdx < items_.size(); itemIdx++) {
			auto &item = items_[itemIdx];
			if (item->shape->traversalMask() == 0) continue;
			item->node = nullptr;
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
		uint32_t newItemIdx = static_cast<uint32_t>(items_.size());
		items_.push_back(item);

		if (insert1(root_, newItemIdx, true)) {
			shapeToItem_[item->shape.get()] = newItemIdx;
		} else {
			freeItem(item);
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
	auto nextBufferSize = math::nextPow2(numLeaves_);
	if (nextBufferSize > priv_->queuedMinX_.size()) {
		priv_->queuedNodes_[0].resize(nextBufferSize);
		priv_->queuedNodes_[1].resize(nextBufferSize);
		priv_->queuedMinX_.resize(nextBufferSize);
		priv_->queuedMinY_.resize(nextBufferSize);
		priv_->queuedMaxX_.resize(nextBufferSize);
		priv_->queuedMaxY_.resize(nextBufferSize);
		priv_->successorIdx_.resize(nextBufferSize);
#ifdef QUAD_TREE_DEFERRED_BATCH_STORE
		priv_->batchResults_.resize(nextBufferSize / regen::simd::RegisterWidth + 1);
#endif
	}
#ifdef QUAD_TREE_DEBUG_TIME
	elapsedTime.push("buffer-resize");
#endif

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
	for (auto &item: items_) {
		if (item->shape->traversalMask() == 0) continue;
		auto &projection = item->projection;
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
