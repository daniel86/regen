#include <stack>
#include <chrono>
#include <limits>

#include "quad-tree.h"

#undef QUAD_TREE_THREADING
#undef QUAD_TREE_DEBUG
#define QUAD_TREE_EVER_GROWING
#define QUAD_TREE_SQUARED
#define QUAD_TREE_SUBDIVIDE_THRESHOLD 4
#define QUAD_TREE_COLLAPSE_THRESHOLD 2

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
	std::set<const BoundingShape *> shapes;
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

void QuadTree::freeNode(Node *node) {
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

QuadTree::Item* QuadTree::getItem(const ref_ptr<BoundingShape> &shape) {
	auto it = items_.find(shape.get());
	if (it != items_.end()) {
		return it->second;
	}
	return nullptr;
}

void QuadTree::insert(const ref_ptr<BoundingShape> &shape) {
	newItems_.push_back(new Item(shape));
	addToIndex(shape);
}

bool QuadTree::insert(Node *node, Item *shape, bool allowSubdivision) { // NOLINT(no-recursion)
	if (!node->intersects(shape->projection)) {
		return false;
	} else {
		// only subdivide if the node is larger than the shape
		auto nodeSize = node->bounds.max - node->bounds.min;
		auto shapeSize = shape->projection.bounds().max - shape->projection.bounds().min;
		bool isNodeLargeEnough = (nodeSize.x > 2.0*shapeSize.x && nodeSize.y > 2.0*shapeSize.y);

		return insert1(node, shape, isNodeLargeEnough && allowSubdivision);
	}
}

bool QuadTree::insert1(Node *node, Item *newShape, bool allowSubdivision) { // NOLINT(no-recursion)
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
		// the node has child nodes, must insert into one of them
		// note: a shape can be inserted into multiple nodes, so we allways need to check all children
		//       (at least on the next level)
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

void QuadTree::collapse(Node *node) {
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
	if (parent->children[0]->shapes.size() > QUAD_TREE_COLLAPSE_THRESHOLD) {
		// no collapse
		return;
	}
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

std::pair<float, float> project(const Bounds<Vec2f> &b, const Vec2f &axis) {
	std::array<float, 4> projections = {
		b.min.dot(axis),
		Vec2f(b.max.x, b.min.y).dot(axis),
		Vec2f(b.min.x, b.max.y).dot(axis),
		b.max.dot(axis)
	};

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

#ifdef QUAD_TREE_THREADING
namespace regen {
	class QuadTreeWorker : public ThreadPool::Runner {
	public:
		using Callback = std::function<void(const BoundingShape &)>;

		ThreadPool *threadPool;
		const BoundingShape *shape;
		const OrthogonalProjection *projection;
		const QuadTree::Node *node;
		std::set<const QuadTree::Item *> *visited;
		std::mutex *mutex;
		const Callback &callback;
		std::vector<std::shared_ptr<QuadTreeWorker>> children_;

		explicit QuadTreeWorker(const Callback &callback)
				: callback(callback) {
		}

		~QuadTreeWorker() override = default;

		void run() override {
			// 2D intersection test with the xz-projection
			if (!node->intersects(*projection)) {
				return;
			}
			if (node->isLeaf()) {
				// 3D intersection test with the shapes in the node
				for (const auto &quadShape: node->shapes) {
					{
						std::lock_guard<std::mutex> lock(*mutex);
						if (visited->find(quadShape) != visited->end()) {
							continue;
						}
						visited->insert(quadShape);
					}
					if (quadShape->shape->hasIntersectionWith(*shape)) {
						callback(*quadShape->shape.get());
					}
				}
			}
			else {
				static auto excHandler = [](const std::exception &) {};
				// Add the children to the stack
				for (auto &child: node->children) {
					if (child && (!child->isLeaf() || !child->shapes.empty())) {
						auto childWorker = std::make_shared<QuadTreeWorker>(callback);
						childWorker->threadPool = threadPool;
						childWorker->node = child;
						childWorker->shape = shape;
						childWorker->projection = projection;
						childWorker->visited = visited;
						childWorker->mutex = mutex;
						threadPool->pushWork(childWorker, excHandler);
						children_.push_back(childWorker);
					}
				}
			}
		}
	};
}
#endif

void QuadTree::foreachIntersection(
		const BoundingShape &shape,
		const std::function<void(const BoundingShape &)> &callback) {
	if (!root_) return;
	if (root_->isLeaf() && root_->shapes.empty()) return;

#ifdef QUAD_TREE_DEBUG
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::milliseconds;
	GLuint num2DTests = 0;
	GLuint num3DTests = 0;
	auto t1 = high_resolution_clock::now();
#endif

	std::set<const Item *> visited;
	// project the shape onto the xz-plane for faster intersection tests
	// with the quad tree nodes.
	OrthogonalProjection shape_projection(shape);

#ifdef QUAD_TREE_THREADING
	std::mutex mutex;
	//std::atomic<unsigned int> jobCounter(1);
	//auto runner = std::make_shared<ThreadPool::LambdaRunner>([&](const ThreadPool::LambdaRunner::StopChecker&) {
	//	intersect2D(shape, shape_projection, root_, jobCounter, visited, mutex, callback);
	//});
	//threadPool_.pushWork(runner, [](const std::exception &) {});
	auto firstWorker = std::make_shared<QuadTreeWorker>(callback);
	firstWorker->threadPool = &threadPool_;
	firstWorker->node = root_;
	firstWorker->shape = &shape;
	firstWorker->projection = &shape_projection;
	firstWorker->visited = &visited;
	firstWorker->mutex = &mutex;
	threadPool_.pushWork(firstWorker, [](const std::exception &) {});

	std::stack<QuadTreeWorker*> stack;
	stack.push(firstWorker.get());
	while (!stack.empty()) {
		auto *worker = stack.top();
		stack.pop();
		if (!worker->isTerminated()) {
			worker->join();
		}
		for (auto &child: worker->children_) {
			stack.push(child.get());
		}
	}
#else
	std::stack<Node *> stack;
	stack.push(root_);

	while (!stack.empty()) {
		Node *node = stack.top();
		stack.pop();

		// skip empty nodes
		if (node->isLeaf() && node->shapes.empty()) {
			continue;
		}

#ifdef QUAD_TREE_DEBUG
		num2DTests++;
#endif
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
#ifdef QUAD_TREE_DEBUG
				num3DTests++;
#endif
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
#endif

#ifdef QUAD_TREE_DEBUG
	auto t2 = high_resolution_clock::now();
	duration<double, std::milli> ms_double = t2 - t1;
	REGEN_INFO("QUAD TREE STATS");
	REGEN_INFO("     time: " << ms_double.count() << " ms");
	unsigned int numShapes = 0;
	for (const auto &x: shapes_) {
		numShapes += x.second.size();
	}
	REGEN_INFO("     #Shapes: " << numShapes);
	REGEN_INFO("     #Nodes: " << numNodes());
	REGEN_INFO("     #2D Tests: " << num2DTests);
	REGEN_INFO("     #3D Tests: " << num3DTests);
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
			// TODO: rather update the old projection instead of creating a new one
			item->projection = OrthogonalProjection(*item->shape.get());
			changedItems_.push_back(item);
		}
		newBounds_.extend(item->projection.bounds());
	}
	// do the same for any additional items added to the tree
	for (const auto &item: newItems_) {
		hasChanged = item->shape->updateGeometry();
		hasChanged = item->shape->updateTransform(hasChanged) || hasChanged;
		if (hasChanged) {
			item->projection = OrthogonalProjection(*item->shape.get());
		}
		newBounds_.extend(item->projection.bounds());
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
			delete item;
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
