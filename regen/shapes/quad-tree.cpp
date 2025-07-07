#include <stack>
#include <chrono>
#include <limits>
#include <unordered_set>

#include "quad-tree.h"
#include "regen/math/simd.h"

//#define QUAD_TREE_DEBUG
#define QUAD_TREE_EVER_GROWING
#define QUAD_TREE_SQUARED
//#define QUAD_TREE_DISABLE_SIMD
#define QUAD_TREE_MASK_EARLY_EXIT
//#define QUAD_TREE_DEFERRED_BATCH_STORE
#define QUAD_TREE_SUBDIVIDE_THRESHOLD 4
#define QUAD_TREE_COLLAPSE_THRESHOLD 2

#ifdef QUAD_TREE_DISABLE_SIMD
	#ifdef QUAD_TREE_DEFERRED_BATCH_STORE
	#undef QUAD_TREE_DEFERRED_BATCH_STORE
	#endif // QUAD_TREE_DEFERRED_BATCH_STORE
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
#ifdef QUAD_TREE_DEBUG
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
	if (root_) {
		delete root_;
		root_ = nullptr;
	}
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
	while (!nodePool_.empty()) {
		delete nodePool_.top();
		nodePool_.pop();
	}
	delete priv_;
}

unsigned int QuadTree::numShapes() const {
	if (!root_) {
		return 0;
	}
	std::stack<const Node *> stack;
	std::unordered_set<const BoundingShape *> shapes;
	stack.push(root_);
	while (!stack.empty()) {
		auto *node = stack.top();
		stack.pop();
		for (const auto &shape: node->shapes) {
			shapes.insert(shape->shape.get());
		}
		if (!node->isLeaf()) {
			for (int i = 0; i < 4; i++) {
				stack.push(node->children[i]);
			}
		}
	}
	return shapes.size();
}

QuadTree::Node *QuadTree::createNode(const Vec2f &min, const Vec2f &max) {
	if (nodePool_.empty()) {
		return new Node(min, max);
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
			freeNode(node->children[i]);
			node->children[i] = nullptr;
		}
	}
	node->shapes.clear();
	node->parent = nullptr;
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
	item->nodes.clear();
	itemPool_.push(item);
}

QuadTree::Item *QuadTree::getItem(const ref_ptr<BoundingShape> &shape) {
	auto it = shapeToItem_.find(shape.get());
	if (it != shapeToItem_.end()) {
		return it->second;
	}
	return nullptr;
}

void QuadTree::insert(const ref_ptr<BoundingShape> &shape) {
	newItems_.push_back(createItem(shape));
	addToIndex(shape);
}

bool QuadTree::insert(Node *node, Item *shape, bool allowSubdivision) { // NOLINT(misc-no-recursion)
	if (!node->intersects(shape->projection)) {
		return false;
	} else {
		// only subdivide if the node is larger than the shape
		auto nodeSize = node->bounds.max - node->bounds.min;
		auto shapeSize = shape->projection.bounds.max - shape->projection.bounds.min;
		bool isNodeLargeEnough = (nodeSize.x > 2.0 * shapeSize.x && nodeSize.y > 2.0 * shapeSize.y);

		return insert1(node, shape, isNodeLargeEnough && allowSubdivision);
	}
}

bool QuadTree::insert1(Node *node, Item *newShape, bool allowSubdivision) { // NOLINT(misc-no-recursion)
	if (node->isLeaf()) {
		// the node does not have child nodes (yet).
		// the shape can be added to the node in three cases:
		// 1. the node has still not exceeded the maximum number of shapes per node
		// 2. the node has reached the minimum size and cannot be subdivided further
		// 3. the node was just created by subdividing a parent node
		if (!allowSubdivision ||
			node->shapes.size() < QUAD_TREE_SUBDIVIDE_THRESHOLD ||
			node->bounds.size() < minNodeSize_) {
			node->shapes.push_back(newShape);
			newShape->nodes.push_back(node);
			return true;
		} else {
			// split the node into four children
			subdivide(node);
			bool inserted = false;
			// remove the subdivided node from the list of nodes of the existing shapes
			for (const auto &existingShape: node->shapes) {
				existingShape->removeNode(node);
			}
			for (auto &child: node->children) {
				// reinsert the existing shapes into the new children nodes
				for (const auto &existingShape: node->shapes) {
					insert(child, existingShape, true);
				}
				// also insert the new shape
				inserted = insert(child, newShape, true) || inserted;
			}
			// subdivided node does not contain shapes anymore
			node->shapes.clear();
			return inserted;
		}
	} else {
		// the node has child nodes, must insert into (at least) one of them.
		// note: a shape can be inserted into multiple nodes, so we allways need to check all children
		//       (at least on the next level)
		for (auto &child: node->children) {
			if (child->contains(newShape->projection)) {
				// the child node fully contains the shape, insert it
				if (insert(child, newShape, allowSubdivision)) return true;
			}
		}
		bool inserted = false;
		for (auto &child: node->children) {
			inserted = insert(child, newShape, allowSubdivision) || inserted;
		}
		return inserted;
	}
}

