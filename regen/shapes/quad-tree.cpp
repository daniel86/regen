#include <stack>

#include "quad-tree.h"

#define MAX_SHAPES_PER_NODE 4

using namespace regen;

QuadTree::QuadTree()
		: SpatialIndex(),
		  root_(nullptr) {
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

namespace regen {
	enum class ResizeDirection : int {
		LEFT = 1 << 1,
		RIGHT = 1 << 2,
		TOP = 1 << 3,
		BOTTOM = 1 << 4
	};
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

void QuadTree::freeNode(Node *node) {
	if (!node->isLeaf()) {
		for (int i = 0; i < 4; i++) {
			freeNode(node->children[i]);
			node->children[i] = nullptr;
		}
	}
	node->shapes.clear();
	nodePool_.push(node);
}

QuadTree::Item* QuadTree::getItem(const ref_ptr<BoundingShape> &shape) {
	// TODO: make this faster
	for (auto &item: items_) {
		if (item->shape.get() == shape.get()) {
			return item;
		}
	}
	return nullptr;
}

void QuadTree::insert(const ref_ptr<BoundingShape> &shape) {
	auto *quadShape = new Item(shape);
	if (insert(quadShape)) {
		addToIndex(quadShape->shape);
		items_.push_back(quadShape);
	} else {
		delete quadShape;
		REGEN_WARN("Failed to insert shape into quad tree. This should not happen!");
	}
}

bool QuadTree::insert(Item *quadShape) {
	// project the shape onto the xz-plane for faster intersection tests
	// with the quad tree nodes.

	if (!root_) {
		auto bounds = quadShape->projection.bounds();
		root_ = createNode(bounds.min, bounds.max);
	}
	else {
		// enlarge the tree if the shape does not fit into it
		root_->enlarge(quadShape->projection.bounds(),
					   static_cast<int>(ResizeDirection::LEFT) |
					   static_cast<int>(ResizeDirection::RIGHT) |
					   static_cast<int>(ResizeDirection::TOP) |
					   static_cast<int>(ResizeDirection::BOTTOM));
	}

	if (insert1(root_, quadShape, true)) {
		return true;
	} else {
		return false;
	}
}

bool QuadTree::insert(Node *node, Item *shape, bool allowSubdivision) { // NOLINT(no-recursion)
	if (!node->intersects(shape->projection)) {
		return false;
	} else {
		return insert1(node, shape, allowSubdivision);
	}
}

bool QuadTree::insert1(Node *node, Item *shape, bool allowSubdivision) { // NOLINT(no-recursion)
	if (node->isLeaf()) {
		// the node does not have child nodes (yet).
		// the shape can be added to the node in three cases:
		// 1. the node has still not exceeded the maximum number of shapes per node
		// 2. the node has reached the minimum size and cannot be subdivided further
		// 3. the node was just created by subdividing a parent node
		if (!allowSubdivision || node->shapes.size() < MAX_SHAPES_PER_NODE || node->bounds.size() < minNodeSize_) {
			node->shapes.push_back(shape);
			shape->nodes.push_back(node);
			return true;
		} else {
			subdivide(node);
			for (const auto &existingShape: node->shapes) {
				existingShape->removeNode(node);
				insert1(node, existingShape, false);
			}
			node->shapes.clear();
			shape->removeNode(node);
			return insert1(node, shape, false);
		}
	} else {
		// the node has child nodes, must insert into one of them
		// note: a shape can be inserted into multiple nodes, so we allways need to check all children
		//       (at least on the next level)
		bool inserted = false;
		for (auto &child: node->children) {
			inserted = insert(child, shape, allowSubdivision) || inserted;
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
	remove(item);
}

void QuadTree::remove(Item *item) {
	for (auto &node : item->nodes) {
		remove(node, item);
	}
	item->nodes.clear();
}

void QuadTree::remove(Node *node, Item *shape) {
	auto it = std::find(node->shapes.begin(), node->shapes.end(), shape);
	if (it == node->shapes.end()) {
		return;
	}
	node->shapes.erase(it);
	collapse(node);
}

void QuadTree::collapse(Node *node) {
	if (!node->parent) {
		return;
	}
	auto *parent = node->parent;
	bool allEmpty = true;
	for (auto &child: parent->children) {
		if (!child) {
			continue;
		}
		if (!child->shapes.empty() && child->isLeaf()) {
			allEmpty = false;
			break;
		}
	}
	if (allEmpty) {
		for (int i = 0; i < 4; i++) {
			freeNode(parent->children[i]);
			parent->children[i] = nullptr;
		}
		collapse(parent);
	}
}

void QuadTree::subdivide(Node *node) {
	// Subdivide the node into four children.
	auto center = (node->bounds.min + node->bounds.max) / 2;
	Node *child;
	// bottom-left
	child = createNode(node->bounds.min, center);
	child->parent = node;
	node->children[static_cast<int>(QuadSegment::BOTTOM_LEFT)] = child;
	// bottom-right
	child = createNode(Vec2f(center.x, node->bounds.min.y), Vec2f(node->bounds.max.x, center.y));
	child->parent = node;
	node->children[static_cast<int>(QuadSegment::BOTTOM_RIGHT)] = child;
	// top-right
	child = createNode(center, node->bounds.max);
	child->parent = node;
	node->children[static_cast<int>(QuadSegment::TOP_RIGHT)] = child;
	// top-left
	child = createNode(Vec2f(node->bounds.min.x, center.y), Vec2f(center.x, node->bounds.max.y));
	child->parent = node;
	node->children[static_cast<int>(QuadSegment::TOP_LEFT)] = child;
}

inline ref_ptr<BoundingShape> initShape(const ref_ptr<BoundingShape> &shape) {
	shape->update();
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

std::pair<float, float> project(const Bounds<Vec2f> &b, const Vec2f &axis) {
	std::array<float, 4> projections = {
			Vec2f(b.min.x, b.min.y).dot(axis),
			Vec2f(b.max.x, b.min.y).dot(axis),
			Vec2f(b.min.x, b.max.y).dot(axis),
			Vec2f(b.max.x, b.max.y).dot(axis)
	};
	auto [minIt, maxIt] = std::minmax_element(projections.begin(), projections.end());
	return {*minIt, *maxIt};
}

std::pair<float, float> project(const std::vector<Vec2f> &points, const Vec2f &axis) {
	std::vector<float> projections(points.size());
	std::transform(points.begin(), points.end(), projections.begin(), [&axis](const Vec2f &point) {
		return point.dot(axis);
	});
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
		case OrthogonalProjection::Type::RECTANGLE: {
			// Check for separation along the axes of the rectangle and the axis-aligned quad
			std::array<Vec2f, 4> axes = {
					Vec2f(1, 0), Vec2f(0, 1),
					projection.points[1] - projection.points[0],
					projection.points[3] - projection.points[0]
			};
			for (const auto &axis: axes) {
				auto [minA, maxA] = project(bounds, axis);
				auto [minB, maxB] = project(projection.points, axis);
				if (maxA < minB || maxB < minA) {
					return false;
				}
			}
			return true;
		}
		case OrthogonalProjection::Type::TRIANGLE: {
			// Check for separation along the axes of the triangle and the axis-aligned quad
			std::array<Vec2f, 5> axes = {
					Vec2f(1, 0), Vec2f(0, 1),
					projection.points[1] - projection.points[0],
					projection.points[2] - projection.points[1],
					projection.points[0] - projection.points[2]
			};
			for (const auto &axis: axes) {
				auto [minA, maxA] = project(bounds, axis);
				auto [minB, maxB] = project(projection.points, axis);
				if (maxA < minB || maxB < minA) {
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

void QuadTree::Node::enlarge(const Bounds<Vec2f> &b, int resizeMask) {
	// b: the bounds that need to fit into the tree
	// resizeMask: a bitmask indicating which sides of the node can be resized

	// calculate the new bounds
	auto &newMin = bounds.min;
	auto &newMax = bounds.max;
	if (resizeMask & static_cast<int>(ResizeDirection::LEFT)) {
		//newMin.x = std::min(newMin.x, b.min.x);
		if (b.min.x < newMin.x) {
			newMin.x = b.min.x;
		} else {
			// remove the LEFT flag from the resize mask
			resizeMask &= ~static_cast<int>(ResizeDirection::LEFT);
		}
	}
	if (resizeMask & static_cast<int>(ResizeDirection::RIGHT)) {
		//newMax.x = std::max(newMax.x, b.max.x);
		if (b.max.x > newMax.x) {
			newMax.x = b.max.x;
		} else {
			// remove the RIGHT flag from the resize mask
			resizeMask &= ~static_cast<int>(ResizeDirection::RIGHT);
		}
	}
	if (resizeMask & static_cast<int>(ResizeDirection::TOP)) {
		//newMax.y = std::max(newMax.y, b.max.y);
		if (b.max.y > newMax.y) {
			newMax.y = b.max.y;
		} else {
			// remove the TOP flag from the resize mask
			resizeMask &= ~static_cast<int>(ResizeDirection::TOP);
		}
	}
	if (resizeMask & static_cast<int>(ResizeDirection::BOTTOM)) {
		//newMin.y = std::min(newMin.y, b.min.y);
		if (b.min.y < newMin.y) {
			newMin.y = b.min.y;
		} else {
			// remove the BOTTOM flag from the resize mask
			resizeMask &= ~static_cast<int>(ResizeDirection::BOTTOM);
		}
	}
	if (resizeMask == 0) {
		// nothing to do
		return;
	}

	// resize the children, and set the resize mask accordingly
	if (!isLeaf()) {
		// bottom-left node can only resize to the left and bottom
		auto &bottomLeft = children[static_cast<int>(QuadSegment::BOTTOM_LEFT)];
		int childResizeMask = 0;
		if (resizeMask & static_cast<int>(ResizeDirection::LEFT)) {
			childResizeMask |= static_cast<int>(ResizeDirection::LEFT);
		}
		if (resizeMask & static_cast<int>(ResizeDirection::BOTTOM)) {
			childResizeMask |= static_cast<int>(ResizeDirection::BOTTOM);
		}
		if (childResizeMask != 0) {
			bottomLeft->enlarge(bounds, childResizeMask);
		}

		// bottom-right node can only resize to the right and bottom
		auto &bottomRight = children[static_cast<int>(QuadSegment::BOTTOM_RIGHT)];
		childResizeMask = 0;
		if (resizeMask & static_cast<int>(ResizeDirection::RIGHT)) {
			childResizeMask |= static_cast<int>(ResizeDirection::RIGHT);
		}
		if (resizeMask & static_cast<int>(ResizeDirection::BOTTOM)) {
			childResizeMask |= static_cast<int>(ResizeDirection::BOTTOM);
		}
		if (childResizeMask != 0) {
			bottomRight->enlarge(bounds, childResizeMask);
		}

		// top-right node can only resize to the right and top
		auto &topRight = children[static_cast<int>(QuadSegment::TOP_RIGHT)];
		childResizeMask = 0;
		if (resizeMask & static_cast<int>(ResizeDirection::RIGHT)) {
			childResizeMask |= static_cast<int>(ResizeDirection::RIGHT);
		}
		if (resizeMask & static_cast<int>(ResizeDirection::TOP)) {
			childResizeMask |= static_cast<int>(ResizeDirection::TOP);
		}
		if (childResizeMask != 0) {
			topRight->enlarge(bounds, childResizeMask);
		}

		// top-left node can only resize to the left and top
		auto &topLeft = children[static_cast<int>(QuadSegment::TOP_LEFT)];
		childResizeMask = 0;
		if (resizeMask & static_cast<int>(ResizeDirection::LEFT)) {
			childResizeMask |= static_cast<int>(ResizeDirection::LEFT);
		}
		if (resizeMask & static_cast<int>(ResizeDirection::TOP)) {
			childResizeMask |= static_cast<int>(ResizeDirection::TOP);
		}
		if (childResizeMask != 0) {
			topLeft->enlarge(bounds, childResizeMask);
		}
	}
}

bool QuadTree::hasIntersection(const BoundingShape &shape) const {
	int count = 0;
	foreachIntersection(shape, [&count](const BoundingShape &shape) {
		count++;
	});
	return count > 0;
}

int QuadTree::numIntersections(const BoundingShape &shape) const {
	int count = 0;
	foreachIntersection(shape, [&count](const BoundingShape &shape) {
		count++;
	});
	return count;
}

void QuadTree::foreachIntersection(
		const BoundingShape &shape,
		const std::function<void(const BoundingShape &)> &callback) const {
	if (!root_) return;

	std::stack<Node *> stack;
	std::set<const Item *> visited;
	stack.push(root_);

	// project the shape onto the xz-plane for faster intersection tests
	// with the quad tree nodes.
	OrthogonalProjection shape_projection(shape);

	while (!stack.empty()) {
		Node *node = stack.top();
		stack.pop();

		// skip empty nodes
		if (node->isLeaf() && node->shapes.empty()) {
			continue;
		}

		// 2D intersection test with the xz-projection
		if (!node->intersects(shape_projection)) {
			continue;
		}

		if (node->isLeaf()) {
			// 3D intersection test with the shapes in the node
			for (const auto &quadShape: node->shapes) {
				if (visited.find(quadShape) != visited.end()) {
					continue;
				}
				visited.insert(quadShape);
				if (quadShape->shape->hasIntersectionWith(shape)) {
					callback(*quadShape->shape.get());
				}
			}
		} else {
			// Add the children to the stack
			for (auto &child: node->children) {
				if (child) {
					stack.push(child);
				}
			}
		}
	}
}

void QuadTree::update(float dt) {
	// iterate over all items in the tree, and update them.
	// update returns true if the shape has changed, in this case
	// the item is removed and reinserted into the tree.
	for (const auto &item: items_) {
		if (item->shape->update()) {
			item->projection = OrthogonalProjection(*item->shape.get());
			// remove/insert item
			remove(item);
			insert(item);
		}
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
	Vec3f lineColor(1, 0, 0);

	// draw the bounds of nodes
	std::stack<const Node *> stack;
	stack.push(root_);
	while (!stack.empty()) {
		const Node *node = stack.top();
		stack.pop();
		debug.drawLine(
				Vec3f(node->bounds.min.x, 0, node->bounds.min.y),
				Vec3f(node->bounds.max.x, 0, node->bounds.min.y),
				lineColor);
		debug.drawLine(
				Vec3f(node->bounds.max.x, 0, node->bounds.min.y),
				Vec3f(node->bounds.max.x, 0, node->bounds.max.y),
				lineColor);
		debug.drawLine(
				Vec3f(node->bounds.max.x, 0, node->bounds.max.y),
				Vec3f(node->bounds.min.x, 0, node->bounds.max.y),
				lineColor);
		debug.drawLine(
				Vec3f(node->bounds.min.x, 0, node->bounds.max.y),
				Vec3f(node->bounds.min.x, 0, node->bounds.min.y),
				lineColor);
		if (!node->isLeaf()) {
			for (int i = 0; i < 4; i++) {
				stack.push(node->children[i]);
			}
		}
	}

	// draw 2d projections of the shapes
	lineColor = Vec3f(0, 1, 0);
	const GLfloat h = 0.1f;
	for (auto &item: items_) {
		auto &projection = item->projection;
		auto &points = projection.points;
		switch (projection.type) {
			case OrthogonalProjection::Type::CIRCLE: {
				auto radius = std::sqrt(points[1].x);
				debug.drawCircle(toVec3(points[0], h), radius, lineColor);
				break;
			}
			case OrthogonalProjection::Type::RECTANGLE: {
				debug.drawLine(toVec3(points[0], h), toVec3(points[1], h), lineColor);
				debug.drawLine(toVec3(points[1], h), toVec3(points[2], h), lineColor);
				debug.drawLine(toVec3(points[2], h), toVec3(points[3], h), lineColor);
				debug.drawLine(toVec3(points[3], h), toVec3(points[0], h), lineColor);
				break;
			}
			case OrthogonalProjection::Type::TRIANGLE: {
				debug.drawLine(toVec3(points[0], h), toVec3(points[1], h), lineColor);
				debug.drawLine(toVec3(points[1], h), toVec3(points[2], h), lineColor);
				debug.drawLine(toVec3(points[2], h), toVec3(points[0], h), lineColor);
				break;
			}
		}
	}
}
