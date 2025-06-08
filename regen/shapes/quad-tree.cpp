#include <stack>
#include <chrono>
#include <limits>
#include <unordered_set>

#include "quad-tree.h"

#define QUAD_TREE_DEBUG
#define QUAD_TREE_EVER_GROWING
#define QUAD_TREE_SQUARED
#define QUAD_TREE_SUBDIVIDE_THRESHOLD 4
#define QUAD_TREE_COLLAPSE_THRESHOLD 2

//#define QUAD_TREE_NO_SSE
#if defined(__SSE2__) && !defined(QUAD_TREE_NO_SSE)
	// Use SSE2 for faster intersection tests
	#define QUAD_TREE_SSE
	#include <simde/x86/sse2.h>
#endif

using namespace regen;

QuadTree::QuadTree()
		: SpatialIndex(),
		  root_(nullptr),
		  newBounds_(0,0) {
}

QuadTree::~QuadTree() {
	if (root_) {
		delete root_;
		root_ = nullptr;
	}
	for (auto item: items_) {
		delete item.second;
	}
	items_.clear();
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
}

unsigned int QuadTree::numNodes() const {
	if (!root_) {
		return 0;
	}
	unsigned int count = 0;
	std::stack<const Node *> stack;
	stack.push(root_);
	while (!stack.empty()) {
		auto *node = stack.top();
		stack.pop();
		count++;
		if (!node->isLeaf()) {
			for (int i = 0; i < 4; i++) {
				stack.push(node->children[i]);
			}
		}
	}
	return count;
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

QuadTree::Item* QuadTree::getItem(const ref_ptr<BoundingShape> &shape) {
	auto it = items_.find(shape.get());
	if (it != items_.end()) {
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
		bool isNodeLargeEnough = (nodeSize.x > 2.0*shapeSize.x && nodeSize.y > 2.0*shapeSize.y);

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
	}
	else {
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
	for (auto &node : nodes) {
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
	for (auto &child : parent->children) {
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

static inline void pushSphereIntersections(
		std::stack<QuadTree::Node *> &stack,
		QuadTree::Node **nodes,
		const OrthogonalProjection &projection) {
	const auto &radiusSqr = projection.points[1].x; // = radius * radius
	const auto &center = projection.points[0];
#ifdef QUAD_TREE_SSE
	// Load center.xy + radiusSqr into SSE registers
	simde__m128 centerX = simde_mm_set1_ps(center.x);
	simde__m128 centerY = simde_mm_set1_ps(center.y);
	simde__m128 radiusSqrSSE = simde_mm_set1_ps(radiusSqr);
	// Load bounds for 4 nodes into SSE vectors
	float minX[4], maxX[4], minY[4], maxY[4];
	for (int i = 0; i < 4; ++i) {
		minX[i] = nodes[i]->bounds.min.x;
		maxX[i] = nodes[i]->bounds.max.x;
		minY[i] = nodes[i]->bounds.min.y;
		maxY[i] = nodes[i]->bounds.max.y;
	}
	simde__m128 minXv = simde_mm_loadu_ps(minX);
	simde__m128 maxXv = simde_mm_loadu_ps(maxX);
	simde__m128 minYv = simde_mm_loadu_ps(minY);
	simde__m128 maxYv = simde_mm_loadu_ps(maxY);

	// Compute distance along X
	simde__m128 distLX = simde_mm_sub_ps(minXv, centerX);
	simde__m128 distRX = simde_mm_sub_ps(centerX, maxXv);
	simde__m128 maskLX = simde_mm_cmplt_ps(centerX, minXv);
	simde__m128 maskRX = simde_mm_cmpgt_ps(centerX, maxXv);
	simde__m128 distX = simde_mm_or_ps(
		simde_mm_and_ps(maskLX, distLX),
		simde_mm_and_ps(maskRX, distRX)
	);
	// Compute distance along Y
	simde__m128 distLY = simde_mm_sub_ps(minYv, centerY);
	simde__m128 distRY = simde_mm_sub_ps(centerY, maxYv);
	simde__m128 maskLY = simde_mm_cmplt_ps(centerY, minYv);
	simde__m128 maskRY = simde_mm_cmpgt_ps(centerY, maxYv);
	simde__m128 distY = simde_mm_or_ps(
		simde_mm_and_ps(maskLY, distLY),
		simde_mm_and_ps(maskRY, distRY)
	);
	// Compute total squared distance
	simde__m128 sqDistX = simde_mm_mul_ps(distX, distX);
	simde__m128 sqDistY = simde_mm_mul_ps(distY, distY);
	simde__m128 sqDist  = simde_mm_add_ps(sqDistX, sqDistY);

	// Finally, compare against radius², and push nodes that intersect
	simde__m128 mask = simde_mm_cmplt_ps(sqDist, radiusSqrSSE);
	int bitmask = simde_mm_movemask_ps(mask); // 4 bits, one per node
	if (bitmask & (1 << 0)) { stack.push(nodes[0]); }
	if (bitmask & (1 << 1)) { stack.push(nodes[1]); }
	if (bitmask & (1 << 2)) { stack.push(nodes[2]); }
	if (bitmask & (1 << 3)) { stack.push(nodes[3]); }
#else
	for (int i=0; i < 4; i++) {
		auto *n = nodes[i];
		// Calculate the squared distance from the circle's center to the AABB
		float sqDist = 0.0f;
		if (center.x < n->bounds.min.x) {
			sqDist += (n->bounds.min.x - center.x) * (n->bounds.min.x - center.x);
		} else if (center.x > n->bounds.max.x) {
			sqDist += (center.x - n->bounds.max.x) * (center.x - n->bounds.max.x);
		}
		if (center.y < n->bounds.min.y) {
			sqDist += (n->bounds.min.y - center.y) * (n->bounds.min.y - center.y);
		} else if (center.y > n->bounds.max.y) {
			sqDist += (center.y - n->bounds.max.y) * (center.y - n->bounds.max.y);
		}
		if (sqDist < radiusSqr) {
			// the node intersects with the sphere
			stack.push(n);
		}
	}
#endif
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

#ifdef QUAD_TREE_SSE
static inline void project_simd(QuadTree::Node **nodes, const Vec2f &axis, float outMin[4], float outMax[4]) {
	// Each array stores the same corner across all 4 nodes
	alignas(16) float x0[4], y0[4]; // b.min
	alignas(16) float x1[4], y1[4]; // (b.max.x, b.min.y)
	alignas(16) float x2[4], y2[4]; // (b.min.x, b.max.y)
	alignas(16) float x3[4], y3[4]; // b.max
	for (int i = 0; i < 4; ++i) {
		const auto &b = nodes[i]->bounds;
		x0[i] = b.min.x; y0[i] = b.min.y;
		x1[i] = b.max.x; y1[i] = b.min.y;
		x2[i] = b.min.x; y2[i] = b.max.y;
		x3[i] = b.max.x; y3[i] = b.max.y;
	}

	// Compute dot products for each corner with the axis
	auto dot4 = [&](const float *xs, const float *ys) -> simde__m128 {
		simde__m128 vx = simde_mm_load_ps(xs);
		simde__m128 vy = simde_mm_load_ps(ys);
		simde__m128 ax = simde_mm_set1_ps(axis.x);
		simde__m128 ay = simde_mm_set1_ps(axis.y);
		return simde_mm_add_ps(simde_mm_mul_ps(vx, ax), simde_mm_mul_ps(vy, ay));
	};
	simde__m128 d0 = dot4(x0, y0);
	simde__m128 d1 = dot4(x1, y1);
	simde__m128 d2 = dot4(x2, y2);
	simde__m128 d3 = dot4(x3, y3);

	// Compute per-node min and max
	simde__m128 min1 = simde_mm_min_ps(d0, d1);
	simde__m128 min2 = simde_mm_min_ps(d2, d3);
	simde__m128 minA = simde_mm_min_ps(min1, min2);
	simde__m128 max1 = simde_mm_max_ps(d0, d1);
	simde__m128 max2 = simde_mm_max_ps(d2, d3);
	simde__m128 maxA = simde_mm_max_ps(max1, max2);

	// Store result
	simde_mm_storeu_ps(outMin, minA);
	simde_mm_storeu_ps(outMax, maxA);
}
#endif

static inline void pushPolygonIntersections(
		std::stack<QuadTree::Node *> &stack,
		QuadTree::Node **nodes,
		const OrthogonalProjection &projection) {
#ifdef QUAD_TREE_SSE
	uint8_t intersectMask = 0b1111;
	for (const auto &axis: projection.axes) {
		float min[4], max[4];
		project_simd(nodes, axis.dir, min, max);

		// Compute `(maxA_n < axis.min) || (axis.max < minA_n)`
		simde__m128 minA = simde_mm_loadu_ps(min);
		simde__m128 maxA = simde_mm_loadu_ps(max);
		simde__m128 axisMin = simde_mm_set1_ps(axis.min);
		simde__m128 axisMax = simde_mm_set1_ps(axis.max);
		simde__m128 mask1 = simde_mm_cmplt_ps(maxA, axisMin);
		simde__m128 mask2 = simde_mm_cmplt_ps(axisMax, minA);
		simde__m128 sep = simde_mm_or_ps(mask1, mask2);

		// Convert mask to bits
		uint8_t sepMask = simde_mm_movemask_ps(sep); // 1 = separated
		intersectMask &= ~sepMask; // Clear bits in intersectMask where sepMask is 1
		// Early exit: if no bits are set, all 4 nodes culled
		if (intersectMask == 0) return;
	}
	if (intersectMask & (1 << 0)) { stack.push(nodes[0]); }
	if (intersectMask & (1 << 1)) { stack.push(nodes[1]); }
	if (intersectMask & (1 << 2)) { stack.push(nodes[2]); }
	if (intersectMask & (1 << 3)) { stack.push(nodes[3]); }
#else
	for (int i=0; i < 4; i++) {
		auto *n = nodes[i];
		bool intersects = true;
		// Check for separation along the axes of the shape and the axis-aligned quad
		for (const auto &axis: projection.axes) {
			auto [minA, maxA] = project(n->bounds, axis.dir);
			if (maxA < axis.min || axis.max < minA) {
				intersects = false;
				break; // no intersection along this axis
			}
		}
		if (intersects) {
			// the node intersects with the box
			stack.push(n);
		}
	}
#endif
}

static inline QuadTree::Node *getEnclosingNode(QuadTree::Node **nodes, const OrthogonalProjection &projection) {
	// Check if one of the nodes fully contains the projection
	if (nodes[0]->contains(projection)) { return nodes[0]; }
	if (nodes[1]->contains(projection)) { return nodes[1]; }
	if (nodes[2]->contains(projection)) { return nodes[2]; }
	if (nodes[3]->contains(projection)) { return nodes[3]; }
	return nullptr;
}

static void pushIntersections(
		std::stack<QuadTree::Node *> &stack,
		QuadTree::Node **nodes,
		const OrthogonalProjection &projection) {
	// Make a contains check first, to avoid unnecessary intersection tests
	/**
	QuadTree::Node *enclosingNode = getEnclosingNode(nodes, projection);
	if (enclosingNode) {
		stack = std::stack<QuadTree::Node *>();
		stack.push(enclosingNode);
		REGEN_INFO("QUAD TREE INTERSECTION: found enclosing node");
		return;
	}
	 **/

	if (projection.type == OrthogonalProjection::Type::CIRCLE) {
		pushSphereIntersections(stack, nodes, projection);
	} else {
		pushPolygonIntersections(stack, nodes, projection);
	}
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

bool QuadTree::hasIntersection(const BoundingShape &shape) {
	int count = 0;
	foreachIntersection(shape, [&count](const BoundingShape &shape) {
		count++;
	});
	return count > 0;
}

int QuadTree::numIntersections(const BoundingShape &shape) {
	int count = 0;
	foreachIntersection(shape, [&count](const BoundingShape &shape) {
		count++;
	});
	return count;
}

void QuadTree::foreachIntersection(
		const BoundingShape &shape,
		const std::function<void(const BoundingShape &)> &callback) {
	if (!root_) return;
	if (root_->isLeaf() && root_->shapes.empty()) return;

	// TODO: make configurable
	static const float minDistanceThresholdSq = 20.0f * 20.0f; // heuristic threshold for distance to camera position

	std::unordered_set<const Item *> visited;
	// project the shape onto the xz-plane for faster intersection tests
	// with the quad tree nodes.
	OrthogonalProjection shape_projection(shape);
	std::stack<Node *> stack;
	stack.push(root_);

#ifdef QUAD_TREE_DEBUG
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::milliseconds;
	GLuint num2DTests = 0;
	GLuint num3DTests = 0;
	GLuint num3DPruned = 0;
	auto t1 = high_resolution_clock::now();
#endif

	// FIXME: only works for frustum shapes!
	Vec2f &basePoint = shape_projection.points[0];

	while (!stack.empty()) {
		Node *node = stack.top();
		stack.pop();

#ifdef QUAD_TREE_DEBUG
		num2DTests++;
#endif

		if (node->isLeaf()) {
			// 3D intersection test with the shapes in the node
			for (const auto &quadShape: node->shapes) {
				if (visited.find(quadShape) != visited.end()) {
					continue;
				}
				visited.insert(quadShape);

				// heuristic: only test shapes that are close to the shape's projection origin (e.g. camera position)
				// This is a good approach because:
				//     (1) shapes that are close use higher level of detail -> more expensive to draw false positives
				//     (2) most false positives are close to camera position in case camera is above/below the ground level
				float distSq = (basePoint - node->bounds.center()).lengthSquared();

				if (distSq > minDistanceThresholdSq) {
					callback(*quadShape->shape.get());
				}
				else {
					if (quadShape->shape->hasIntersectionWith(shape)) {
						callback(*quadShape->shape.get());
					} else {
						num3DPruned += 1;
					}
					#ifdef QUAD_TREE_DEBUG
					num3DTests++;
					#endif
				}
			}
		} else {
			// Add the children to the stack
			pushIntersections(stack, node->children, shape_projection);
		}
	}

#ifdef QUAD_TREE_DEBUG
	auto t2 = high_resolution_clock::now();
	duration<double, std::milli> ms_double = t2 - t1;
	REGEN_INFO("QUAD TREE INTERSECTION STATS");
	REGEN_INFO("     time: " << ms_double.count() << " ms");
	unsigned int numShapes = 0;
	for (const auto &x: shapes_) {
		numShapes += x.second.size();
	}
	REGEN_INFO("     #Shapes: " << numShapes);
	REGEN_INFO("     #Nodes: " << numNodes());
	REGEN_INFO("     #2D Tests: " << num2DTests);
	REGEN_INFO("     #3D Tests: " << num3DTests);
	REGEN_INFO("     #3D Prune: " << num3DPruned);
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
	for (const auto &it: items_) {
		auto &item = it.second;
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
		if(root_) freeNode(root_);
		root_ = createNode(newBounds_.min, newBounds_.max);

		for (auto &it: items_) {
			auto &item = it.second;
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
		if(insert1(root_, item, true)) {
			items_[item->shape.get()] = item;
		} else {
			freeItem(item);
			REGEN_WARN("Failed to insert shape into quad tree. This should not happen!");
		}
	}
	newItems_.clear();

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
		auto &projection = item.second->projection;
		auto &points = projection.points;
		switch (projection.type) {
			case OrthogonalProjection::Type::CIRCLE: {
				auto radius = std::sqrt(points[1].x);
				debug.drawCircle(toVec3(points[0], h), radius, lineColor);
				break;
			}
			default:
				for (size_t i=0; i<points.size(); i++) {
					debug.drawLine(
						toVec3(points[i], h),
						toVec3(points[(i+1)%points.size()], h),
						lineColor);
				}
				break;
		}
	}
}