void QuadTree::remove(const ref_ptr<BoundingShape> &shape) {
	auto item = getItem(shape);
	if (!item) {
		REGEN_WARN("Shape not found in quad tree.");
		return;
	}
	removeFromNodes(item);
}

void QuadTree::removeFromNodes(Item *item) {
	auto nodes = item->nodes;
	for (auto &node: nodes) {
		removeFromNode(node, item);
	}
	item->nodes.clear();
}

void QuadTree::removeFromNode(Node *node, Item *shape) {
	auto it = std::find(node->shapes.begin(), node->shapes.end(), shape);
	if (it == node->shapes.end()) {
		return;
	}
	node->shapes.erase(it);
	collapse(node);
}

void QuadTree::collapse(Node *node) { // NOLINT(misc-no-recursion)
	if (!node->parent) {
		return;
	}
	auto *parent = node->parent;
	auto &firstShapes = parent->children[0]->shapes;
	for (auto &child: parent->children) {
		if (!child->isLeaf()) {
			// at least one child is not a leaf -> no collapse
			return;
		}
	}
	//if (parent->children[0]->shapes.size() > QUAD_TREE_COLLAPSE_THRESHOLD) {
	// no collapse
	//return;
	//}
	for (int i = 1; i < 4; i++) {
		if (parent->children[i]->shapes.size() != firstShapes.size()) {
			// unequal number of shapes -> no collapse
			return;
		}
		for (auto &shape: parent->children[i]->shapes) {
			if (std::find(firstShapes.begin(), firstShapes.end(), shape) == firstShapes.end()) {
				// not all children contain the same set of shapes -> no collapse
				return;
			}
		}
	}
	// finally, collapse the node...
	for (auto &shape: firstShapes) {
		// remove the children nodes from the shape's list of nodes
		for (int i = 0; i < 4; i++) {
			auto it = std::find(shape->nodes.begin(), shape->nodes.end(), parent->children[i]);
			shape->nodes.erase(it);
		}
		// add the parent node to the shape's list of nodes
		shape->nodes.push_back(parent);
		// add the shape to the parent node
		parent->shapes.push_back(shape);
	}
	// free all children nodes
	for (int i = 0; i < 4; i++) {
		freeNode(parent->children[i]);
		parent->children[i] = nullptr;
	}
	// four nodes are removed, parent turns into a leaf node
	numNodes_ -= 4;
	numLeaves_ -= 3;
	// continue collapsing the parent node
	collapse(parent);
}

void QuadTree::subdivide(Node *node) {
	// Subdivide the node into four children.
	auto center = (node->bounds.min + node->bounds.max) * 0.5f;
	Node *child;
	// bottom-left
	child = createNode(node->bounds.min, center);
	child->parent = node;
	node->children[0] = child;
	// bottom-right
	child = createNode(Vec2f(center.x, node->bounds.min.y), Vec2f(node->bounds.max.x, center.y));
	child->parent = node;
	node->children[1] = child;
	// top-right
	child = createNode(center, node->bounds.max);
	child->parent = node;
	node->children[2] = child;
	// top-left
	child = createNode(Vec2f(node->bounds.min.x, center.y), Vec2f(center.x, node->bounds.max.y));
	child->parent = node;
	node->children[3] = child;
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

void QuadTree::Item::removeNode(Node *node) {
	auto it = std::find(nodes.begin(), nodes.end(), node);
	if (it != nodes.end()) {
		nodes.erase(it);
	}
}

QuadTree::Node::Node(const Vec2f &min, const Vec2f &max) : bounds(min, max), parent(nullptr) {
	for (int i = 0; i < 4; i++) {
		children[i] = nullptr;
	}
}

QuadTree::Node::~Node() {
	for (int i = 0; i < 4; i++) {
		delete children[i];
		children[i] = nullptr;
	}
}

bool QuadTree::Node::isLeaf() const {
	return children[0] == nullptr;
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
	switch (projection.type) {
		case OrthogonalProjection::Type::CIRCLE: {
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
		}
		case OrthogonalProjection::Type::TRIANGLE:
		case OrthogonalProjection::Type::RECTANGLE:
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

bool QuadTree::hasIntersection(const BoundingShape &shape) {
	int count = 0;
	foreachIntersection(shape, countIntersections, &count);
	return count > 0;
}

int QuadTree::numIntersections(const BoundingShape &shape) {
	int count = 0;
	foreachIntersection(shape, countIntersections, &count);
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
    if constexpr (NumAxes >= 1u) {
        processAxis_SIMD(td, mask, 0);
#ifdef QUAD_TREE_MASK_EARLY_EXIT
        if (mask == 0) return;
#endif
    }
    if constexpr (NumAxes >= 2u) {
        processAxis_SIMD(td, mask, 1);
#ifdef QUAD_TREE_MASK_EARLY_EXIT
        if (mask == 0) return;
#endif
    }
    if constexpr (NumAxes >= 3u) {
        processAxis_SIMD(td, mask, 2);
#ifdef QUAD_TREE_MASK_EARLY_EXIT
        if (mask == 0) return;
#endif
    }
    if constexpr (NumAxes >= 4u) {
        processAxis_SIMD(td, mask, 3);
#ifdef QUAD_TREE_MASK_EARLY_EXIT
        if (mask == 0) return;
#endif
    }
    if constexpr (NumAxes >= 5u) {
        processAxis_SIMD(td, mask, 4);
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
#ifdef QUAD_TREE_DEBUG
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

		if (successor->isLeaf()) {
			// TODO: Consider using SIMD for processing quad tree leaf node batches.
			//       One difficulty is that different shape types must be supported,
			//       which also would use different code paths.
			processLeafNode(td, successor);
		} else {
			// add child nodes to the queue for the next iteration
			addNodeToQueue(successor->children[0]);
			addNodeToQueue(successor->children[1]);
			addNodeToQueue(successor->children[2]);
			addNodeToQueue(successor->children[3]);
		}
	}
	numSucceedingItems_ = 0;
}

void QuadTree::Private::processLeafNode(QuadTreeTraversal &td, Node *leaf) {
	// 3D intersection test with the shapes in the node

	if (td.tree->testMode3D_ == QUAD_TREE_3D_TEST_NONE) {
		// no intersection test, just call the callback
		for (const auto &quadShape: leaf->shapes) {
			if (!quadShape->visited) {
				quadShape->visited = true;
				td.callback(*quadShape->shape.get(), td.userData);
			}
		}
	} else if (td.tree->testMode3D_ == QUAD_TREE_3D_TEST_ALL) {
		// test all shapes, even if they are not close to the shape's projection origin
		for (const auto &quadShape: leaf->shapes) {
			if (!quadShape->visited) {
				quadShape->visited = true;
				if (quadShape->shape->hasIntersectionWith(*td.shape)) {
					td.callback(*quadShape->shape.get(), td.userData);
				}
#ifdef QUAD_TREE_DEBUG
				num3DTests_ += 1;
#endif
			}
		}
	} else if (td.tree->testMode3D_ == QUAD_TREE_3D_TEST_CLOSEST) {
		// heuristic: only test shapes that are close to the shape's projection origin (e.g. camera position)
		// This is a good approach because:
		//     (1) shapes that are close use higher level of detail -> more expensive to draw false positives
		//     (2) most false positives are close to camera position in case camera is above/below the ground level
		float distSq = (td.basePoint - leaf->bounds.center()).lengthSquared();
		if (distSq > td.tree->closeDistanceSquared_) {
			for (const auto &quadShape: leaf->shapes) {
				// skip shapes that are already visited in this frame
				if (quadShape->visited) { continue; }
				quadShape->visited = true;
				td.callback(*quadShape->shape.get(), td.userData);
			}
		} else {
			for (const auto &quadShape: leaf->shapes) {
				// skip shapes that are already visited in this frame
				if (quadShape->visited) { continue; }
				quadShape->visited = true;
				if (quadShape->shape->hasIntersectionWith(*td.shape)) {
					td.callback(*quadShape->shape.get(), td.userData);
				}
#ifdef QUAD_TREE_DEBUG
				num3DTests_ += 1;
#endif
			}
		}
	}
}

void QuadTree::foreachIntersection(
		const BoundingShape &shape,
		void (*callback)(const BoundingShape &, void *),
		void *userData) {
	if (!root_) return;
	if (root_->isLeaf() && root_->shapes.empty()) return;

	auto &origin = shape.getShapeOrigin();
	// project the shape onto the xz-plane for faster intersection tests
	// with the quad tree nodes.
	OrthogonalProjection shape_projection(shape);
	QuadTreeTraversal td;
	td.tree = this;
	td.shape = &shape;
	td.projection = &shape_projection;
	td.basePoint = Vec2f(origin.x, origin.z);
	td.callback = callback;
	td.userData = userData;

#ifdef QUAD_TREE_DEBUG
	using std::chrono::high_resolution_clock;
	using std::chrono::duration_cast;
	using std::chrono::duration;
	using std::chrono::milliseconds;
	priv_->num2DTests_ = 0;
	priv_->num3DTests_ = 0;
	auto t1 = high_resolution_clock::now();
#endif

	// reset intersection state of items.
	// this is done to avoid duplicates as each item can be in multiple nodes.
	for (const auto &item: items_) {
		item->visited = false;
	}

	priv_->numQueuedItems_ = 0;
	priv_->numSucceedingItems_ = 0;
	if (root_->isLeaf()) {
		priv_->addNodeToQueue(root_);
	} else {
		// add the root node's children to the queue
		priv_->addNodeToQueue(root_->children[0]);
		priv_->addNodeToQueue(root_->children[1]);
		priv_->addNodeToQueue(root_->children[2]);
		priv_->addNodeToQueue(root_->children[3]);
	}

	if (shape_projection.type == OrthogonalProjection::Type::CIRCLE) {
		priv_->intersectionLoop_sphere(td);
	} else {
		if (shape_projection.axes.size() == 4) {
			priv_->intersectionLoop<4>(td);
		} else if (shape_projection.axes.size() == 5) {
			priv_->intersectionLoop<5>(td);
		} else {
			REGEN_ERROR("unsupported number of axes for intersection test: " << shape_projection.axes.size());
			return;
		}
	}

#ifdef QUAD_TREE_DEBUG
	static std::vector<long> times;
	static std::vector<uint32_t> numTests_2d;
	static std::vector<uint32_t> numTests_3d;
	auto t2 = std::chrono::high_resolution_clock::now();
	auto dt = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
	times.push_back(dt);
	numTests_2d.push_back(priv_->num2DTests_);
	numTests_3d.push_back(priv_->num3DTests_);
	if (times.size() > 100) {
		long avgTime = 0;
		long totalTime = 0;
		uint32_t avg2DTests = 0;
		uint32_t avg3DTests = 0;
		for (size_t i = 0; i < times.size(); i++) {
			totalTime += times[i];
			avg2DTests += numTests_2d[i];
			avg3DTests += numTests_3d[i];
		}
		avgTime = totalTime / times.size();
		avg2DTests /= times.size();
		avg3DTests /= times.size();
		REGEN_INFO("QuadTree:"
						   << " t0: " << std::fixed << std::setprecision(4)
						   << static_cast<float>(avgTime) / 1000.0f << "ms"
						   << " t0-t100: " << std::fixed << std::setprecision(2)
						   << static_cast<float>(totalTime) / 1000.0f << "ms"
						   << " #nodes: " << numNodes_
						   << " #leaves: " << numLeaves_
						   << " #avg-2d: " << avg2DTests
						   << " #avg-3d: " << avg3DTests);
		times.clear();
		numTests_2d.clear();
		numTests_3d.clear();
	}
#endif
}

void QuadTree::update(float dt) {
	static auto maxFloat = Vec2f(std::numeric_limits<float>::lowest());
	static auto minFloat = Vec2f(std::numeric_limits<float>::max());
	bool hasChanged;

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
	for (const auto &item: items_) {
		hasChanged = item->shape->updateGeometry();
		hasChanged = item->shape->updateTransform(hasChanged) || hasChanged;
		if (hasChanged) {
			item->projection.update(*item->shape.get());
			changedItems_.push_back(item);
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
	// if bounds have changed, re-initialize the tree
	auto reInit = (root_ == nullptr || newBounds_ != root_->bounds);
	if (reInit) {
		// free the root node and start all over
		if (root_) freeNode(root_);
		root_ = createNode(newBounds_.min, newBounds_.max);
		numNodes_ = 1;
		numLeaves_ = 1;

		for (auto &item: items_) {
			item->nodes.clear();
			insert1(root_, item, true);
		}
	}
		// else remove/insert the changed items
	else {
		for (auto item: changedItems_) {
			removeFromNodes(item);
			insert1(root_, item, true);
		}
	}
	// finally insert the new items
	for (auto item: newItems_) {
		if (insert1(root_, item, true)) {
			shapeToItem_[item->shape.get()] = item;
			items_.push_back(item);
		} else {
			freeItem(item);
			REGEN_WARN("Failed to insert shape into quad tree. This should not happen!");
		}
	}
	newItems_.clear();

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

	// make the visibility computations
	updateVisibility();
}

inline Vec3f toVec3(const Vec2f &v, float y) {
	return {v.x, y, v.y};
}

void QuadTree::debugDraw(DebugInterface &debug) const {
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
				stack.push(node->children[i]);
			}
		}
	}

	// draw 2d projections of the shapes
	lineColor = Vec3f(0, 1, 0);
	const GLfloat h = 5.1f;
	for (auto &item: items_) {
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
